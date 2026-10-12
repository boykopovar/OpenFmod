#ifndef OPENFMOD_STUDIO_EVENT_MODEL_HPP
#define OPENFMOD_STUDIO_EVENT_MODEL_HPP
#include "OpenFmod/Bank/Identity.hpp"
#include <map>
#include <string>
namespace OpenFmod::Studio {
enum class StealingMode : std::uint32_t { Reject, Virtualize, Oldest, Quietest, Furthest };
struct Event {
    Bank::Guid Id{};
    Bank::Guid Snapshot{};
    Bank::Guid Timeline{};
    Bank::Guid InputBus{};
    Bank::Guid MasterBus{};
    std::uint32_t MaximumInstances{};
    std::uint32_t Priority{};
    bool LegacyRejectNew{};
    std::uint32_t SchedulingMode{};
    std::vector<Bank::Guid> ParameterLayouts;
    std::map<std::string, float> NumericProperties;
    std::map<std::string, std::string> StringProperties;
    float DopplerPercent{};
    StealingMode Stealing{};
    float CooldownSeconds{};
    std::uint32_t Flags{};
    std::vector<Bank::Guid> Groups;
    std::vector<std::uint64_t> ParameterIds;
    std::vector<Bank::Guid> Instruments;
    float MinimumDistance{};
    float MaximumDistance{};
};
Event ReadEvent(std::string_view bytes);
}
#endif
