/**
 * @file lf_token.h
 * @brief Definitions for token objects, reference-counted wrappers around dynamically-allocated messages.
 * @ingroup Internal
 * @author Edward A. Lee
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
 * pointers to a destructor and copy constructor.
 *
 * A "template token" is one pointed to by a token_template_t (an action or a port).
 * This template token ensures that port an action values persist until they are
 * overwritten, and hence they can be read at a tag even if not present.
 * Such a token will persist in the template until it is overwritten by another
 * token. Every token_template_t gets initialized with such a token.
 * Before that token is used the first time, its reference count will be 0.
 * Once it has been assigned a value, its reference count will be 1.
 * When the token_template_t (port or action) is assigned a new value, if
 * the reference count is 1, then the same token will be reused, and
 * any previous value (payload) will be freed.
 */

#ifndef LF_TOKEN_H
#define LF_TOKEN_H

#include <stdlib.h>  // Defines size_t
#include <stdbool.h> // Defines bool type

// Forward declarations
struct environment_t;

//////////////////////////////////////////////////////////
//// Constants and enums

/**
 * @brief Possible return values for @ref _lf_done_using and @ref _lf_free_token.
 * @ingroup Internal
 */
typedef enum token_freed {
  NOT_FREED = 0,        // Nothing was freed.
  VALUE_FREED,          // The value (payload) was freed.
  TOKEN_FREED,          // The token was freed but not the value.
  TOKEN_AND_VALUE_FREED // Both were freed
} token_freed;

//////////////////////////////////////////////////////////
//// Data structures

/**
 * @brief Type information for tokens.
 * @ingroup Internal
 * Specifically, this struct contains the fields needed to support
 * token types, which carry dynamically allocated data.
 */
typedef struct token_type_t {
  /** @brief Size of the struct or array element. */
  size_t element_size;
  /** @brief The destructor or NULL to use the default free(). */
  void (*destructor)(void* value);
  /** @brief The copy constructor or NULL to use memcpy. */
  void* (*copy_constructor)(void* value);
} token_type_t;

/**
 * @brief Token type for dynamically allocated arrays and structs sent as messages.
 * @ingroup API
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
  /** @brief Pointer to dynamically allocated memory containing a message. */
  void* value;
  /** @brief Length of the array or 1 for a non-array. */
  size_t length;
  /** @brief Pointer to the port or action defining the type of the data carried. */
  token_type_t* type;
  /** @brief The number of times this token is on the event queue. */
  size_t ref_count;
  /** @brief Convenience for constructing a temporary list of tokens. */
  struct lf_token_t* next;
} lf_token_t;

/**
 * @brief A record of the subset of channels of a multiport that have present inputs.
 * @ingroup Internal
 *
 * This struct is used to efficiently track which channels of a multiport
 * have present inputs, particularly useful for sparse I/O operations where
 * only a small subset of channels are active.
 */
typedef struct lf_sparse_io_record_t {
  /**
   * @brief Number of present channels or status indicator.
   *
   * -1 indicates the record has overflowed (too many present channels),
   * 0 indicates no channels are present,
   * positive values indicate the number of present channels.
   */
  int size;

  /**
   * @brief Maximum number of channels that can be tracked before overflow.
   *
   * When the number of present channels exceeds this capacity,
   * the record is considered overflowed and size is set to -1.
   */
  size_t capacity;

  /**
   * @brief Array of channel indices that have present inputs.
   *
   * Stores the indices of channels that have present inputs.
   * Only valid when size > 0. The array size is determined by capacity.
   */
  size_t* present_channels;
} lf_sparse_io_record_t;

/**
 * @brief Base type for ports (lf_port_base_t) and actions (trigger_t), which can carry tokens.
 * @ingroup Internal
 *
 * The structs lf_port_base_t and trigger_t should start with an instance of this struct
 * so that they can be cast to this struct to access these fields in a uniform way.
 * This template provides the common structure for handling tokens in both ports and actions,
 * ensuring consistent token management across different types of connections.
 */
typedef struct token_template_t {
  /**
   * @brief Type information for the token.
   *
   * This field means a token_template_tcan be cast to token_type_t to access type-specific
   * information such as element size, destructor, and copy constructor. This allows for
   * uniform handling of different token types while maintaining type safety.
   */
  token_type_t type;

  /**
   * @brief Pointer to the current token.
   *
   * Points to the token currently associated with this template.
   * May be NULL if no token is currently assigned. The token's
   * reference count is managed by the runtime system to ensure
   * proper memory management.
   */
  lf_token_t* token;

  /**
   * @brief Length of the token's value array.
   *
   * Provides convenient access to the token's length in reactions.
   * For non-array values, this will be 1. For arrays, this indicates
   * the number of elements in the array. This field is cached here
   * to avoid repeated dereferencing of the token structure.
   */
  size_t length;
} token_template_t;

