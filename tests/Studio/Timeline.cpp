#include "OpenFmod/Studio/Timeline/Model.hpp"
#include "OpenFmod/Studio/Timeline/Transition.hpp"
#include "OpenFmod/Studio/Timeline/Timing/TempoGrid.hpp"
#include "OpenFmod/Studio/Timeline/Timing/Clock.hpp"
#include "OpenFmod/Studio/Timeline/Playback/Bounds.hpp"
#include "../Data/Writer.hpp"
#include <stdexcept>
void CheckTimeline() {
    using namespace TestData;
    using namespace OpenFmod::Studio;
    TimelineResource bounds;
    bounds.Definition.Triggered.push_back({{}, 10, 40});
    bounds.Definition.Markers.push_back({{}, 60, "destination", 10});
    Transition sequenceTransition;
    sequenceTransition.Sequence = TransitionSequence{};
    sequenceTransition.Sequence->Duration = 20;
    sequenceTransition.Sequence->DestinationOffset = 35;
    bounds.Transitions.push_back(sequenceTransition);
    if (TimelineDuration(bounds) != 95 || TimelineEnd(0xfffffff0u, 32) != 0xffffffffu) {
        throw std::runtime_error("Incorrect timeline transition or saturated endpoint");
    }
    for (const auto rate : {22050u, 44100u, 48000u, 96000u}) {
        TimelineClock whole(rate), split(rate);
        whole.Advance(rate * 17 + 13);
        for (unsigned index = 0; index < rate * 17 + 13; ++index) split.Advance(1);
        if (whole.GetTick() != split.GetTick() || whole.GetMilliseconds() != split.GetMilliseconds()) {
            throw std::runtime_error("Timeline time depends on render block size");
        }
        const auto target = whole.GetTick() + 127;
        const auto distance = whole.FramesUntil(target);
        whole.Advance(distance - 1);
        if (whole.GetTick() >= target) throw std::runtime_error("Timeline scheduled a boundary too late");
        whole.Advance(1);
        if (whole.GetTick() < target) throw std::runtime_error("Timeline scheduled a boundary too early");
    }
    TempoGrid standard({});
    if (standard.Quantize(1, QuantizationMode::Bars, 1) != 96000
        || standard.Quantize(12000, QuantizationMode::EighthNotes, 1) != 12000
        || standard.Quantize(12001, QuantizationMode::EighthNotes, 1) != 24000) {
        throw std::runtime_error("Incorrect musical quantization");
    }
    TempoGrid changed({{{}, 4, 4, 24000, 120}, {{}, 3, 4, 60000, 60}});
    if (changed.Quantize(1, QuantizationMode::EighthNotes, 1) != 12000
        || changed.Quantize(24001, QuantizationMode::Bars, 1) != 60000
        || changed.Quantize(60001, QuantizationMode::EighthNotes, 1) != 84000) {
        throw std::runtime_error("Tempo change lost its grid anchor");
    }
    std::string bytes(16, '\0');
    Append(bytes, 1, 2);
    Append(bytes, 3, 2);
    Append(bytes, 24, 2);
    bytes.append(16, '\1');
    Append(bytes, 123, 4);
    Append(bytes, 456, 4);
    Append(bytes, 2, 2);
    Append(bytes, 8, 2);
    Append(bytes, 400, 4);
    Append(bytes, 0, 4);
    Append(bytes, 2, 2);
    Append(bytes, 30, 2);
    bytes.append(16, '\2');
    Append(bytes, 500, 4);
    Append(bytes, 4, 2);
    bytes += "test";
    Append(bytes, 0, 4);
    Append(bytes, 3, 2);
    Append(bytes, 32, 2);
    bytes.append(16, '\3');
    Append(bytes, 3, 4);
    Append(bytes, 8, 4);
    Append(bytes, 700, 4);
    AppendFloat(bytes, 96);
    const auto model = OpenFmod::Studio::ReadTimeline(bytes);
    if (!model.Triggered.empty() || model.TimeLocked.size() != 1 || model.TimeLocked[0].Start != 123
        || model.TimeLocked[0].Duration != 456 || model.Markers[0].Name != "test"
        || model.Tempo[0].BeatsPerMinute != 96 || !model.SustainPoints[0].Trigger.Evaluate({}, false)) {
        throw std::runtime_error("Incorrect timeline model");
    }
    std::string transition(32, '\0');
    Append(transition, 100, 4);
    Append(transition, 900, 4);
    Append(transition, 0, 4);
    Append(transition, 1, 4);
    Append(transition, 2, 4);
    AppendFloat(transition, 75);
    Append(transition, 4, 4);
    const auto region = OpenFmod::Studio::ReadTransition(transition);
    if (region.Start != 100 || region.End != 900 || region.QuantizationMode != 1
        || region.QuantizationInterval != 2 || region.Probability != 75 || region.Flags != 4) {
        throw std::runtime_error("Incorrect transition region");
    }
    bytes.pop_back();
    try { OpenFmod::Studio::ReadTimeline(bytes); }
    catch (const OpenFmod::FormatError&) { return; }
    throw std::runtime_error("Truncated timeline accepted");
}
