#include "OpenFmod/Studio/Timeline/Playback/Schedule.hpp"
#include "OpenFmod/Studio/Timeline/Playback/Bounds.hpp"
#include <algorithm>
#include <tuple>
namespace OpenFmod::Studio {
bool TimelineTrigger::operator<(const TimelineTrigger& other) const {
    return std::tie(Generation, TimeLocked, Index) < std::tie(other.Generation, other.TimeLocked, other.Index);
}
TimelineSchedule::TimelineSchedule(const TimelineResource& model) : model(model) {}
void TimelineSchedule::schedule(const TimelineSpan& span, const TimelineCursor& cursor, const std::vector<ScheduledInstrument>& entries, bool timeLocked, std::vector<TimelineAction>& actions) {
    const auto end = cursor.Advancing ? TimelineEnd(cursor.Position, span.Duration) : cursor.Position;
    const bool entering = std::none_of(positions.begin(), positions.begin() + positionCount, [&](const auto& entry) { return entry.first == cursor.Generation; });
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto& entry = entries[index];
        const auto finish = TimelineEnd(entry.Start, entry.Duration);
        const TimelineTrigger key{cursor.Generation, index, timeLocked};
        const bool starts = cursor.Advancing && cursor.Position <= entry.Start && entry.Start < end;
        const bool overlaps = entering && entry.Start <= cursor.Position && cursor.Position < finish;
        if ((starts || overlaps) && !active.count(key)) {
            const auto position = std::max(cursor.Position, entry.Start);
            TimelineAction action{key, entry.Instrument, position - cursor.Position, timeLocked ? position - entry.Start : 0, cursor.Role, true};
            actions.push_back(action);
            active.emplace(key, action);
        }
        auto found = active.find(key);
        if (found != active.end()) {
            found->second.Role = cursor.Role;
            if (cursor.Advancing && cursor.Position <= finish && finish <= end) {
                auto action = found->second;
                action.Activate = false;
                action.Delay = finish - cursor.Position;
                actions.push_back(action);
                active.erase(found);
            }
        }
    }
}
std::vector<TimelineAction> TimelineSchedule::Process(const TimelineSpan& span) {
    if (!span.Duration || !span.Count || span.Count > span.Cursors.size()) throw FormatError("Invalid timeline schedule span");
    const auto containsGeneration = [&](std::uint64_t generation) {
        return std::any_of(span.Cursors.begin(), span.Cursors.begin() + span.Count, [&](const auto& cursor) { return cursor.Generation == generation; });
    };
    for (std::size_t i = 0; i < span.Count; ++i) {
        const auto& cursor = span.Cursors[i];
        if (!cursor.Generation || std::any_of(span.Cursors.begin(), span.Cursors.begin() + i, [&](const auto& previous) { return previous.Generation == cursor.Generation; })) throw FormatError("Duplicate timeline cursor generation");
        const auto found = std::find_if(positions.begin(), positions.begin() + positionCount, [&](const auto& previous) { return previous.first == cursor.Generation; });
        if (found != positions.begin() + positionCount && found->second != cursor.Position) throw FormatError("Discontinuous timeline cursor generation");
        if (cursor.Role == TimelineRole::Transition && !span.Sequence) throw FormatError("Transition cursor has no sequence");
    }
    std::vector<TimelineAction> actions;
    for (auto it = active.begin(); it != active.end();) {
        if (!containsGeneration(it->first.Generation)) {
            auto action = it->second;
            action.Activate = false;
            action.Delay = 0;
            actions.push_back(action);
            it = active.erase(it);
        } else ++it;
    }
    for (std::size_t i = 0; i < span.Count; ++i) {
        const auto& cursor = span.Cursors[i];
        const auto& triggered = cursor.Role == TimelineRole::Transition ? span.Sequence->Triggered : model.Definition.Triggered;
        const auto& locked = cursor.Role == TimelineRole::Transition ? span.Sequence->TimeLocked : model.Definition.TimeLocked;
        schedule(span, cursor, triggered, false, actions);
        schedule(span, cursor, locked, true, actions);
    }
    positionCount = span.Count;
    for (std::size_t i = 0; i < span.Count; ++i) {
        const auto& cursor = span.Cursors[i];
        positions[i] = {cursor.Generation, cursor.Advancing ? TimelineEnd(cursor.Position, span.Duration) : cursor.Position};
    }
    std::stable_sort(actions.begin(), actions.end(), [](const auto& left, const auto& right) { return left.Delay < right.Delay; });
    return actions;
}
std::vector<TimelineAction> TimelineSchedule::Clear() {
    std::vector<TimelineAction> result;
    result.reserve(active.size());
    for (const auto& [key, value] : active) {
        auto action = value;
        action.Delay = 0;
        action.Activate = false;
        result.push_back(action);
        static_cast<void>(key);
    }
    active.clear();
    positionCount = 0;
    return result;
}
}
