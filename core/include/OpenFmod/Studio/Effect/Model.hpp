#ifndef OPENFMOD_STUDIO_EFFECT_MODEL_HPP
#define OPENFMOD_STUDIO_EFFECT_MODEL_HPP
#include "OpenFmod/Studio/Modulation/Property.hpp"
#include <variant>
namespace OpenFmod::Studio {
using EffectParameter = std::variant<float, std::int32_t, bool, std::string>;
struct Effect {
    Bank::Guid Id{};
    std::uint32_t Type{};
    bool LegacyBypass{};
    bool ParameterFlag{};
    std::vector<EffectParameter> Parameters;
    std::map<std::uint32_t, Property> Properties;
};
struct Sidechain {
    Bank::Guid Id{};
    bool LegacyFlag{};
    std::vector<Bank::Guid> Effects;
    std::vector<Bank::Guid> Followers;
    float GainDecibels{};
    std::map<std::uint32_t, Property> Properties;
};
Sidechain ReadSidechain(std::string_view bytes, const Bank::Chunk& chunk);
Effect ReadEffect(std::string_view bytes, const Bank::Chunk& chunk);
}
#endif
