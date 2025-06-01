// Doxygen group definitions

/**
 * @defgroup API API for Reactions
 * @brief API for use in inline reaction bodies.
 *
 * These macros and functions are automatically available in any reaction body that is inlined
 * inside the `.lf` file; there is no need to add any `#include`.
 *
 * \note
 *  For reactions that are defined in ordinary C files instead of inline in the `.lf` file,
 *  (see [Reaction Declarations](http://lf-lang.org/docs/next/writing-reactors/reaction-declarations)),
 *  you may need to use a function directly instead of a macro.
 *  The corresponding very likely takes an `environment` argument.
 *  The self struct struct has an `environment` field, so you can pass as the argument `self->environment`. 
 */

/**
 * @defgroup Types Basic Types
 * @brief Types and structs.
 * 
 * These types and structs are automatically available in any reaction body that is inlined
 * inside the `.lf` file; there is no need to add any `#include`.
 */

/**
 * @defgroup Constants Constants
 * @brief Constants provided for convenience and readability.
 * 
 * These macros are provided for convenience and readability.
 */

/**
 * @defgroup Utilities Utilities
 * @brief Useful functions for application developers.
 * 
 * This collection of functions provides commonly used functionality that is not part of the core API.
 */

/**
 * @defgroup Platform Platform API
 * @brief API for functions that have platform-specific implementations.
 * 
 * These functions are not meant to be used directly by users, but are useful for developers.
 * They are used to implement the platform-independent API for specific platforms.
 */

/**
 * @defgroup Internal Internal
 * @brief API mainly used internally, but occasionally useful for users.
 * 
 * These functions and types are not meant to be used directly by users, but are useful for developers.
 */

/**
 * @defgroup IntTypes Internal Types
 * @brief Structs and types used internally.
 * 
 * These structs and types are not meant to be used directly by users, but are useful for developers.
 */
