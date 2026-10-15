#ifndef OPENFMOD_STUDIO_INSTRUMENT_SETTINGS_HPP
#define OPENFMOD_STUDIO_INSTRUMENT_SETTINGS_HPP
#include "OpenFmod/Studio/Condition.hpp"
#include <map>
#include "OpenFmod/Studio/Bus/Routing.hpp"
namespace OpenFmod::Studio {
struct InstrumentSettings {
    Bank::Guid Owner{};
    std::map<std::uint32_t, float> Properties;
    std::int32_t LoopCount{};
    std::uint32_t Flags{};
    float MinimumDelayMilliseconds{};
    float MaximumDelayMilliseconds{};
    std::uint32_t QuantizationMode{};
    std::uint32_t QuantizationInterval{};
    Bank::Guid AutoPitchParameter{};
    float AutoPitchMaximum{};
    float AutoPitchMinimum{};
    std::uint32_t MaximumInstances{};
    Studio::Routing Routing;
    std::uint32_t StealingMode{};
    std::uint32_t SourceOffsetTicks{};
    Condition Trigger{std::string_view{}};
};
InstrumentSettings ReadInstrumentSettings(std::string_view bytes);
}
#endif
