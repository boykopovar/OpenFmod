#include "OpenFmod/Studio/Timeline/Scan.hpp"
#include <stdexcept>
#include "OpenFmod/Studio/Timeline/Bounds.hpp"
#include "OpenFmod/Studio/Timeline/TempoGrid.hpp"
void CheckTimelineTransport();
void CheckTimelineSchedule();
void CheckTimelineScan() {
    CheckTimelineTransport();
    CheckTimelineSchedule();
    using namespace OpenFmod::Studio;
    TempoGrid tempo({{{}, 4, 4, 0, 120}, {{}, 4, 4, 48000, 60}});
    if (tempo.ToBars(0, 96000) != 3 || tempo.ToTicks(0, 3) != 96000
        || tempo.ToTicks(24000, 2) != 72000) throw std::runtime_error("Musical conversion lost a tempo boundary");
    Timeline destinationModel;
    NamedMarker destination;
    destination.Id[0] = 1;
    destination.Position = 100;
    destination.Duration = 20;
    destinationModel.Markers.push_back(destination);
    Transition relative;
    relative.Destination = destination.Id;
    relative.Start = 10;
    relative.Flags = 4;
    if (TransitionDestination(destinationModel, relative, 30) != 119
        || TransitionDestination(destinationModel, relative, 33) != 103) throw std::runtime_error("Relative transition wrapped incorrectly");
    relative.Flags |= 8;
    if (TransitionDestination(destinationModel, relative, 10) != 119
        || TransitionDestination(destinationModel, relative, 30) != 100
        || TransitionDestination(destinationModel, relative, 33) != 117) throw std::runtime_error("Reverse relative transition wrapped incorrectly");
    TimelineResource model;
    Transition transition;
    transition.Destination[0] = 1;
    transition.Start = 10;
    transition.End = 20;
    transition.Probability = 50;
    model.Transitions.push_back(transition);
    TimelineScan scanner(model);
    unsigned rolls = 0;
    const auto fail = [&] { ++rolls; return 0.9f; };
    if (scanner.Find({0, 11}, {}, fail) || scanner.Find({11, 12}, {}, fail) || rolls != 1) {
        throw std::runtime_error("Rejected transition rerolled inside the same region");
    }
    scanner.Find({20, 21}, {}, fail);
    scanner.Find({10, 11}, {}, fail);
    if (rolls != 2) throw std::runtime_error("Transition rejection survived leaving its region");
    scanner.Reset();
    const auto selected = scanner.Find({0, 11}, {}, [] { return 0.1f; });
    if (!selected || selected->Tick != 10 || selected->Transition != 0) throw std::runtime_error("Transition start boundary was lost");
    model.Transitions[0].Probability = 100;
    model.Transitions[0].End = 10;
    scanner.Reset();
    if (scanner.Find({10, 11}, {}, {})) throw std::runtime_error("Point transition retriggered at an excluded boundary");
    if (!scanner.Find({10, 11, true}, {}, {})) throw std::runtime_error("Initial point transition was skipped");
    model.Transitions[0].Flags = 1;
    if (!scanner.Find({10, 11}, {}, {})) throw std::runtime_error("Inclusive point transition ignored its flag");
    model.Definition.SustainPoints.push_back({10, Condition(std::string_view{})});
    auto held = scanner.Find({0, 11}, {}, {});
    if (!held || held->Sustain != 0 || held->Transition) throw std::runtime_error("Transition took priority over coincident sustain");
    auto released = scanner.Find({0, 11, false, false, false, 1}, {}, {});
    if (!released || !released->Transition || released->Sustain) throw std::runtime_error("Key off did not bypass sustain");
    model.Definition.SustainPoints.clear();
    model.Transitions[0].Start = 0;
    model.Transitions[0].End = 48000;
    model.Transitions[0].QuantizationMode = 2;
    model.Transitions[0].QuantizationInterval = 1;
    model.Transitions[0].Probability = 50;
    rolls = 0;
    auto quantized = scanner.Find({1, 24001}, {}, [&] { return ++rolls == 1 ? 0.9f : 0.1f; });
    if (!quantized || quantized->Tick != 24000 || rolls != 2) throw std::runtime_error("Transition did not retry at the next musical boundary");
    scanner.Reset();
    if (scanner.Find({1, 12000}, {}, [] { return 0.1f; })) throw std::runtime_error("Quantized transition crossed its exclusive window end");
    TimelineResource simultaneous;
    Transition first;
    first.Destination[0] = 1;
    first.Start = 10;
    first.End = 10;
    first.Probability = 100;
    first.Sequence = TransitionSequence{};
    first.Sequence->Duration = 100;
    simultaneous.Transitions.push_back(first);
    first.Flags = 2;
    simultaneous.Transitions.push_back(first);
    TimelineScan ties(simultaneous);
    if (ties.Find({0, 11}, {}, {})->LeadTransition) throw std::runtime_error("Transition without source overlap accepted a lead");
    simultaneous.Transitions[0].Sequence->SourceOffset = 5;
    if (ties.Find({0, 11}, {}, {})->LeadTransition != 1) throw std::runtime_error("Transition source overlap lost its lead");

}
