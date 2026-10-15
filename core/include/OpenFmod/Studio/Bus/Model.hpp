#ifndef OPENFMOD_STUDIO_BUS_MODEL_HPP
#define OPENFMOD_STUDIO_BUS_MODEL_HPP
#include "OpenFmod/Studio/Bus/Routing.hpp"
#include "OpenFmod/Studio/Modulation/Property.hpp"
namespace OpenFmod::Studio {
enum class BusKind { Input, Group, Master, Return };
struct BusEffect {
    Bank::Guid Id{};
    std::uint32_t ChannelLayout{};
};
struct Bus {
    Bank::Guid Id{};
    BusKind Kind{};
    Studio::Routing Routing;
    std::uint32_t Flags{};
    std::uint32_t ChannelLayout{};
    std::vector<BusEffect> PreEffects;
    std::vector<BusEffect> PostEffects;
    float VolumeDecibels{};
    float PitchSemitones{};
    std::vector<Bank::Guid> VolumeControls;
    std::int32_t MaximumInstances{};
    std::int32_t StealingMode{};
    std::int32_t ChannelIndex{};
    std::uint32_t ChannelOrder{};
    std::map<std::uint32_t, Property> Properties;
};
Bus ReadBus(std::string_view bytes, const Bank::Chunk& chunk);
}
#endif
