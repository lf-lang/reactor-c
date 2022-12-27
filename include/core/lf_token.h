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
 * This header file supports token objects, which are reference-counted wrappers
 * around values that are carried by events scheduled on the event queue and held
 * in ports and actions when the type is not a primitive type.
 * 
 * A token has type lf_token_t. It points to a value, a dynamically allocated
 * chunk of memory on the heap. It has a length field, which enables its value
 * to be interpreted as an array of the given length. It has a pointer to type
 * (token_type_t), which has an element_size field specifying the size of each
 * array element (or the size of the whole value if it is not an array and has
 * length 1). The type also optionally has function pointers to a destructor
 * and copy constructor. These must be specified if the payload (value) is a complex
 * struct that cannot be freed by a simple call to free() or copied by a call
 * to memcpy().
 * 
 * An instance of a port struct and trigger_t struct (an action or an input port)
 * can be cast to token_template_t, which has a token_type_t field called type
 * and a pointer to a token (which may be NULL).  The same instance can also be
 * cast to token_type_t, which has an element_size field and (possibly) function
 * pointers to a destructor and a copy constructor.
 * 
 * A "template token" is one pointed to by a token_template_t (an action or a port).
 * Such a token will persist in the template until it is overwritten by another
 * token. 
 */

#ifndef LF_TOKEN_H
#define LF_TOKEN_H

#include <stdlib.h> // Defines size_t

//////////////////////////////////////////////////////////
//// Constants and enums

/** Possible return values for _lf_done_using and _lf_free_token. */
typedef enum token_freed {
    NOT_FREED = 0, // Nothing was freed.
    VALUE_FREED,   // The value (payload) was freed.
    TOKEN_FREED,    // The token was freed but not the value.
    TOKEN_AND_VALUE_FREED // Both were freed
} token_freed;

//////////////////////////////////////////////////////////
//// Data structures

/**
 * @brief Type information for tokens.
 * Specifically, this struct contains the fields needed to support
 * token types, which carry dynamically allocated data.
 */
typedef struct token_type_t {
    /** Size of the struct or array element. */
    size_t element_size;
    /** The destructor or NULL to use the default free(). */
    void (*destructor) (void* value);
    /** The copy constructor or NULL to use memcpy. */
    void* (*copy_constructor) (void* value);
} token_type_t;

/**
 * Token type for dynamically allocated arrays and structs sent as messages.
 *
 * This struct is the wrapper around the dynamically allocated memory
 * that carries the message.  The message can be an array of values,
 * where the size of each value is element_size (in bytes). If it is
 * not an array, or is not to be treated as an array, the length == 1.
 *
 * In the C LF target, a type for an output that ends in '*' or '[]' is
 * treated specially. The value carried by the output is assumed
 * to be in dynamically allocated memory, and, using reference
 * counting, after the last downstream reader of the value has
 * finished, the memory will be freed.  To prevent this freeing
 * from occurring, the output type can be specified using the
 * syntax {= type* =}; this will not be treated as dynamically
 * allocated memory. Alternatively, the programmer can give a typedef
 * in the preamble that masks the trailing *.
 */
typedef struct lf_token_t {
    /** Pointer to dynamically allocated memory containing a message. */
    void* value;
    /** Length of the array or 1 for a non-array. */
    size_t length;
    /** Pointer to the port or action defining the type of the data carried. */
    token_type_t* type;
    /** The number of times this token is on the event queue. */
    size_t ref_count;
    /** Indicator that the token is a template token and should not be freed. */
    bool is_template;
    /** Convenience for constructing a temporary list of tokens. */
    struct lf_token_t* next;
} lf_token_t;

/**
 * @brief Base type for ports (port_base_t) and actions (trigger_t), which can carry tokens.
 * The structs port_base_t and trigger_t should start with an instance of this struct
 * so that they can be cast to this struct to access these fields in a uniform way.
 */
typedef struct token_template_t {
	/** Instances of this struct can be cast to token_type_t. */
	token_type_t type;
	lf_token_t* token;
    size_t length;       // The token's length, for convenient access.
} token_template_t;

//////////////////////////////////////////////////////////
//// Global variables

/**
 * @brief List of tokens created within reactions that must be freed.
 * Tokens created by lf_writable_copy, which is automatically invoked
 * when an input is mutable, must have their reference count decremented
 * at the end of a tag (or the beginning of the next tag).
 * Otherwise, their memory could leak. If they are passed on to
 * an output or to a call to lf_schedule during the reaction, then
 * those will also result in incremented reference counts, enabling
 * the token to live on until used.
 */
extern lf_token_t* _lf_tokens_allocated_in_reactions;

/**
 * Counter used to issue a warning if memory is
 * allocated for message payloads and never freed.
 */
static int _lf_count_payload_allocations;

/**
 * Counter used to issue a warning if memory is
 * allocated for tokens and never freed. Note that
 * every trigger will have one token allocated for
 * it. That token is not counted because it is not
 * expected to be freed.
 */
static int _lf_count_token_allocations;

//////////////////////////////////////////////////////////
//// Functions that users may call

