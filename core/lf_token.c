/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
 *
 * @section LICENSE
 * Copyright (c) 2022, The University of California at Berkeley.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @section DESCRIPTION
 *
 * Functions supporting token types.  See lf_token.h for docs.
 */

#include <stdbool.h>
#include <assert.h>
#include <string.h>  // Defines memcpy
#include "lf_token.h"
#include "hashset/hashset_itr.h"
#include "util.h"

lf_token_t* _lf_tokens_allocated_in_reactions = NULL;

////////////////////////////////////////////////////////////////////
//// Global variables not visible outside this file.

/**
 * Tokens always have the same size in memory so they are easily recycled.
 * When a token is freed, it is inserted into this recycling bin.
 */
static hashset_t _lf_token_recycling_bin = NULL;

/**
 * To allow a system to recover from burst of activity, the token recycling
 * bin has a limited size. When it becomes full, token are freed using free().
 */
#define _LF_TOKEN_RECYCLING_BIN_SIZE_LIMIT 512

/**
 * Set of token templates (trigger_t or port_base_t objects) that
 * have been initialized. This is used to free their tokens at
 * the end of program execution. 
 */
static hashset_t _lf_token_templates = NULL;

////////////////////////////////////////////////////////////////////
//// Internal functions.

void _lf_free_token_value(lf_token_t* token) {
    if (token->value != NULL) {
        // Count frees to issue a warning if this is never freed.
        _lf_count_payload_allocations--;
        // Free the value field (the payload).
        // First check whether the value field is garbage collected (e.g. in the
        // Python target), in which case the payload should not be freed.
#ifndef _LF_GARBAGE_COLLECTED
        LF_PRINT_DEBUG("_lf_free_token_value: Freeing allocated memory for payload (token value): %p",
                token->value);
        if (token->type->destructor == NULL) {
            free(token->value);
        } else {
            token->type->destructor(token->value);
        }
#endif
        token->value = NULL;
    }
}

token_freed _lf_free_token(lf_token_t* token) {
    LF_PRINT_DEBUG("_lf_free_token: %p", token);
    token_freed result = NOT_FREED;
    if (token == NULL) return result;
    if (token->ref_count > 0) return result;
    _lf_free_token_value(token);

    // Tokens that are created at the start of execution and associated with
    // output ports or actions are pointed to by those actions and output
    // ports and should not be freed. They are expected to be reused instead.
    // FIXME: Need to acquire a mutex to access the recycle bin.
    if (_lf_token_recycling_bin == NULL) {
        _lf_token_recycling_bin = hashset_create(4); // Initial size is 16.
        if (_lf_token_recycling_bin == NULL) {
            lf_print_error_and_exit("Out of memory: failed to setup _lf_token_recycling_bin");
        }
    }
    if (hashset_num_items(_lf_token_recycling_bin) < _LF_TOKEN_RECYCLING_BIN_SIZE_LIMIT) {
        // Recycle instead of freeing.
        LF_PRINT_DEBUG("_lf_free_token: Putting token on the recycling bin: %p", token);
        if (!hashset_add(_lf_token_recycling_bin, token)) {
            lf_print_warning("Putting token %p on the recycling bin, but it is already there!", token);
        }
    } else {
        // Recycling bin is full.
        LF_PRINT_DEBUG("_lf_free_token: Freeing allocated memory for token: %p", token);
        free(token);
    }
    _lf_count_token_allocations--;
    result &= TOKEN_FREED;

    return result;
}

