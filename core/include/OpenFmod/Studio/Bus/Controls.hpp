#ifndef OPENFMOD_STUDIO_BUS_CONTROLS_HPP
#define OPENFMOD_STUDIO_BUS_CONTROLS_HPP
#include "OpenFmod/Studio/Modulation/Property.hpp"
namespace OpenFmod::Studio {
struct MixControls {
    float VolumeDecibels{};
    float PitchSemitones{};
    std::vector<Bank::Guid> VolumeControls;
};
struct VolumeControl {
    Bank::Guid Id{};
    std::vector<Bank::Guid> Targets;
    MixControls Levels;
    std::map<std::uint32_t, Property> Properties;
};
MixControls ReadMixControls(Binary::Reader& reader);
VolumeControl ReadVolumeControl(std::string_view bytes, const Bank::Chunk& chunk);
}
#endif
