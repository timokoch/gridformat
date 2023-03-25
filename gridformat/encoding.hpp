// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup API
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_ENCODING_HPP_
#define GRIDFORMAT_ENCODING_HPP_

#include <gridformat/common/type_traits.hpp>

#include <gridformat/encoding/ascii.hpp>
#include <gridformat/encoding/base64.hpp>
#include <gridformat/encoding/raw.hpp>

namespace GridFormat {

using Encoder = UniqueVariant<
    Encoding::Ascii,
    Encoding::Base64,
    Encoding::RawBinary
>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_ENCODING_HPP_
