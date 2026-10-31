#include "OpenFmod/Studio/Playback/Control/Nested.hpp"
#include <cmath>
namespace OpenFmod::Studio {
NestedPlayback::NestedPlayback(const Library& library, const Bank::Guid& instrument, const AutomationBindings& bindings, const std::function<float()>& random, const EventVoiceFactory& factory)
    : model(library.Retain<Instrument>(instrument)), controls(library, *model, bindings, random) {
    const auto* definition = std::get_if<EventInstrument>(&model->Definition.Source);
    if (!definition || !factory) throw FormatError("Nested playback requires an event instrument and event factory");
    snapshot = library.Require<PropertyModel<Event>>(definition->Event).Definition.Snapshot != Bank::Guid{};
    child = factory(definition->Event);
    if (!child) throw FormatError("Event factory returned an empty voice");
}
void NestedPlayback::apply(const AutomationInput& input, float gain, float pitch) {
    const auto& definition = std::get<EventInstrument>(model->Definition.Source);
    if (snapshot) {
        const auto value = controls.Get(3);
        const auto found = previous.find(3);
        if (found == previous.end() || found->second != value) {
            child->SetSnapshotIntensity(value);
            previous[3] = value;
        }
    }
    for (const auto& parameter : definition.Parameters) {
        const auto value = controls.Get(parameter.PropertyId);
        const auto found = previous.find(parameter.PropertyId);
        if (found == previous.end() || found->second != value) {
            child->SetParameter(parameter.Parameter, value);
            previous[parameter.PropertyId] = value;
        }
    }
    child->SetMix(gain * controls.GetGain(), pitch * controls.GetPitch(input));
}
void NestedPlayback::Start(const SoundStart& start, const AutomationInput& input) {
    if (started) throw FormatError("Nested playback has already started");
    if (start.Repeats < -1 || !std::isfinite(start.OffsetPercent) || start.OffsetPercent < 0 || start.OffsetPercent > 100) throw FormatError("Invalid nested event start position");
    controls.Update(input, 0);
    apply(input, 1, 1);
    child->Start(start);
    started = true;
}
void NestedPlayback::Update(const AutomationInput& input, double milliseconds, float parentGain, float parentPitch) {
    if (!started) throw FormatError("Nested playback has not started");
    if (!std::isfinite(milliseconds) || milliseconds < lastUpdate || !std::isfinite(parentGain) || parentGain < 0
        || !std::isfinite(parentPitch) || parentPitch < 0) throw FormatError("Invalid nested event update");
    lastUpdate = milliseconds;
    if (!child->IsPlaying()) return;
    controls.Update(input, milliseconds);
    apply(input, parentGain, parentPitch);
}
void NestedPlayback::Stop(bool immediate, double milliseconds) {
    if (!started) throw FormatError("Nested playback has not started");
    if (!std::isfinite(milliseconds) || milliseconds < lastUpdate) throw FormatError("Invalid nested event stop time");
    if (immediate) child->Stop(true);
    else if (!releasing) {
        controls.Release(milliseconds);
        releasing = true;
        child->Stop(false);
    }
}
bool NestedPlayback::IsPlaying() const { return started && child->IsPlaying(); }
void NestedPlayback::End(bool cutOff, double milliseconds) {
    if (!started || !std::isfinite(milliseconds) || milliseconds < lastUpdate) throw FormatError("Invalid nested event end time");
    if (controls.HasEnvelope()) Stop(false, milliseconds);
    else child->End(cutOff || (model->Settings.Flags & 1));
}
}