lf_token_t* _lf_new_token(token_type_t* type, void* value, size_t length) {
    lf_token_t* result = NULL;
    // Check the recycling bin.
    // FIXME: Need a mutex lock on this! Perhaps condition on threading.
    if (_lf_token_recycling_bin != NULL) {
        hashset_itr_t iterator = hashset_iterator(_lf_token_recycling_bin);
        if (hashset_iterator_next(iterator) >= 0) {
            result = hashset_iterator_value(iterator);
            hashset_remove(_lf_token_recycling_bin, result);
            LF_PRINT_DEBUG("_lf_new_token: Retrieved token from the recycling bin: %p", result);
        }
        free(iterator);
    }
    if (result == NULL) {
        // Nothing found on the recycle bin.
        result = (lf_token_t*)calloc(1, sizeof(lf_token_t));
        LF_PRINT_DEBUG("_lf_new_token: Allocated memory for token: %p", result);
    }
    result->type = type;
    result->length = length;
    result->value = value;
    result->ref_count = 0;
    return result;
}

lf_token_t* _lf_get_token(token_template_t* template) {
    if (template->token != NULL) {
        if (template->token->ref_count <= 1) {
            LF_PRINT_DEBUG("_lf_get_token: Reusing template token: %p with ref_count %zu",
                    template->token, template->token->ref_count);
            // Free any previous value in the token.
            if (template->token->value != NULL) {
                if (template->token->type->destructor == NULL) {
                    free(template->token->value);
                } else {
                    template->token->type->destructor(template->token->value);
                }
                template->token->value = NULL;
                _lf_count_payload_allocations--;
            }
            return template->token;
        } else {
            // Liberate the token.
            _lf_done_using(template->token);
        }
    }
    // If we get here, we need a new token.
    template->token = _lf_new_token((token_type_t*)template, NULL, 0);
    template->token->ref_count = 1;
    return template->token;
}

void _lf_initialize_template(token_template_t* template, size_t element_size) {
    assert(template != NULL);
    if (_lf_token_templates == NULL) {
        _lf_token_templates = hashset_create(4); // Initial size is 16.
    }
    hashset_add(_lf_token_templates, template);
    if (template->token != NULL) {
        if (template->token->ref_count == 1 && template->token->type->element_size == element_size) {
            // Template token is already set.
            // If it has a value, free it.
            _lf_free_token_value(template->token);
            // Make sure its reference count is 1 (it should not be 0).
            template->token->ref_count = 1;
            return;
        }
        // Replace the token.
        _lf_done_using(template->token);
        template->token = NULL;
    }
    template->type.element_size = element_size;
    template->token = _lf_new_token((token_type_t*)template, NULL, 0);
    template->token->ref_count = 1;
}

lf_token_t* _lf_initialize_token_with_value(token_template_t* template, void* value, size_t length) {
    assert(template != NULL);
    LF_PRINT_DEBUG("_lf_initialize_token_with_value: template %p, value %p", template, value);
    lf_token_t* result = _lf_get_token(template);
    result->value = value;
    // Count allocations to issue a warning if this is never freed.
    _lf_count_payload_allocations++;
    result->length = length;
    return result;
}

lf_token_t* _lf_initialize_token(token_template_t* template, size_t length) {
    assert(template != NULL);
    // Allocate memory for storing the array.
    void* value = calloc(length, template->type.element_size);
    lf_token_t* result = _lf_initialize_token_with_value(template, value, length);
    return result;
}

void _lf_free_all_tokens() {
    // Free template tokens.
    // It is possible for a token to be a template token for more than one port
    // or action because the same token may be sent to multiple output ports.
    if (_lf_token_templates != NULL) {
        hashset_itr_t iterator = hashset_iterator(_lf_token_templates);
        while (hashset_iterator_next(iterator) >= 0) {
            token_template_t* template = (token_template_t*)hashset_iterator_value(iterator);
            _lf_done_using(template->token);
            template->token = NULL;
        }
        free(iterator);
        hashset_destroy(_lf_token_templates);
        _lf_token_templates = NULL;
    }

    if (_lf_token_recycling_bin != NULL) {
        hashset_itr_t iterator = hashset_iterator(_lf_token_recycling_bin);
        while (hashset_iterator_next(iterator) >= 0) {
            void* token = hashset_iterator_value(iterator);
            LF_PRINT_DEBUG("Freeing token from _lf_token_recycling_bin: %p", token);
            // Payload should already be freed, so we just free the token:
            free(token);
        }
        free(iterator);
        hashset_destroy(_lf_token_recycling_bin);
        _lf_token_recycling_bin = NULL;
    }
}