// Forward declaration for self_base_t
typedef struct self_base_t self_base_t;

/**
 * @brief Base type for ports.
 * @ingroup Internal
 *
 * Port structs are customized types because their payloads are type
 * specific. This struct represents their common features. Given any
 * pointer to a port struct, it can be cast to lf_port_base_t and then
 * these common fields can be accessed.
 *
 * IMPORTANT: If this is changed, it must also be changed in
 * CPortGenerator.java generateAuxiliaryStruct().
 */
typedef struct lf_port_base_t {
  /**
   * @brief Template containing type and token information.
   *
   * This field contains the common token handling structure that
   * allows the port to carry typed values. The template provides
   * type information and manages the token's lifecycle.
   * @note 'template' is a C++ keyword, hence the abbreviated name.
   */
  token_template_t tmplt;

  /**
   * @brief Indicates whether the port has a present value.
   *
   * Set to true when the port has a value present at the current tag.
   * This flag is used to determine whether the port's value should be
   * considered in reactions.
   */
  bool is_present;

  /**
   * @brief Record of present channels for sparse I/O.
   *
   * Points to a record that tracks which channels of a multiport
   * have present inputs. NULL if this port is not using sparse I/O
   * or if it's not a multiport.
   */
  lf_sparse_io_record_t* sparse_record;

  /**
   * @brief Channel index for the destination port.
   *
   * Indicates which channel this port writes to in its destination
   * reactor. Set to -1 if there is no destination or if this is
   * not a multiport connection.
   */
  int destination_channel;

  /**
   * @brief Number of destination reactors.
   *
   * Indicates how many reactors this port writes to. For simple
   * connections, this will be 1. For multiport connections, this
   * may be greater than 1.
   */
  int num_destinations;

  /**
   * @brief Pointer to the source reactor.
   *
   * Points to the self struct of the reactor that provides data
   * to this port. For input ports, this typically points to the
   * container of the output port that sends data to this port.
   */
  self_base_t* source_reactor;
} lf_port_base_t;

//////////////////////////////////////////////////////////
//// Global variables

/**
 * @brief Counter used to issue a warning if memory is
 * allocated for tokens and never freed.
 * @ingroup Internal
 *
 * Note that every trigger will have one token allocated for it.
 * That token is not counted because it is not expected to be freed.
 */
extern int _lf_count_token_allocations;

//////////////////////////////////////////////////////////
//// Functions that users may call

/**
 * @brief Return a new disassociated token with type matching
 * the specified port or action and containing the specified
 * value and length.
 * @ingroup API
 *
 * The value is assumed to point to dynamically
 * allocated memory that will be automatically freed. The length is 1
 * unless the type of the port is an array, in which case the
 * value points to an array of the specified length.
 * The token must then be sent to the port using `lf_set_token`
 * or scheduled with the action using `lf_schedule_token`.
 * The token can also be safely sent to any other port or
 * scheduled with any other action that has the same type.
 * If it is not scheduled or sent, then it is up to the user
 * to free the memory allocated for the token and its value.
 * @param port_or_action A port or action.
 * @param val The value.
 * @param len The length, or 1 if it not an array.
 * @return A pointer to a lf_token_t struct.
 */
lf_token_t* lf_new_token(void* port_or_action, void* val, size_t len);

/**
 * @brief Return a writable copy of the token in the specified template.
 * @ingroup API
 *
 * If the reference count is 1, this returns the template's token
 * rather than a copy. The reference count will be 1.
 * Otherwise, if the size of the token payload is zero, this also
 * returns the original token, again with reference count of 1.
 * Otherwise, this returns a new token with a reference count of 1.
 * The new token is added to a list of tokens whose reference counts will
 * be decremented at the start of the next tag.
 * If the template has no token (it has a primitive type), then there
 * is no need for a writable copy. Return NULL.
 *
 * @param port An input port, cast to (lf_port_base_t*).
 * @return A pointer to a writable copy of the token, or NULL if the type is primitive.
 */
lf_token_t* lf_writable_copy(lf_port_base_t* port);

//////////////////////////////////////////////////////////
//// Functions not intended to be used by users

