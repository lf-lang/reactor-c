// Doxygen group definitions

/**
 * @defgroup API Reaction API
 * @brief API for use in inline reaction bodies.
 *
 * These macros and functions are automatically available in any reaction body that is inlined
 * inside the `.lf` file; there is no need to add any `#include`.
 *
 * For reactions that are defined in ordinary C files
 * (see [Reaction Declarations](http://lf-lang.org/docs/next/writing-reactors/reaction-declarations)),
 * for those functions that take an environment argument, the self struct struct has an
 * `environment` field, so you can pass as the argument `self->environment`.
 */

/**
 * @defgroup Constants Constants
 * @brief Constants provided for convenience and readability.
 */

/**
 * @defgroup Types Types
 * @brief Types and structs.
 */

/**
 * @defgroup Platform Platform API
 * @brief API for functions that have platform-specific implementations.
 */

/**
 * @defgroup Advanced Advanced
 * @brief API mainly used internally, but occasionally useful for users.
 */

/**
 * @defgroup AdvTypes Advanced Types
 * @brief Structs and types used internally.
 */