void _lf_replace_template_token(token_template_t* template, lf_token_t* newtoken) {
    assert(template != NULL);
    if (template->token != newtoken) {
        if (template->token != NULL) {
            _lf_done_using(template->token);
        }
        newtoken->ref_count++;
        LF_PRINT_DEBUG("_lf_replace_template_token: Incremented ref_count of %p to %zu.",
                newtoken, newtoken->ref_count);
        template->token = newtoken;
    }
}

token_freed _lf_done_using(lf_token_t* token) {
    if (token == NULL) {
        return NOT_FREED;
    }
    LF_PRINT_DEBUG("_lf_done_using: token = %p, ref_count = %zu.", token, token->ref_count);
    if (token->ref_count == 0) {
        lf_print_warning("Token being freed that has already been freed: %p", token);
        return NOT_FREED;
    }
    token->ref_count--;
    return _lf_free_token(token);
}

lf_token_t* lf_writable_copy(token_template_t* template) {
    assert(template != NULL);
    lf_token_t* token = template->token;
    if (token == NULL) return NULL;
    LF_PRINT_DEBUG("lf_writable_copy: Requesting writable copy of token %p with reference count %zu.",
            token, token->ref_count);
    // FIXME: token_template_t needs to be augmented with a field
    // single_reader that is set to true in the code generator
    // for an output port or action that triggers exactly one
    // downstream reaction. That should be tested here.
    // Search for where "dominating" field of ReactionInstance is populated.
    // For now, always copy. Or use num_destinations field.
    if (false /* template->single_reader */ && token->ref_count == 1) {
        LF_PRINT_DEBUG("lf_writable_copy: Avoided copy because there "
                "is only one reader and the reference count is %zu.", token->ref_count);
        return token;
    }
    LF_PRINT_DEBUG("lf_writable_copy: Copying value. Reference count is %zu.",
            token->ref_count);
    // Copy the payload.
    void* copy;
    if (template->type.copy_constructor == NULL) {
        LF_PRINT_DEBUG("lf_writable_copy: Copy constructor is NULL. Using default strategy.");
        size_t size = template->type.element_size * token->length;
        if (size == 0) {
            return token;
        }
        copy = malloc(size);
        LF_PRINT_DEBUG("Allocating memory for writable copy %p.", copy);
        memcpy(copy, token->value, size);
    } else {
        LF_PRINT_DEBUG("lf_writable_copy: Copy constructor is not NULL. Using copy constructor.");
        if (template->type.destructor == NULL) {
            lf_print_warning("lf_writable_copy: Using non-default copy constructor "
                    "without setting destructor. Potential memory leak.");
        }
        copy = template->type.copy_constructor(token->value);
    }
    LF_PRINT_DEBUG("lf_writable_copy: Allocated memory for payload (token value): %p", copy);

    // Count allocations to issue a warning if this is never freed.
    _lf_count_payload_allocations++;

    // Create a new, dynamically allocated token.
    lf_token_t* result = _lf_new_token((token_type_t*)template, copy, token->length);
    result->ref_count = 1;
    // Arrange for the token to be released (and possibly freed) at
    // the start of the next time step.
    result->next = _lf_tokens_allocated_in_reactions;
    _lf_tokens_allocated_in_reactions = result;

    return result;
}

void _lf_free_token_copies() {
    while (_lf_tokens_allocated_in_reactions != NULL) {
        lf_token_t* next = _lf_tokens_allocated_in_reactions->next;
        _lf_done_using(_lf_tokens_allocated_in_reactions);
        _lf_tokens_allocated_in_reactions = next;
    }
}