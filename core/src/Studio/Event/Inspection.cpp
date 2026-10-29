#include "OpenFmod/Studio/Event/Inspection.hpp"
#include "OpenFmod/Studio/Timeline/Bounds.hpp"
#include <set>
namespace OpenFmod::Studio {
namespace {
class Inspection {
public:
    explicit Inspection(const Library& library) : library(library) {}
    bool EventOneshot(const Bank::Guid& id) {
        const auto cached = events.find(id);
        if (cached != events.end()) return cached->second;
        if (!activeEvents.insert(id).second) throw FormatError("Cyclic nested event reference");
        const auto& event = library.Require<PropertyModel<Event>>(id).Definition;
        if (event.Flags & 0x1000) {
            activeEvents.erase(id);
            const bool result = (event.Flags & 0x800) != 0;
            events.emplace(id, result);
            return result;
        }
        bool result = (event.Flags & 2) == 0;
        for (const auto& layoutId : event.ParameterLayouts) {
            if (!result) break;
            const auto& layout = library.Require<ParameterLayout>(layoutId);
            library.Require<Parameter>(layout.ParameterId);
            for (const auto& instrument : layout.Instruments) {
                if (!instrumentOneshot(instrument)) { result = false; break; }
            }
        }
        if (result && event.Timeline != Bank::Guid{}) result = timelineOneshot(library.Require<TimelineResource>(event.Timeline));
        activeEvents.erase(id);
        events.emplace(id, result);
        return result;
    }
private:
    bool timelineOneshot(const TimelineResource& timeline) const {
        if (!timeline.Definition.SustainPoints.empty()) return false;
        for (const auto& transition : timeline.Transitions) {
            if (transition.Destination == Bank::Guid{}) continue;
            if (RequireMarker(timeline.Definition, transition.Destination).Position < transition.End) return false;
        }
        return true;
    }
    bool instrumentOneshot(const Bank::Guid& id) {
        if (!activeInstruments.insert(id).second) throw FormatError("Cyclic playlist instrument reference");
        const auto& model = library.Require<Instrument>(id);
        bool result = model.Settings.LoopCount != -1;
        if (result) {
            if (std::holds_alternative<ProgrammerInstrument>(model.Definition.Source)) result = false;
            else if (const auto* nested = std::get_if<EventInstrument>(&model.Definition.Source)) result = EventOneshot(nested->Event);
            else if (std::holds_alternative<MultiInstrument>(model.Definition.Source)) {
                if (!model.Variants) throw FormatError("Multi instrument has no playlist");
                for (const auto& entry : model.Variants->Entries) {
                    if (!instrumentOneshot(entry.Id)) { result = false; break; }
                }
            }
        }
        activeInstruments.erase(id);
        return result;
    }
    const Library& library;
    std::map<Bank::Guid, bool> events;
    std::set<Bank::Guid> activeEvents;
    std::set<Bank::Guid> activeInstruments;
};
}
bool IsEventOneshot(const Library& library, const Bank::Guid& event) { return Inspection(library).EventOneshot(event); }
}
