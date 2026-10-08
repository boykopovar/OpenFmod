#ifndef OPENFMOD_STUDIO_MODULATION_PROPERTY_HPP
#define OPENFMOD_STUDIO_MODULATION_PROPERTY_HPP
#include "OpenFmod/Bank/Identity.hpp"
namespace OpenFmod::Studio {
struct Property {
    std::uint32_t Id{};
    std::uint16_t Type{};
    std::uint16_t Flags{};
    Bank::Guid Source{};
    std::vector<Bank::Guid> Curves;
    std::vector<Bank::Guid> Modulators;
};
Property ReadProperty(std::string_view bytes);
}
#endif
