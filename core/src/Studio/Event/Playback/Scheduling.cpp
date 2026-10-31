#include "OpenFmod/Studio/Event/Playback/Instruments.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace OpenFmod::Studio {
void EventInstruments::schedule(const TimelineTrigger& key, const Trigger& trigger, const AutomationInput& input, std::uint64_t tick) {
    const auto& model = *trigger.Model;
    const auto& settings = model.Settings;
    const auto count = std::count_if(instances.begin(), instances.end(), [&](const auto& instance) { return instance.Model->Definition.Id == model.Definition.Id; });
    if (static_cast<std::uint64_t>(count) >= settings.MaximumInstances && settings.StealingMode == 0) return;
    float probability = settings.Properties.at(8);
    const auto property = model.Properties.find(8);
    if (property != model.Properties.end()) {
        PropertyInstance automation(library, property->second, bindings, random);
        probability = automation.Evaluate(probability, input, 0);
    }
    if (!std::isfinite(probability)) throw FormatError("Nonfinite instrument playback probability");
    if (probability < 100 && sample() * 100 > probability) return;
    std::uint64_t delay = 0;
    if (settings.MaximumDelayMilliseconds > 0) {
        const auto milliseconds = settings.MinimumDelayMilliseconds + sample() * (settings.MaximumDelayMilliseconds - settings.MinimumDelayMilliseconds);
        const auto ticks = static_cast<double>(milliseconds) * 48;
        if (ticks > std::numeric_limits<std::uint32_t>::max()) throw FormatError("Instrument start delay overflow");
        delay = static_cast<std::uint32_t>(ticks);
    }
    if (delay > std::numeric_limits<std::uint64_t>::max() - tick) throw FormatError("Instrument start time overflow");
    const auto due = quantize(tick + delay, settings.QuantizationMode, settings.QuantizationInterval);
    if (due < tick + delay) throw FormatError("Instrument quantization moved its start backwards");
    auto offset = static_cast<std::uint64_t>(settings.SourceOffsetTicks) + trigger.Offset;
    if (key.TimeLocked) {
        if (due - trigger.Start > std::numeric_limits<std::uint64_t>::max() - offset) throw FormatError("Time locked source offset overflow");
        offset += due - trigger.Start;
    }
    if (offset > std::numeric_limits<std::uint32_t>::max()) throw FormatError("Instrument source offset overflow");
    if (settings.StealingMode == 2 && static_cast<std::uint64_t>(count) >= settings.MaximumInstances) {
        auto remaining = static_cast<std::uint64_t>(count);
        for (auto it = instances.begin(); it != instances.end() && remaining >= settings.MaximumInstances;) {
            if (it->Model->Definition.Id != model.Definition.Id) { ++it; continue; }
            if (it->Started) it->Playback->Stop(true, static_cast<double>(tick - it->Due) / 48);
            it = instances.erase(it);
            --remaining;
        }
    }
    instances.push_back({key, trigger.Model, {}, due, static_cast<std::uint32_t>(offset), false});
}
}
