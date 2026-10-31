#include "OpenFmod/Studio/Event/Playback/Instruments.hpp"
#include <algorithm>
#include <cmath>
namespace OpenFmod::Studio {
EventInstruments::EventInstruments(const Library& library, const AutomationBindings& bindings, InstrumentFactory& factory, std::function<float()> random, InstrumentQuantizer quantize)
    : library(library), bindings(bindings), factory(factory), random(std::move(random)), quantize(std::move(quantize)) {
    if (!this->random || !this->quantize) throw FormatError("Missing instrument scheduling dependency");
}
void EventInstruments::checkTime(std::uint64_t tick) {
    if (tick < lastTick) throw FormatError("Instrument scheduling time moved backwards");
    lastTick = tick;
}
float EventInstruments::sample() {
    const auto value = random();
    if (!std::isfinite(value) || value < 0 || value > 1) throw FormatError("Invalid instrument scheduling random sample");
    return value;
}
void EventInstruments::Activate(const TimelineTrigger& key, const Bank::Guid& instrument, std::uint32_t offset, const AutomationInput& input, std::uint64_t tick) {
    checkTime(tick);
    if (stopping) throw FormatError("Cannot activate a stopped instrument scheduler");
    if (triggers.count(key)) throw FormatError("Instrument trigger is already active");
    Trigger trigger{library.Retain<Instrument>(instrument), tick, offset, false};
    trigger.Enabled = trigger.Model->Settings.Trigger.Evaluate(input.Parameters, false);
    if (trigger.Enabled) schedule(key, trigger, input, tick);
    triggers.emplace(key, std::move(trigger));
}
void EventInstruments::Deactivate(const TimelineTrigger& key, std::uint64_t tick) {
    checkTime(tick);
    const auto found = triggers.find(key);
    if (found == triggers.end()) throw FormatError("Instrument trigger is not active");
    end(key, key.TimeLocked, tick);
    triggers.erase(found);
}
void EventInstruments::end(const TimelineTrigger& key, bool cutOff, std::uint64_t tick) {
    for (auto it = instances.begin(); it != instances.end();) {
        if (key < it->Key || it->Key < key) { ++it; continue; }
        if (!it->Started) it = instances.erase(it);
        else {
            it->Playback->End(cutOff, static_cast<double>(tick - it->Due) / 48);
            ++it;
        }
    }
}
void EventInstruments::Update(const AutomationInput& input, std::uint64_t tick) {
    checkTime(tick);
    if (!stopping) for (auto& [key, trigger] : triggers) {
        const bool enabled = trigger.Model->Settings.Trigger.Evaluate(input.Parameters, false);
        if (enabled && !trigger.Enabled) schedule(key, trigger, input, tick);
        else if (!enabled && trigger.Enabled) end(key, true, tick);
        trigger.Enabled = enabled;
    }
    for (auto it = instances.begin(); it != instances.end();) {
        if (it->Due > tick) { ++it; continue; }
        if (!it->Started) {
            it->Playback = factory.Create(it->Model->Definition.Id);
            it->Playback->Start({it->Offset, 0, it->Model->Settings.LoopCount}, input);
            it->Started = true;
        }
        it->Playback->Update(input, static_cast<double>(tick - it->Due) / 48);
        if (!it->Playback->IsPlaying()) it = instances.erase(it);
        else ++it;
    }
}
void EventInstruments::Stop(bool immediate, std::uint64_t tick) {
    checkTime(tick);
    for (auto it = instances.begin(); it != instances.end();) {
        if (!it->Started) it = instances.erase(it);
        else {
            it->Playback->Stop(immediate, static_cast<double>(tick - it->Due) / 48);
            ++it;
        }
    }
    triggers.clear();
    stopping = true;
}
bool EventInstruments::IsPlaying() const {
    return std::any_of(instances.begin(), instances.end(), [](const auto& instance) { return !instance.Started || instance.Playback->IsPlaying(); });
}
std::optional<std::uint64_t> EventInstruments::NextStart() const {
    std::optional<std::uint64_t> result;
    for (const auto& instance : instances) if (!instance.Started && (!result || instance.Due < *result)) result = instance.Due;
    return result;
}
}
