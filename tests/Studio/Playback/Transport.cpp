#include "OpenFmod/Studio/Timeline/Playback/Transport.hpp"
#include <stdexcept>
void CheckTimelineTransport() {
    using namespace OpenFmod::Studio;
    TimelineResource model;
    NamedMarker destination;
    destination.Id[0] = 1;
    destination.Position = 100;
    model.Definition.Markers.push_back(destination);
    Transition jump;
    jump.Start = jump.End = 10;
    jump.Destination = destination.Id;
    jump.Probability = 100;
    model.Transitions.push_back(jump);
    TimelineTransport transport(model);
    std::vector<TimelineSpan> spans;
    const auto consume = [&](const auto& span) { spans.push_back(span); };
    transport.Advance(30, {}, {}, consume);
    if (transport.GetPosition() != 120 || spans.size() != 2 || spans[0].Duration != 10
        || spans[1].Cursors[0].Position != 100) throw std::runtime_error("Timeline jump consumed the wrong duration");
    model.Definition.SustainPoints.push_back({105, Condition(std::string_view{})});
    transport.Seek(100);
    spans.clear();
    transport.Advance(20, {}, {}, consume);
    if (!transport.IsSustaining() || transport.GetPosition() != 105 || spans.size() != 2
        || spans.back().Cursors[0].Advancing || spans.back().Duration != 15) throw std::runtime_error("Sustain did not hold timeline while consuming audio time");
    transport.KeyOff();
    transport.Advance(10, {}, {}, consume);
    if (transport.IsSustaining() || transport.GetPosition() != 115) throw std::runtime_error("Key off did not resume the timeline");
    model.Definition.SustainPoints.clear();
    TransitionSequence sequence;
    sequence.Duration = 20;
    sequence.SourceOffset = 8;
    sequence.DestinationOffset = 6;
    model.Transitions[0].Sequence = sequence;
    transport.Seek(0);
    spans.clear();
    transport.Advance(35, {}, {}, consume);
    if (spans.size() != 5 || spans[1].Count != 2 || spans[2].Count != 1 || spans[3].Count != 2
        || spans[2].Cursors[0].Role != TimelineRole::Transition || transport.GetPosition() != 111) {
        throw std::runtime_error("Transition sequence source and destination overlaps were not separated");
    }
    const auto destinationGeneration = spans[3].Cursors[0].Generation;
    if (spans[4].Cursors[0].Generation != destinationGeneration) throw std::runtime_error("Destination playback restarted at transition completion");
    model.Transitions[0].Sequence.reset();
    model.Definition.Markers[0].Position = 10;
    model.Transitions[0].Flags = 2;
    transport.Seek(10);
    transport.Advance(1, {}, {}, consume);
    if (transport.GetPosition() != 11) throw std::runtime_error("Point transition retriggered after its own jump");
    model.Transitions[0].Flags = 1;
    transport.Seek(10);
    bool rejected = false;
    try { transport.Advance(1, {}, {}, consume); }
    catch (const OpenFmod::FormatError&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Instantaneous transition cycle did not fail");
}
