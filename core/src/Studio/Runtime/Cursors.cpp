#include "OpenFmod/Studio/Runtime/State.hpp"
#include <algorithm>
#include <cmath>
namespace OpenFmod::Studio {
void EventState::updateCursors(std::uint64_t tick) {
    for (const auto& span : spans) {
        if (span.Tick > tick) break;
        const auto elapsed = static_cast<std::uint32_t>(std::min<std::uint64_t>(tick - span.Tick, span.Value.Duration));
        std::uint64_t sequenceGeneration = 0;
        for (std::size_t index = 0; index < span.Value.Count; ++index) {
            const auto& cursor = span.Value.Cursors[index];
            if (cursor.Role == TimelineRole::Transition) sequenceGeneration = cursor.Generation;
        }
        const auto sequencePosition = span.Value.SequencePosition + elapsed;
        for (std::size_t index = 0; index < span.Value.Count; ++index) {
            const auto& cursor = span.Value.Cursors[index];
            cursors[cursor.Generation] = {cursor.Position + (cursor.Advancing ? elapsed : 0), cursor.Role, span.Value.Sequence, sequencePosition, sequenceGeneration};
        }
        if (sequenceGeneration) for (auto& [generation, cursor] : cursors) {
            if (cursor.SequenceGeneration == sequenceGeneration) cursor.SequencePosition = sequencePosition;
            static_cast<void>(generation);
        }
    }
}
InstrumentFrame EventState::instrumentFrame(const TimelineTrigger& trigger, const Bank::Guid& instrument) const {
    InstrumentFrame frame{Input(), 1};
    if (!trigger.Generation) return frame;
    const auto found = cursors.find(trigger.Generation);
    if (found == cursors.end()) throw FormatError("Instrument timeline cursor is missing");
    const auto& cursor = found->second;
    frame.Input.Tick = cursor.Position;
    if (!cursor.Sequence || (cursor.Role != TimelineRole::Source && cursor.Role != TimelineRole::Destination)) return frame;
    const auto& fades = cursor.Role == TimelineRole::Source ? cursor.Sequence->SourceFades : cursor.Sequence->DestinationFades;
    for (const auto& fade : fades) if (fade.Target == instrument) {
        frame.Gain = Context->Resources.Require<Curve>(fade.Curve).EvaluateTick(cursor.SequencePosition);
        if (cursor.Sequence->FadeMapping != Bank::Guid{}) frame.Gain = Context->Resources.Require<Mapping>(cursor.Sequence->FadeMapping).Transform(frame.Gain);
        if (!std::isfinite(frame.Gain) || frame.Gain < 0) throw FormatError("Invalid transition fade gain");
        break;
    }
    return frame;
}
}
