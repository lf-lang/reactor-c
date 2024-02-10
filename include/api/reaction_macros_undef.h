/**
 * @file
 * @author Hou Seng Wong
 * @copyright (c) 2020-2024, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Undefine macros defined in api/reaction_macros.h.
 * 
 * This file is included at the end of each reaction body to undefine the macros used in reaction bodies.
 */


#ifdef REACTION_MACROS_UNDEF_H
#undef REACTION_MACROS_UNDEF_H

#undef lf_set
#undef lf_set_token
#undef lf_set_destructor
#undef lf_set_copy_constructor

#ifdef MODAL_REACTORS
#undef lf_set_mode
#endif

#undef lf_tag
#undef lf_time_logical 
#undef lf_time_logical_elapsed 
#endif // REACTION_MACROS_UNDEF_H
