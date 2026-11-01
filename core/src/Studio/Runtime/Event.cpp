#include "OpenFmod/Studio/Runtime/State.hpp"
#include <cmath>
namespace OpenFmod::Studio {
EventInstance::EventInstance(std::shared_ptr<RuntimeContext> context, const Bank::Guid& event, std::shared_ptr<EventRouting> parent, std::optional<Bank::Guid> input)
    : state(std::make_unique<EventState>(std::move(context), event, std::move(parent), input)) {}
EventInstance::~EventInstance() = default;
void EventInstance::Start(const SoundStart& position) {
    state->Stop(true);
    if (!state->Context->Admit(*this)) return;
    state->Start(position);
    state->Context->Retain(shared_from_this());
}
void EventInstance::Stop(bool immediate) { state->Stop(immediate); }
void EventInstance::End(bool cutOff) { state->End(cutOff); }
bool EventInstance::IsPlaying() const { return state->State != PlaybackState::Stopped; }
PlaybackState EventInstance::GetState() const { return state->State; }
std::uint32_t EventInstance::GetTimelinePosition() const { return state->Position; }
bool EventInstance::IsOneshot() const { return state->Oneshot; }
const Bank::Guid& EventInstance::GetId() const { return state->Model->Definition.Id; }
void EventInstance::SetMix(float gain, float pitch) {
    if (!std::isfinite(gain) || gain < 0 || !std::isfinite(pitch) || pitch <= 0) throw FormatError("Invalid event mix controls");
    if (state->Routing) {
        state->Context->Buses.SetVolume(state->Model->Definition.MasterBus, gain, state->Routing->Scope);
        state->Context->Buses.SetPitch(state->Model->Definition.MasterBus, pitch, state->Routing->Scope);
    }
    state->Gain = gain;
    state->Pitch = pitch;
}
void EventInstance::SetParameter(const Bank::Guid& parameter, float value) { state->Parameters.SetValue(parameter, value); }
void EventInstance::SetSnapshotIntensity(float percent) {
    if (!std::isfinite(percent) || percent < 0 || percent > 100) throw FormatError("Invalid event snapshot intensity");
    if (state->Model->Definition.Snapshot == Bank::Guid{}) throw FormatError("Event has no mixer snapshot");
    state->Intensity = percent;
    if (state->Snapshot) state->Snapshot->SetIntensity(percent);
}
void EventInstance::KeyOff() {
    if (!state->Transport) throw FormatError("Event has no timeline");
    state->Transport->KeyOff();
}
}
