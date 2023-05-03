/**
 * @file
 * @author Muhammad Khubaib Umer (khubaib@magnition.io)
 *
 * @section LICENSE
Copyright (c) 2023, MagnitionIO

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * @section DESCRIPTION
 *
 * This file provides macros for Generic Reactors in C-Target
 * The macros are wrappers on compiler builtin and provide the
 * programmer to auto-infer types and conditionals based on types
 */

#ifndef GENERICS_H
#define GENERICS_H

/// If buitin are not available on target toolchain we may not be able to support generics
#if defined __has_builtin
/// Auto-Deduce variable type based on assigned value
#define auto_t __auto_type

/// Checks if types of both `a` and `b` are same
#define is_same_type(a, b) __builtin_types_compatible_p(__typeof__(a), __typeof__(b))

/// Checks if type of `b` is same as the specified `typename`
#define is_same(typename, b) __builtin_types_compatible_p(typename, __typeof__(b))

/// Checks if `typename_a` and `typename_b` are same
#define is_type_equal(typename_a, typename_b) __builtin_types_compatible_p(typename_a, typename_b)

/// Checks if the passed variable `p` is array or a pointer
#define is_pointer_or_array(p)  (__builtin_classify_type(p) == 5)

#define decay(p)  (&*__builtin_choose_expr(is_pointer_or_array(p), p, NULL))

/// Checks if passed variable `p` is a pointer
#define is_pointer(p)  is_same_type(p, decay(p))

/// Returns the pointer for specified `p`
#define get_pointer(p) __builtin_choose_expr(is_pointer(p), p, &p)

/// Checks types for both `left` and `right` and returns appropriate value based on `left` type
#define to_left_type(left, right) __builtin_choose_expr(is_pointer_or_array(left), __builtin_choose_expr(is_pointer_or_array(right), (right), &(right)), __builtin_choose_expr(is_pointer_or_array(right), *(right), (right)))

#else // buitin are not available

#define auto_t
#define is_same_type(a, b)
#define is_same(typename, b)
#define is_pointer_or_array(p)
#define decay(p)
#define is_pointer(p)
#define get_pointer(p)
#define to_left_type(left, right)

#endif // __has_builtin

#endif // GENERICS_H

