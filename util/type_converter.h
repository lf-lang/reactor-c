/**
 * @file
 * @author Muhammad Khubaib Umer
 *
 * @brief This file provides macro `DO_CONVERT(fromType, toType, value)`
 *
 * Sometimes the generic Reactor can work as a connector between two reactors
 * We provide this macro to enable user to provide their own converter libraries
 * as long as they follow the convention for naming their conversion functions this macro will work
 *
 * Convention: toType convert__fromType_to__toType(fromType x)
 */

#ifndef TYPE_CONVERTER_H_
#define TYPE_CONVERTER_H_

#define PASTE(x, y) x##y

#define RESOLVE(i, o, in) PASTE(convert__##i, _to__##o)(in)

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
