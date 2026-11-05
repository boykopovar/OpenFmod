#include "OpenFmod/Studio/Timeline/Playback/Schedule.hpp"
#include <stdexcept>
void CheckTimelineSchedule() {
    using namespace OpenFmod::Studio;
    TimelineResource model;
    ScheduledInstrument sound;
    sound.Instrument[0] = 1;
    sound.Start = 10;
    sound.Duration = 20;
    model.Definition.TimeLocked.push_back(sound);
    model.Definition.Triggered.push_back(sound);
    TimelineSchedule schedule(model);
    auto actions = schedule.Process({5, {{{15, 1, TimelineRole::Main, true}}}, 1});
    if (actions.size() != 2 || actions[0].Offset != 0 || actions[1].Offset != 5
        || !actions[0].Activate || !actions[1].Activate) throw std::runtime_error("Timeline seek did not distinguish locked and triggered sound offsets");
    actions = schedule.Process({10, {{{20, 1, TimelineRole::Main, false}}}, 1});
    if (!actions.empty()) throw std::runtime_error("Sustain restarted an active instrument");
    actions = schedule.Process({10, {{{20, 1, TimelineRole::Main, true}}}, 1});
    if (actions.size() != 2 || actions[0].Activate || actions[1].Activate || actions[0].Delay != 10
        || actions[1].Delay != 10) throw std::runtime_error("Instrument end was not scheduled on its exact boundary");
    actions = schedule.Process({10, {{{30, 1, TimelineRole::Main, true}}}, 1});
    if (!actions.empty()) throw std::runtime_error("Finished instrument retriggered");
    schedule.Clear();
    schedule.Process({5, {{{10, 2, TimelineRole::Source, true}}}, 1});
    actions = schedule.Process({5, {{{100, 3, TimelineRole::Destination, true}}}, 1});
    if (actions.size() != 2 || actions[0].Activate || actions[0].Delay != 0) throw std::runtime_error("Removed transition source was not deactivated");
    schedule.Clear();
    model.Definition.TimeLocked[0].Duration = 0;
    model.Definition.Triggered.clear();
    actions = schedule.Process({20, {{{0, 4, TimelineRole::Main, true}}}, 1});
    if (actions.size() != 2 || !actions[0].Activate || actions[1].Activate
        || actions[0].Delay != 10 || actions[1].Delay != 10) throw std::runtime_error("Zero length timeline trigger changed action ordering");
    TimelineResource empty;
    TimelineSchedule transitions(empty);
    TransitionSequence sequence;
    transitions.Process({5, {{{0, 1, TimelineRole::Source, true}, {10, 2, TimelineRole::Destination, true}, {20, 3, TimelineRole::Transition, true}}}, 3, &sequence});
    transitions.Process({5, {{{25, 3, TimelineRole::Transition, true}, {5, 1, TimelineRole::Source, true}, {15, 2, TimelineRole::Destination, true}}}, 3, &sequence});
    transitions.Process({5, {{{20, 2, TimelineRole::Main, true}}}, 1});
    bool rejected = false;
    try { transitions.Process({5, {{{24, 2, TimelineRole::Main, true}}}, 1}); }
    catch (const OpenFmod::FormatError&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Timeline cursor continuity was not checked after a transition");
    rejected = false;
    try { transitions.Process({5, {{{25, 2, TimelineRole::Source, true}, {25, 2, TimelineRole::Destination, true}}}, 2}); }
    catch (const OpenFmod::FormatError&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Duplicate timeline cursor generation was accepted");
    transitions.Clear();
    transitions.Process({5, {{{0, 2, TimelineRole::Main, true}}}, 1});

}
