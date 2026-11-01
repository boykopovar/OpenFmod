#include "OpenFmod/Studio/Runtime/State.hpp"
namespace OpenFmod::Studio {
void EventState::Prepare(std::uint32_t ticks) {
    if (!Transport || !ticks || State == PlaybackState::Stopped || State == PlaybackState::Stopping) return;
    const auto begin = Context->Clock.GetTick();
    spans.clear();
    std::uint64_t elapsed = 0;
    Transport->Advance(ticks, Input().Parameters, [this] { return Context->Random(); }, [&](const auto& span) {
        if (span.Sequence && (!span.Sequence->Curves.empty() || !span.Sequence->CurveReplacements.empty())) {
            throw FormatError("Transition playback requires per cursor automation bindings");
        }
        spans.push_back({begin + elapsed, span});
        for (auto action : Schedule->Process(span)) actions.push_back({begin + elapsed + action.Delay, std::move(action)});
        elapsed += span.Duration;
    });
    plannedPosition = Transport->GetPosition();
    plannedUntil = begin + ticks;
}
void EventState::Update() {
    if (State == PlaybackState::Stopped) return;
    const auto now = Context->Clock.GetTick();
    Parameters.Advance(static_cast<double>(now - updated) / 48000);
    updated = now;
    if (now >= plannedUntil) Position = plannedPosition;
    else for (const auto& span : spans) if (span.Tick <= now && now < span.Tick + span.Value.Duration) {
        const TimelineCursor* cursor = &span.Value.Cursors[0];
        for (std::size_t index = 0; index < span.Value.Count; ++index) {
            if (span.Value.Cursors[index].Role == TimelineRole::Destination) cursor = &span.Value.Cursors[index];
        }
        Position = cursor->Position + (cursor->Advancing ? static_cast<std::uint32_t>(now - span.Tick) : 0);
        break;
    }
    updateCursors(now);
    const auto input = Input();
    updateMix(input, static_cast<double>(now - started) / 48);
    Instruments->Update(input, now);
    while (!actions.empty() && actions.front().Tick <= now) {
        const auto action = actions.front().Value;
        actions.pop_front();
        if (action.Activate) Instruments->Activate(action.Trigger, action.Instrument, action.Offset, input, now);
        else Instruments->Deactivate(action.Trigger, now);
        Instruments->Update(input, now);
    }
    if (Snapshot) Snapshot->Update(input, static_cast<double>(now - started) / 48);
    if (State != PlaybackState::Stopping) State = Transport && Transport->IsSustaining() ? PlaybackState::Sustaining : PlaybackState::Playing;
    const bool audioFinished = !Instruments->IsPlaying();
    const bool mayFinish = (Model->Definition.Flags & 2) == 0 || ending;
    if (State != PlaybackState::Stopping && mayFinish && audioFinished && Position >= duration && State != PlaybackState::Sustaining) Stop(false);
    if (State == PlaybackState::Stopping && audioFinished && mixFinished(static_cast<double>(now - started) / 48) && (!Snapshot || !Snapshot->IsPlaying())) State = PlaybackState::Stopped;
}
std::optional<std::uint64_t> EventState::NextBoundary() const {
    if (State == PlaybackState::Stopped) return {};
    auto result = Instruments->NextStart();
    if (!actions.empty() && (!result || actions.front().Tick < *result)) result = actions.front().Tick;
    return result;
}
void EventState::StartEvent(const Bank::Guid& event) {
    auto context = Context;
    Context->Commands.push_back([context, event] { auto instance = context->Create(event); instance->Start(); });
}
void EventState::StopEvents(const Bank::Guid& event, bool immediate) {
    auto context = Context;
    Context->Commands.push_back([context, event, immediate] {
        for (const auto& entry : context->Events) if (auto instance = entry.lock(); instance && instance->GetId() == event) instance->Stop(immediate);
    });
}
float EventState::GetParameter(const Bank::Guid& parameter) const {
    return GetParameterModel(parameter).Flags & 1 ? Context->Globals.GetValue(parameter) : Parameters.GetValue(parameter);
}
const Parameter& EventState::GetParameterModel(const Bank::Guid& parameter) const { return Context->Resources.Require<Parameter>(parameter); }
void EventState::SetParameter(const Bank::Guid& parameter, float value) {
    if (GetParameterModel(parameter).Flags & 1) Context->Globals.SetValue(parameter, value);
    else Parameters.SetValue(parameter, value);
}
}
