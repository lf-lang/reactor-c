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
 * This file provides macro `DO_CONVERT(fromType, toType, value)`
 * Sometimes the generic Reactor can work as a connector between two reactors
 * We provide this macro to enable user to provide their own converter libraries
 * as long as they follow the convention for naming their conversion functions this macro will work
 *
 * Convention: toType convert__fromType_to__toType(fromType x)
 */

#ifndef TYPE_CONVERTER_H_
#define TYPE_CONVERTER_H_

#define PASTE(x,y) x ## y

#define RESOLVE(i, o, in)  PASTE(convert__##i, _to__##o)(in)

/// @name DO_CONVERT
/// @param fromType Typename of <code> value </code> field
/// @param toType Typename of desired type
/// @param value Actual value of type <code> fromType </code>
/// @brief  This macro to enable user to provide their own converter libraries
///         as long as they follow the convention for naming their conversion functions this macro will work
/// @attention Converter library functions must follow this convention
///         <br> <code> toType convert__fromType_to__toType(fromType x) </code>
#define DO_CONVERT(fromType, toType, value) RESOLVE(fromType, toType, value)

#endif // TYPE_CONVERTER_H_
