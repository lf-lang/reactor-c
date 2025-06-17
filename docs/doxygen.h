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
 * @defgroup Modal Modal
 * @brief API used internally to support modal reactors.
 *
 * These functions and types are not meant to be used directly by users, but are useful for developers.
 * @note Only used in modal reactor execution.
 */

/**
 * @defgroup Federated Federated
 * @brief Functions for federated execution.
 *
 * This group contains functions for federated execution.
 * The message types and protocols are defined in @ref net_common.h.
 */

/**
 * @defgroup Tracing Tracing
 * @brief Functions for tracing.
 *
 * This group contains functions for tracing.
 * Tracing is described in the [Lingua Franca handbook](https://www.lf-lang.org/docs/next/reference/tracing).
 */

/**
 * @defgroup RTI RTI
 * @brief Functions for the runtime infrastructure for federated execution.
 *
 * This group contains functions for the runtime infrastructure for federated execution.
 * The message types and protocols are defined in @ref net_common.h.
 */
