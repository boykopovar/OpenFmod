#ifndef OPENFMOD_STUDIO_TIMELINE_PLAYBACK_SCHEDULE_HPP
#define OPENFMOD_STUDIO_TIMELINE_PLAYBACK_SCHEDULE_HPP
#include "OpenFmod/Studio/Timeline/Playback/Transport.hpp"
#include <map>
namespace OpenFmod::Studio {
struct TimelineTrigger {
    std::uint64_t Generation{};
    std::size_t Index{};
    bool TimeLocked{};
    bool operator<(const TimelineTrigger& other) const;
};
struct TimelineAction {
    TimelineTrigger Trigger;
    Bank::Guid Instrument{};
    std::uint32_t Delay{};
    std::uint32_t Offset{};
    TimelineRole Role{};
    bool Activate{};
};
class TimelineSchedule {
public:
    explicit TimelineSchedule(const TimelineResource& model);
    std::vector<TimelineAction> Process(const TimelineSpan& span);
    std::vector<TimelineAction> Clear();
private:
    void schedule(const TimelineSpan& span, const TimelineCursor& cursor, const std::vector<ScheduledInstrument>& entries, bool timeLocked, std::vector<TimelineAction>& actions);
    const TimelineResource& model;
    std::map<TimelineTrigger, TimelineAction> active;
    std::array<std::pair<std::uint64_t, std::uint32_t>, TimelineSpan{}.Cursors.size()> positions{};
    std::size_t positionCount{};
};
}
#endif
