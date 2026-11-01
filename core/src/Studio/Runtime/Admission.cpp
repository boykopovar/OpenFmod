#include "OpenFmod/Studio/Runtime/State.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace OpenFmod::Studio {
bool RuntimeContext::Admit(EventInstance& event) {
    auto& state = *event.state;
    const auto& model = state.Model->Definition;
    const auto cooldown = static_cast<double>(model.CooldownSeconds) * 48000;
    if (!std::isfinite(cooldown) || cooldown < 0 || cooldown >= static_cast<double>(std::numeric_limits<std::uint64_t>::max())) throw FormatError("Invalid event cooldown");
    const auto tick = Clock.GetTick();
    const auto previous = lastStarts.find(model.Id);
    if (previous != lastStarts.end() && tick - previous->second < static_cast<std::uint64_t>(cooldown)) return false;
    if (model.Stealing != StealingMode::Virtualize) {
        std::vector<std::shared_ptr<EventInstance>> active;
        for (const auto& entry : Events) if (auto instance = entry.lock(); instance && instance->IsPlaying() && instance->GetId() == model.Id) active.push_back(std::move(instance));
        if (active.size() >= model.MaximumInstances) {
            if (!model.MaximumInstances || model.Stealing == StealingMode::Reject) return false;
            if (model.Stealing != StealingMode::Oldest) throw FormatError("Event stealing requires evaluated spatial audibility");
            std::sort(active.begin(), active.end(), [](const auto& first, const auto& second) { return first->state->LaunchOrder < second->state->LaunchOrder; });
            const auto count = active.size() - model.MaximumInstances + 1;
            for (std::size_t index = 0; index < count; ++index) active[index]->Stop(true);
        }
    }
    if (launchSequence == std::numeric_limits<std::uint64_t>::max()) throw FormatError("Event launch sequence overflow");
    state.LaunchOrder = ++launchSequence;
    lastStarts[model.Id] = tick;
    return true;
}
}
