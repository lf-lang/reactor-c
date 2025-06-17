/**
 * @file type_converter.h
 * @author Muhammad Khubaib Umer
 *
 * @brief This file provides macro `DO_CONVERT(fromType, toType, value)`.
 * @ingroup Utilities
 *
 * Sometimes the generic Reactor can work as a connector between two reactors.
 * We provide this macro to enable user to provide their own converter libraries
 * as long as they follow the convention for naming their conversion functions this macro will work.
 *
 * Convention: `toType convert__fromType_to__toType(fromType x)`.
 */

#ifndef TYPE_CONVERTER_H_
#define TYPE_CONVERTER_H_

#define PASTE(x, y) x##y

#define RESOLVE(i, o, in) PASTE(convert__##i, _to__##o)(in)

/**
 * @brief Convert the specified value from one type to another.
 * @ingroup Utilities
 *
 * This macro enables user to provide their own converter libraries as long as they
 * follow the convention for naming their conversion functions.
 * @note Converter library functions must follow this convention: `toType convert__fromType_to__toType(fromType x)`.
 *
 * @param fromType Typename of `value` field
 * @param toType Typename of desired type
 * @param value Actual value of type `fromType`
 */
#define DO_CONVERT(fromType, toType, value) RESOLVE(fromType, toType, value)

#endif // TYPE_CONVERTER_H_
