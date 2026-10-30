#include "OpenFmod/Studio/Timeline/Playback/Transport.hpp"
#include "OpenFmod/Studio/Timeline/Bounds.hpp"
#include <algorithm>
namespace OpenFmod::Studio {
std::uint32_t TimelineTransport::advanceSequence(std::uint32_t ticks, const std::function<float(const Bank::Guid&)>& parameters, const std::function<float()>& random, const std::function<void(const TimelineSpan&)>& consume) {
    auto& state = *sequence;
    const auto& definition = *state.Model->Sequence;
    const auto destinationStart = definition.Duration - definition.DestinationOffset;
    auto count = std::min(ticks, definition.Duration - state.Position);
    const bool sourceActive = state.Position < definition.SourceOffset;
    const bool destinationActive = state.Position >= destinationStart;
    if (sourceActive) count = std::min(count, definition.SourceOffset - state.Position);
    if (!destinationActive) count = std::min(count, destinationStart - state.Position);
    std::optional<LogicPoint> sourceLead;
    std::optional<LogicPoint> destinationLead;
    if (sourceActive) {
        sourceLead = sourceScan.Find({state.Source.Position, TimelineEnd(state.Source.Position, count), false, true, stopping, 0}, parameters, random);
        if (sourceLead) count = std::min(count, sourceLead->Tick - state.Source.Position);
    }
    if (destinationActive) {
        destinationLead = destinationScan.Find({state.Destination.Position, TimelineEnd(state.Destination.Position, count), false, true, stopping, 0}, parameters, random);
        if (destinationLead) count = std::min(count, destinationLead->Tick - state.Destination.Position);
    }

    TimelineSpan span;
    span.Duration = count;
    span.Sequence = &definition;
    span.SequencePosition = state.Position;
    if (sourceActive) span.Cursors[span.Count++] = state.Source;
    if (destinationActive) span.Cursors[span.Count++] = state.Destination;
    span.Cursors[span.Count++] = {state.Position, state.Generation, TimelineRole::Transition, true};
    if (count) consume(span);
    state.Position += count;
    const auto advance = [&](TimelineCursor& cursor, const std::optional<LogicPoint>& lead) {
        cursor.Position = TimelineEnd(cursor.Position, count);
        if (lead && cursor.Position == lead->Tick) {
            cursor.Position = RequireMarker(model.Definition, model.Transitions.at(*lead->Transition).Destination).Position;
            cursor.Generation = nextGeneration();
        }
    };
    if (sourceActive) {
        advance(state.Source, sourceLead);
        if (sourceLead && sourceLead->Tick == span.Cursors[0].Position + count) sourceScan.Reset();
    }
    if (destinationActive) {
        const auto previous = state.Destination.Position;
        advance(state.Destination, destinationLead);
        if (destinationLead && destinationLead->Tick == previous + count) destinationScan.Reset();
    }
    if (state.Position == definition.Duration) {
        current = state.Destination;
        current.Role = TimelineRole::Main;
        sequence.reset();
        scan.Reset();
        includeStart = true;
    }
    return count;
}
}
