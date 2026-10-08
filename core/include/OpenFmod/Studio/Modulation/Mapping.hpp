#ifndef OPENFMOD_STUDIO_MODULATION_MAPPING_HPP
#define OPENFMOD_STUDIO_MODULATION_MAPPING_HPP
#include "OpenFmod/Bank/Identity.hpp"
namespace OpenFmod::Studio {
struct MappingPoint {
    float Input{};
    float Output{};
};
class Mapping {
public:
    Mapping(Bank::Guid id, std::vector<MappingPoint> points);
    const Bank::Guid& GetId() const;
    float Transform(float value) const;
    float Inverse(float value) const;
private:
    float evaluate(float value, bool inverse) const;
    Bank::Guid id;
    std::vector<MappingPoint> points;
};
Mapping ReadMapping(std::string_view bytes);
}
#endif
