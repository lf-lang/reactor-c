/**
 * @file
 * @author Hou Seng Wong
 * @copyright (c) 2020-2024, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Undefine macros defined in api/set.h.
 */


#ifdef CTARGET_SET
#undef CTARGET_SET

//////////////////////////////////////////////////////////////
/////////////  SET Functions (to produce an output)

// NOTE: According to the "Swallowing the Semicolon" section on this page:
//    https://gcc.gnu.org/onlinedocs/gcc-3.0.1/cpp_3.html
// the following macros should use an odd do-while construct to avoid
// problems with if ... else statements that do not use braces around the
// two branches.

/**
 * Set the specified output (or input of a contained reactor)
 * to the specified value.
 */
#undef lf_set
#undef SET

/**
 * Version of lf_set for output types given as 'type[]' where you
 * want to send a previously dynamically allocated array.
 */
#undef SET_ARRAY

/**
 * Version of lf_set() for output types given as 'type*' that
 * allocates a new object of the type of the specified output port.
 */
#undef SET_NEW

/**
 * Version of lf_set() for output types given as 'type[]'.
 */
#undef SET_NEW_ARRAY

/**
 * Version of lf_set() for output types given as 'type[number]'.
 */
#undef SET_PRESENT

/**
 * Version of lf_set() for output types given as 'type*' or 'type[]' where you want
 * to forward an input or action without copying it.
 */
#undef lf_set_token
#undef SET_TOKEN

/**
 * Set the destructor used to free "token->value" set on "out".
 * That memory will be automatically freed once all downstream
 * reactions no longer need the value.
 */
#undef lf_set_destructor


/**
 * Set the destructor used to copy construct "token->value" received
 * by "in" if "in" is mutable.
 */
#undef lf_set_copy_constructor

//////////////////////////////////////////////////////////////
/////////////  SET_MODE Function (to switch a mode)

/**
 * Sets the next mode of a modal reactor. Same as SET for outputs, only
 * the last value will have effect if invoked multiple times.
 * Works only in reactions with the target mode declared as effect.
 */
#ifdef MODAL_REACTORS
#undef lf_set_mode
#undef SET_MODE
#endif

#undef lf_time_logical 
#undef lf_time_logical_elapsed 
#undef get_logical_time
#undef get_elapsed_logical_time
#undef lf_request_stop
#undef lf_tag
#undef get_current_tag
#undef get_microstep
#endif // CTARGET_SET
