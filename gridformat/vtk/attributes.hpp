// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief Helper functions to get the VTK-specific names of things.
 */
#ifndef GRIDFORMAT_VTK_ATTRIBUTES_HPP_
#define GRIDFORMAT_VTK_ATTRIBUTES_HPP_

#include <bit>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/callable_overload_set.hpp>
#include <gridformat/vtk/common.hpp>

// forward declarations
namespace GridFormat { class DynamicPrecision; }
namespace GridFormat::Compression { class LZMA; class ZLIB; class LZ4; }
namespace GridFormat::Encoding { struct Ascii; struct Base64; struct RawBinary; }
// end forward declarations

namespace GridFormat::VTK {

//! \addtogroup VTK
//! \{

std::string attribute_name(const DynamicPrecision& prec) {
    std::string prefix = prec.is_integral() ? (prec.is_signed() ? "Int" : "UInt") : "Float";
    return prefix + std::to_string(prec.size_in_bytes()*8);
}

std::string attribute_name(std::endian e) {
    return e == std::endian::little ? "LittleEndian" : "BigEndian";
}

std::string attribute_name(const Encoding::Ascii&) { return "ascii"; }
std::string attribute_name(const Encoding::Base64&) { return "base64"; }
std::string attribute_name(const Encoding::RawBinary&) { return "raw"; }

std::string attribute_name(const Compression::LZMA&) { return "vtkLZMADataCompressor"; };
std::string attribute_name(const Compression::ZLIB&) { return "vtkZLibDataCompressor"; };
std::string attribute_name(const Compression::LZ4&) { return "vtkLZ4DataCompressor"; };

std::string data_format_name(const Encoding::RawBinary&, const DataFormat::Appended&) { return "appended"; }
std::string data_format_name(const Encoding::Base64&, const DataFormat::Appended&) { return "appended"; }
std::string data_format_name(const Encoding::Base64&, const DataFormat::Inlined&) { return "binary"; }
std::string data_format_name(const Encoding::Ascii&, const DataFormat::Inlined&) { return "ascii"; }

template<typename Enc, typename Format>
std::string data_format_name(const Enc& e, const Format& format) {
    const std::string encoder_name = attribute_name(e);
    const std::string format_name = Overload{
        [] (const DataFormat::Appended&) { return "appended"; },
        [] (const DataFormat::Inlined&) { return "inlined"; }
    } (format);
    const std::string other_format_name = Overload{
        [] (const DataFormat::Appended&) { return "GridFormat::VTK::inlined"; },
        [] (const DataFormat::Inlined&) { return "GridFormat::VTK::appended"; }
    } (format);
    throw ValueError(
        "VTK's '" + format_name + "' data format cannot be used with "  + encoder_name
        + " encoding. Please choose '" + other_format_name + "' or a different encoder."
    );
}

//! \} group VTK

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_ATTRIBUTES_HPP_