#ifndef OPENFMOD_STUDIO_PARAMETER_MODEL_HPP
#define OPENFMOD_STUDIO_PARAMETER_MODEL_HPP
#include "OpenFmod/Studio/Modulation/Property.hpp"
#include <string>
namespace OpenFmod::Studio {
struct Parameter {
    Bank::Guid Id{};
    std::uint32_t Flags{};
    std::uint32_t Type{};
    std::string Name;
    float Minimum{};
    float Maximum{};
    float Default{};
    float AuxiliaryValue{};
    float SeekSpeed{};
    float DescendingSeekSpeed{};
    std::vector<std::string> Labels;
    std::map<std::uint32_t, Property> Properties;
};
struct ParameterLayout {
    Bank::Guid Id{};
    Bank::Guid ParameterId{};
    std::vector<Bank::Guid> Instruments;
    std::uint32_t Flags{};
    std::vector<Bank::Guid> Controllers;
};
Parameter ReadParameter(std::string_view bytes, const Bank::Chunk& chunk, std::uint32_t version);
ParameterLayout ReadParameterLayout(std::string_view bytes, const Bank::Chunk& chunk);
}
#endif
