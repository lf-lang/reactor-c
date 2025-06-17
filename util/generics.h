/**
 * @file generics.h
 * @author Muhammad Khubaib Umer
 *
 * @brief This file provides macros for Generic Reactors in C-Target.
 * @ingroup Utilities
 *
 * The macros are wrappers on compiler builtin and provide the
 * programmer to auto-infer types and conditionals based on types
 */

#ifndef GENERICS_H
#define GENERICS_H

// If buitin are not available on target toolchain we may not be able to support generics
#if defined __has_builtin
// Auto-Deduce variable type based on assigned value
#define var __auto_type

/**
 * @brief Check whether the types of both `a` and `b` are same.
 * @ingroup Utilities
 *
 * @param a The first value to compare.
 * @param b The second value to compare.
 * @return True if the types are the same, false otherwise.
 */
#define lf_is_same_type(a, b) __builtin_types_compatible_p(__typeof__(a), __typeof__(b))

/**
 * @brief Check whether the type of `b` is same as the specified `typename`.
 * @ingroup Utilities
 *
 * @param typename The type to compare against.
 * @param b The value to compare.
 * @return True if the types are the same, false otherwise.
 */
#define lf_is_same(typename, b) __builtin_types_compatible_p(typename, __typeof__(b))

/**
 * @brief Check whether the type of `typename_a` is same as the type of `typename_b`.
 * @ingroup Utilities
 *
 * @param typename_a The first type to compare against.
 * @param typename_b The second type to compare against.
 * @return True if the types are the same, false otherwise.
 */
#define lf_is_type_equal(typename_a, typename_b) __builtin_types_compatible_p(typename_a, typename_b)

/**
 * @brief Check whether the passed variable `p` is an array or a pointer.
 * @ingroup Utilities
 *
 * @param p The variable to check.
 * @return True if the variable is an array or a pointer, false otherwise.
 */
#define lf_is_pointer_or_array(p) (__builtin_classify_type(p) == 5)

/**
 * @brief Decay the specified pointer.
 * @ingroup Utilities
 *
 * @param p The pointer to decay.
 * @return The decayed pointer.
 */
#define lf_decay(p) (&*__builtin_choose_expr(lf_is_pointer_or_array(p), p, NULL))

/**
 * @brief Check whether the passed variable `p` is a pointer.
 * @ingroup Utilities
 *
 * @param p The variable to check.
 * @return True if the variable is a pointer, false otherwise.
 */
#define lf_is_pointer(p) lf_is_same_type(p, lf_decay(p))

/**
 * @brief Return the pointer for specified `p`.
 * @ingroup Utilities
 *
 * @param p The variable to get the pointer for.
 * @return The pointer for the specified variable.
 */
#define lf_get_pointer(p) __builtin_choose_expr(lf_is_pointer(p), p, &p)

/**
 * @brief Check types for both `left` and `right` and return appropriate value based on `left` type.
 * @ingroup Utilities
 *
 * @param left The left value to check.
 * @param right The right value to check.
 * @return The appropriate value based on `left` type.
 */
#define lf_to_left_type(left, right)                                                                                   \
  __builtin_choose_expr(lf_is_pointer_or_array(left),                                                                  \
                        __builtin_choose_expr(lf_is_pointer_or_array(right), (right), &(right)),                       \
                        __builtin_choose_expr(lf_is_pointer_or_array(right), *(right), (right)))

#else // buitin are not available

#define var
#define lf_is_same_type(a, b)
#define lf_is_same(typename, b)
#define lf_is_pointer_or_array(p)
#define lf_decay(p)
#define lf_is_pointer(p)
#define lf_get_pointer(p)
#define lf_to_left_type(left, right)

#endif // __has_builtin

#endif // GENERICS_H
