/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
 * @copyright (c) 2020-2024, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Macros providing an API for use in inline reaction bodies.
 * 
 * This set of macros is defined prior to each reaction body and undefined after the reaction body
 * using the set_undef.h header file.
 * 
 * Note for target language developers. This is one way of developing a target language where 
 * the C core runtime is adopted. This file is a translation layer that implements Lingua Franca 
 * APIs which interact with the internal APIs.
 */

#ifndef CTARGET_SET
#define CTARGET_SET

// NOTE: According to the "Swallowing the Semicolon" section on this page:
//    https://gcc.gnu.org/onlinedocs/gcc-3.0.1/cpp_3.html
// the following macros should use an odd do-while construct to avoid
// problems with if ... else statements that do not use braces around the
// two branches.

/**
 * @brief Set the specified output (or input of a contained reactor) to the specified value.
 *
 * If the value argument is a primitive type such as int,
 * double, etc. as well as the built-in types bool and string, 
 * the value is copied and therefore the variable carrying the
 * value can be subsequently modified without changing the output.
 * This also applies to structs with a type defined by a typedef
 * so that the type designating string does not end in '*'.
 * 
 * If the value argument is a pointer
 * to memory that the calling reaction has dynamically allocated,
 * the memory will be automatically freed once all downstream
 * reactions no longer need the value.
 * If 'lf_set_destructor' is called on 'out', then that destructor
 * will be used to free 'value'. 
 * Otherwise, the default void free(void*) function is used.
 * 
 * @param out The output port (by name) or input of a contained
 *  reactor in form input_name.port_name.
 * @param value The value to insert into the self struct.
 */
#define lf_set(out, val) \
do { \
    out->value = val; \
   _lf_set_present((lf_port_base_t*)out); \
    if (((token_template_t*)out)->token != NULL) { \
        /* The cast "*((void**) &out->value)" is a hack to make the code */ \
        /* compile with non-token types where value is not a pointer. */ \
        lf_token_t* token = _lf_initialize_token_with_value((token_template_t*)out, *((void**) &out->value), 1); \
    } \
} while(0)

/**
 * Version of lf_set for output types given as `type[]` or `type*` where you
 * want to send a previously dynamically allocated array.
 *
 * The deallocation is delegated to downstream reactors, which
 * automatically deallocate when the reference count drops to zero.
 * It also sets the corresponding _is_present variable in the self
 * struct to true (which causes the object message to be sent).
 * @param out The output port (by name).
 * @param val The array to send (a pointer to the first element).
 * @param length The length of the array to send.
 * @see lf_token_t
 */
#define SET_ARRAY(out, val, elem_size, length) \
do { \
        _Pragma ("Warning \"'SET_ARRAY' is deprecated.\""); \
        lf_set_array(out, val, length); \
} while (0)

/**
 * Version of lf_set() for output types given as 'type*' that
 * allocates a new object of the type of the specified output port.
 *
 * This macro dynamically allocates enough memory to contain one
 * instance of the output datatype and sets the variable named
 * by the argument to point to the newly allocated memory.
 * The user code can then populate it with whatever value it
 * wishes to send.
 *
 * This macro also sets the corresponding _is_present variable in the self
 * struct to true (which causes the object message to be sent),
 * @param out The output port (by name).
 */
#define SET_NEW(out) \
do { \
        _Pragma ("Warning \"'SET_NEW' is deprecated.\""); \
        _LF_SET_NEW(out); \
} while (0)

/**
 * Version of lf_set() for output types given as 'type[]'.
 *
 * This allocates a new array of the specified length,
 * sets the corresponding _is_present variable in the self struct to true
 * (which causes the array message to be sent), and sets the variable
 * given by the first argument to point to the new array so that the
 * user code can populate the array. The freeing of the dynamically
 * allocated array will be handled automatically
 * when the last downstream reader of the message has finished.
 * @param out The output port (by name).
 * @param len The length of the array to be sent.
 */
#define SET_NEW_ARRAY(out, len) \
do { \
        _Pragma ("Warning \"'SET_NEW_ARRAY' is deprecated.\""); \
        _LF_SET_NEW_ARRAY(out, len); \
} while (0)

/**
 * Version of lf_set() for output types given as 'type[number]'.
 *
 * This sets the _is_present variable corresponding to the specified output
 * to true (which causes the array message to be sent). The values in the
 * output are normally written directly to the array or struct before or
 * after this is called.
 * @param out The output port (by name).
 */
#define SET_PRESENT(out) \
do { \
	_Pragma ("Warning \"'SET_PRESENT' is deprecated.\""); \
        _lf_set_present((lf_port_base_t*)out); \
} while (0)

/**
 * Version of lf_set() for output types given as 'type*' or 'type[]' where you want
 * to forward an input or action without copying it.
 *
 * The deallocation of memory is delegated to downstream reactors, which
 * automatically deallocate when the reference count drops to zero.
 * @param out The output port (by name).
 * @param token A pointer to token obtained from an input or action.
 */
#define lf_set_token(out, newtoken) _LF_SET_TOKEN(out, newtoken)
#define SET_TOKEN(out, newtoken) \
do { \
        _Pragma ("Warning \"'SET_TOKEN' is deprecated. Use 'lf_set_token' instead.\""); \
        _LF_SET_TOKEN(out, newtoken); \
} while (0)

/**
 * Set the destructor used to free "token->value" set on "out".
 * That memory will be automatically freed once all downstream
 * reactions no longer need the value.
 * 
 * @param out The output port (by name) or input of a contained
 *            reactor in form input_name.port_name.
 * @param dtor A pointer to a void function that takes a pointer argument
 *             or NULL to use the default void free(void*) function. 
 */
#define lf_set_destructor(out, dtor) _LF_SET_DESTRUCTOR(out, dtor)

/**
 * Set the destructor used to copy construct "token->value" received
 * by "in" if "in" is mutable.
 * 
 * @param out The output port (by name) or input of a contained
 *            reactor in form input_name.port_name.
 * @param cpy_ctor A pointer to a void* function that takes a pointer argument
 *                 or NULL to use the memcpy operator.
 */
#define lf_set_copy_constructor(out, cpy_ctor) _LF_SET_COPY_CONSTRUCTOR(out, cpy_ctor)

//////////////////////////////////////////////////////////////
/////////////  SET_MODE Function (to switch a mode)

/**
 * Sets the next mode of a modal reactor. Same as SET for outputs, only
 * the last value will have effect if invoked multiple times.
 * Works only in reactions with the target mode declared as effect.
 *
 * @param mode The target mode to set for activation.
 */
#ifdef MODAL_REACTORS
#define lf_set_mode(mode) _LF_SET_MODE(mode)
#define SET_MODE(mode) \
do { \
        _Pragma ("Warning \"'SET_MODE' is deprecated. Use 'lf_set_mode' instead.\""); \
        _LF_SET_MODE(mode); \
} while (0)
#endif // MODAL_REACTORS

#endif // CTARGET_SET

// For simplicity and backward compatability, dont require the environment-pointer when calling the timing API.
// As long as this is done from the context of a reaction, `self` is in scope and is a pointer to the self-struct
// of the current reactor. 
#define lf_tag() lf_tag(self->base.environment)
#define get_current_tag() get_current_tag(self->base.environment)
#define get_microstep() get_microstep(self->base.environment)
#define lf_time_logical() lf_time_logical(self->base.environment)
#define lf_time_logical_elapsed() lf_time_logical_elapsed(self->base.environment)
#define get_elapsed_logical_time() get_elapsed_logical_time(self->base.environment)
#define get_logical_time() get_logical_time(self->base.environment)
