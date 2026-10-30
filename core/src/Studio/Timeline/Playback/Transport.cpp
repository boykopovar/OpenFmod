#include "OpenFmod/Studio/Timeline/Playback/Transport.hpp"
#include "OpenFmod/Studio/Timeline/Bounds.hpp"
#include <limits>
namespace OpenFmod::Studio {
TimelineTransport::TimelineTransport(const TimelineResource& model)
    : model(model), scan(model), sourceScan(model), destinationScan(model) {
    current.Generation = generation;
    for (const auto& transition : model.Transitions) {
        if (transition.Sequence && (transition.Sequence->SourceOffset > transition.Sequence->Duration
            || transition.Sequence->DestinationOffset > transition.Sequence->Duration)) throw FormatError("Transition overlap exceeds its duration");
    }
}
std::uint64_t TimelineTransport::nextGeneration() {
    if (generation == std::numeric_limits<std::uint64_t>::max()) throw FormatError("Timeline generation overflow");
    return ++generation;
}
void TimelineTransport::Seek(std::uint32_t position) {
    current = {position, nextGeneration(), TimelineRole::Main, true};
    sequence.reset();
    sustain.reset();
    keyOffs = 0;
    includeStart = true;
    scan.Reset();
    sourceScan.Reset();
    destinationScan.Reset();
}
void TimelineTransport::KeyOff() {
    if (sustain) {
        sustain.reset();
        keyOffs = 1;
    }
}
void TimelineTransport::SetStopping(bool value) { stopping = value; }
std::uint32_t TimelineTransport::GetPosition() const {
    if (!sequence) return current.Position;
    const auto& state = *sequence;
    const auto& definition = *state.Model->Sequence;
    return state.Position >= definition.Duration - definition.DestinationOffset ? state.Destination.Position : current.Position;
}
bool TimelineTransport::IsSustaining() const { return sustain.has_value(); }
void TimelineTransport::begin(const LogicPoint& point) {
    if (point.Sustain) {
        sustain = point.Sustain;
        return;
    }
    const auto& transition = model.Transitions.at(*point.Transition);
    const auto destination = TransitionDestination(model.Definition, transition, current.Position);
    if (!transition.Sequence || !transition.Sequence->Duration) {
        Seek(destination);
        includeStart = false;
        return;
    }
    SequenceState state;
    state.Model = &transition;
    state.Source = current;
    state.Source.Role = TimelineRole::Source;
    if (point.LeadTransition) {
        state.Source.Position = RequireMarker(model.Definition, model.Transitions.at(*point.LeadTransition).Destination).Position;
        state.Source.Generation = nextGeneration();
    }
    state.Destination = {destination, nextGeneration(), TimelineRole::Destination, true};
    state.Generation = nextGeneration();
    sequence = state;
    sourceScan.Reset();
    destinationScan.Reset();
}
void TimelineTransport::Advance(std::uint32_t ticks, const std::function<float(const Bank::Guid&)>& parameters, const std::function<float()>& random, const std::function<void(const TimelineSpan&)>& consume) {
    if (!consume) throw FormatError("Missing timeline span consumer");
    unsigned instantaneous = 0;
    while (ticks) {
        if (sequence) {
            const auto count = advanceSequence(ticks, parameters, random, consume);
            ticks -= count;
            if (count) instantaneous = 0;
            else if (++instantaneous > 100) throw FormatError("Transition lead contains an instantaneous cycle");
            continue;
        }
        if (sustain && !model.Definition.SustainPoints.at(*sustain).Trigger.Evaluate(parameters, stopping)) KeyOff();
        if (sustain) {
            auto held = current;
            held.Advancing = false;
            consume({ticks, {held}, 1, nullptr, 0});
            return;
        }
        const auto end = TimelineEnd(current.Position, ticks);
        if (end == current.Position) throw FormatError("Timeline position overflow");
        const auto point = scan.Find({current.Position, end, includeStart, false, stopping, keyOffs}, parameters, random);
        const auto count = (point ? point->Tick : end) - current.Position;
        if (count) {
            consume({count, {current}, 1, nullptr, 0});
            current.Position += count;
            ticks -= count;
            keyOffs = 0;
            instantaneous = 0;
        } else if (++instantaneous > 100) throw FormatError("Timeline contains an instantaneous transition cycle");
        includeStart = false;
        if (point) begin(*point);
    }
}
}
