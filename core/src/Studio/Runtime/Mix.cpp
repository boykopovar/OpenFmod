#include "OpenFmod/Studio/Runtime/State.hpp"
#include <algorithm>
namespace OpenFmod::Studio {
void EventState::initializeMix() {
    mix.clear();
    if (!Routing) return;
    auto buses = Model->Definition.Groups;
    buses.push_back(Model->Definition.MasterBus);
    for (const auto& id : buses) {
        const auto& bus = Context->Resources.Require<Bus>(id);
        if (bus.Properties.empty()) continue;
        std::map<std::uint32_t, float> base{{0, bus.Levels.VolumeDecibels}, {1, bus.Levels.PitchSemitones}};
        mix.emplace(id, std::make_unique<PropertySet>(Context->Resources, std::move(base), bus.Properties, Bindings, [this] { return Context->Random(); }));
    }
}
void EventState::updateMix(const AutomationInput& input, double milliseconds) {
    for (auto& [id, properties] : mix) {
        properties->Update(input, milliseconds);
        Context->Buses.SetLevels(id, properties->Get(0), properties->Get(1), Routing->Scope);
    }
}
bool EventState::mixFinished(double milliseconds) const {
    return std::all_of(mix.begin(), mix.end(), [milliseconds](const auto& entry) { return entry.second->IsFinished(milliseconds); });
}
}
