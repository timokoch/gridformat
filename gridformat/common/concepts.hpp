// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief Basic concept definitions
 */
#ifndef GRIDFORMAT_COMMON_CONCEPTS_HPP_
#define GRIDFORMAT_COMMON_CONCEPTS_HPP_

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <array>
#include <span>

#include <gridformat/common/type_traits.hpp>

namespace GridFormat::Concepts {

//! \addtogroup Common
//! \{

template<typename T>
concept StaticallySizedRange
    = std::ranges::range<T> and
    is_complete<GridFormat::StaticSize<T>> and
    requires {{ GridFormat::StaticSize<T>::value } -> std::convertible_to<std::size_t>; };

template<typename T1, typename T2>
concept Interoperable = std::is_convertible_v<T1, T2> || std::is_convertible_v<T2, T1>;

template<typename T, typename Stream>
concept Streamable = requires(const T& t, Stream& s) {
    { s << t } -> std::same_as<Stream&>;
};

template<typename T, typename Data>
concept OutputStream = requires(T& t, std::span<std::add_const_t<Data>>& data) {
    { t.write(data) };
};

template<typename T, typename ValueType>
concept RangeOf = std::ranges::range<T> and std::convertible_to<std::ranges::range_value_t<T>, ValueType>;

template<typename T, std::size_t dim>
concept MDRange = std::ranges::range<T> and mdrange_dimension<T> == dim;

template<typename T>
concept Scalar = is_scalar<T>;

//! \} group Common

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_CONCEPTS_HPP_
