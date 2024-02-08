/**
 * @file
 * @author Hou Seng Wong
 * @copyright (c) 2020-2024, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Undefine macros defined in api/set.h.
 * 
 * This file is included at the end of each reaction body to undefine the macros used in reaction bodies.
 */


#ifdef CTARGET_SET
#undef CTARGET_SET

#undef lf_set
#undef lf_set_token
#undef lf_set_destructor
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
