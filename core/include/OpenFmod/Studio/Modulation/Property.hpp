#ifndef OPENFMOD_STUDIO_MODULATION_PROPERTY_HPP
#define OPENFMOD_STUDIO_MODULATION_PROPERTY_HPP
#include "OpenFmod/Bank/Identity.hpp"
#include "OpenFmod/Bank/Container.hpp"
#include <map>
namespace OpenFmod::Studio {
struct Property {
    std::uint32_t Id{};
    std::uint16_t Resolution{};
    std::uint16_t ValueType{};
    Bank::Guid MappingId{};
    std::vector<Bank::Guid> Curves;
    std::vector<Bank::Guid> Modulators;
};
Property ReadProperty(std::string_view bytes);
std::map<std::uint32_t, Property> ReadProperties(std::string_view bytes, const Bank::Chunk& chunk);
}
#endif