/**
 * Return a writable copy of the token in the specified template.
 * If the reference count is 1, this returns the template's token
 * rather than a copy. The reference count will be left at 1.
 * Otherwise, if the size of the token payload is zero, this also
 * returns the original token, again with reference count left as it is.
 * Otherwise, this returns a new token with a reference count of 1.
 * The new token is added to a list of tokens whose reference counts will
 * be decremented at the start of the next tag.
 * If the template has no token (it has a primitive type), then there
 * is no need for a writable copy. Return NULL.
 * @param template An action or an input port.
 */
lf_token_t* lf_writable_copy(token_template_t* template);

//////////////////////////////////////////////////////////
//// Functions not intended to be used by users

/**
 * @brief Free the specified token, if appropriate.
 * If the reference count is greater than 0, then do not free 
 * anything. Otherwise, the token value (payload) will be freed.
 * The token itself will be freed if its is_template field is false.
 * The freed token will be put on the recycling bin unless that
 * bin has reached the designated capacity, in which case free()
 * will be used.
 *
 * @param token Pointer to a token.
 * @return NOT_FREED if nothing was freed, VALUE_FREED if the value
 *  was freed, TOKEN_FREED if only the token was freed, and
 *  TOKEN_AND_VALUE_FREED if both the value and the token were freed.
 */
token_freed _lf_free_token(lf_token_t* token);

/**
 * @brief Return a new token with the specified type.
 * This will attempt to get one from the recyling bin, and, if the
 * recycling bin is empty, will allocate a new token using calloc
 * and set its type to point to the specified type.
 * The length of the returned token will be 0 and it will indicate
 * that it is not a template token.
 * @param type The type of the token
 * @return lf_token_t* 
 */
lf_token_t* lf_new_token(token_type_t* type);

/**
 * Get a token for the specified template.
 * If the template already has a token and the reference count is 0,
 * then return that token. Otherwise, create a new token,
 * make it the new template, and dissociate or free the
 * previous template token.
 * @param template The template.
 * @return A new or recycled lf_token_t struct.
 */
lf_token_t* _lf_get_token(token_template_t* template);

/**
 * Initialize the specified template to contain tokens that are
 * arrays with the specified size. Upon return, the token field will
 * be non-NULL. If there was previously a token with a reference
 * count greater than 0 or a non-matching type, it will be replaced.
 * The length of the returned token will be 0.
 * @param template The template.
 * @param element_size The element size.
 */
void _lf_initialize_template(token_template_t* template, size_t element_size);

/**
 * Return a token storing the specified value, which is assumed to
 * be either a scalar (if length is 1) or an array of the specified length.
 * If the token in the specified template is available (it non-null and its
 * reference count is 0), then return it. Otherwise, create a new token
 * and replace the template token with the new one, freeing the
 * previous token from its template association.
 * The element_size for elements of the array is specified by
 * the specified template.
 *
 * @param template A template for the token.
 * @param value The value of the array.
 * @param length The length of the array, or 1 if it is not an array.
 * @return Either the specified token or a new one, in each case with a value
 *  field pointing to newly allocated memory.
 */
lf_token_t* _lf_initialize_token_with_value(token_template_t* template, void* value, size_t length);

/**
 * Return a token for storing an array of the specified length
 * with new memory allocated (using calloc, so initialize to zero)
 * for storing that array. If the template's token is available
 * (it is non-null and its reference count is 0), then reuse it.
 * Otherwise, create a new token and replace the template token
 * with the new one, freeing the previous token from its template
 * association. The element_size for elements
 * of the array is specified by the specified template. The caller
 * should populate the value and ref_count field of the returned
 * token after this returns.
 *
 * @param template The token template (must not be NULL).
 * @param length The length of the array, or 1 if it is not an array.
 * @return Either the template's token or a new one, in each case with a value
 *  field pointing to newly allocated memory.
 */
lf_token_t* _lf_initialize_token(token_template_t* template, size_t length);

/**
 * @brief Free all tokens.
 * Free tokens on the _lf_token_recycling_bin hashset and all
 * template tokens.
 */
void _lf_free_all_tokens();

/**
 * @brief Replace the token in the specified template, if there is one,
 * with a new one. If the new tokens is the same as the token in the template,
 * then this does nothing. Otherwise, it frees the previous template token.
 * @param template Pointer to a template.
 * @param newtoken The replacement token.
 */
void _lf_replace_template_token(token_template_t* template, lf_token_t* newtoken);

/**
 * Decrement the reference count of the specified token.
 * If the reference count hits 0, free the memory for the value
 * carried by the token, and, if the token is not also the template
 * token of its trigger, free the token.
 * @param token Pointer to a token.
 * @return NOT_FREED if nothing was freed, VALUE_FREED if the value
 *  was freed, TOKEN_FREED if only the token was freed, and
 *  TOKEN_AND_VALUE_FREED if both the value and the token were freed.
 */
token_freed _lf_done_using(lf_token_t* token);

/**
 * @brief Free token copies made for mutable inputs.
 * This function should be called at the beginning of each time step
 * to avoid memory leaks.
 */
void _lf_free_token_copies();

#endif /* LF_TOKEN_H */
/** @} */
