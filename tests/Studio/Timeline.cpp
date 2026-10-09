#include "OpenFmod/Studio/Timeline/Model.hpp"
#include "../Data/Writer.hpp"
#include <stdexcept>
void CheckTimeline() {
    using namespace TestData;
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
    bytes.pop_back();
    try { OpenFmod::Studio::ReadTimeline(bytes); }
    catch (const OpenFmod::FormatError&) { return; }
    throw std::runtime_error("Truncated timeline accepted");
}