/**
 * @brief Free the specified token, if appropriate.
 * @ingroup Internal
 *
 * If the reference count is greater than 0, then do not free
 * anything. Otherwise, the token value (payload) will be freed,
 * if there is one. Then the token itself will be freed.
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
 * @brief Return a new token with the specified type, value, and length.
 * @ingroup Internal
 *
 * This will attempt to get one from the recyling bin, and, if the
 * recycling bin is empty, will allocate a new token using calloc
 * and set its type to point to the specified type. The returned token
 * will indicate that it is not a template token, and its reference count
 * will be 0.
 * @param type The type of the token.
 * @param value The value, or NULL to have no value.
 * @param length The array length of the value, 1 to not be an array,
 *  or 0 to have no value.
 * @return lf_token_t*
 */
lf_token_t* _lf_new_token(token_type_t* type, void* value, size_t length);

/**
 * @brief Get a token for the specified template.
 * @ingroup Internal
 *
 * If the template already has a token and the reference count is 1,
 * then return that token. Otherwise, create a new token,
 * make it the new template, and dissociate or free the
 * previous template token.
 *
 * @param tmplt The template. // template is a C++ keyword.
 * @return A new or recycled lf_token_t struct.
 */
lf_token_t* _lf_get_token(token_template_t* tmplt);

/**
 * @brief Initialize the specified template to contain a token that is an
 * @ingroup Internal
 *
 * array with the specified element size. If the template already has
 * a token with a reference count greater than 1 or a non-matching type,
 * it will be replaced and that token will be freed. The length of the
 * returned token will be 0, its value will be NULL, and its reference count
 * will be 1.
 *
 * @param tmplt The template. // template is a C++ keyword.
 * @param element_size The element size.
 */
void _lf_initialize_template(token_template_t* tmplt, size_t element_size);

/**
 * @brief Return a token storing the specified value, which is assumed to
 * be either a scalar (if length is 1) or an array of the specified length.
 * @ingroup Internal
 *
 * If the token in the specified template is available (it non-null and its
 * reference count is 1), then return it. Otherwise, create a new token
 * and replace the template token with the new one, freeing the
 * previous token from its template association.
 * The element_size for elements of the array is specified by
 * the specified template.
 *
 * @param tmplt A template for the token. // template is a C++ keyword.
 * @param value The value of the array.
 * @param length The length of the array, or 1 if it is not an array.
 * @return Either the specified token or a new one, in each case with a value
 *  field pointing to newly allocated memory.
 */
lf_token_t* _lf_initialize_token_with_value(token_template_t* tmplt, void* value, size_t length);

/**
 * @brief Return a token for storing an array of the specified length
 * with new memory allocated (using calloc, so initialize to zero) for storing that array.
 * @ingroup Internal
 *
 * If the template's token is available (it is non-null and its reference count is 1),
 * then reuse it.
 * Otherwise, create a new token and replace the template token
 * with the new one, freeing the previous token from its template
 * association. The element_size for elements
 * of the array is specified by the specified template. The caller
 * should populate the value and ref_count field of the returned
 * token after this returns.
 *
 * @param tmplt The token template (must not be NULL). // template is a C++ keyword.
 * @param length The length of the array, or 1 if it is not an array.
 * @return Either the template's token or a new one, in each case with a value
 *  field pointing to newly allocated memory.
 */
lf_token_t* _lf_initialize_token(token_template_t* tmplt, size_t length);

/**
 * @brief Free all tokens.
 * @ingroup Internal
 *
 * Free tokens on the _lf_token_recycling_bin hashset and all template tokens.
 */
void _lf_free_all_tokens();

/**
 * @brief Replace the token in the specified template, if there is one,
 * with a new one.
 * @ingroup Internal
 *
 * If the new token is the same as the token in the template,
 * then this does nothing. Otherwise, it frees the previous template token.
 *
 * @param tmplt Pointer to a template. // template is a C++ keyword.
 * @param newtoken The replacement token.
 */
void _lf_replace_template_token(token_template_t* tmplt, lf_token_t* newtoken);

/**
 * @brief Decrement the reference count of the specified token.
 * @ingroup Internal
 *
 * If the reference count hits 0, free the memory for the value
 * carried by the token, and, if the token is not also the template
 * token of its trigger, free the token.
 *
 * @param token Pointer to a token.
 * @return NOT_FREED if nothing was freed, VALUE_FREED if the value
 *  was freed, TOKEN_FREED if only the token was freed, and
 *  TOKEN_AND_VALUE_FREED if both the value and the token were freed.
 */
token_freed _lf_done_using(lf_token_t* token);

/**
 * @brief Free token copies made for mutable inputs.
 * @ingroup Internal
 *
 * This function should be called at the beginning of each time step to avoid memory leaks.
 */
void _lf_free_token_copies(void);

#endif /* LF_TOKEN_H */
