#ifndef OPENFMOD_BANK_IDENTITY_HPP
#define OPENFMOD_BANK_IDENTITY_HPP
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
#include <array>
namespace OpenFmod::Bank {
using Guid = std::array<std::uint8_t, 16>;
inline Guid ReadGuid(Binary::Reader& reader) {
    Guid result{};
    const auto bytes = reader.Read(result.size());
    std::copy(bytes.begin(), bytes.end(), result.begin());
    return result;
}
}
#endif
