#include "OpenFmod/Studio/Condition.hpp"
#include "Data/Writer.hpp"
#include <stdexcept>
void CheckCondition() {
    using OpenFmod::Studio::Condition;
    std::string bytes;
    TestData::Append(bytes, 0x11, 4);
    bytes.append(16, '\0');
    TestData::Append(bytes, 0x5220, 4);
    TestData::AppendFloat(bytes, 2);
    TestData::AppendFloat(bytes, 5);
    Condition range(bytes);
    for (int value = 0; value < 7; ++value) {
        if (range.Evaluate([value](const auto&) { return static_cast<float>(value); }, false) != (value >= 2 && value < 5)) {
            throw std::runtime_error("Incorrect half-open condition range");
        }
    }
    bytes.clear();
    TestData::Append(bytes, 0x103, 4);
    TestData::Append(bytes, 0x1002, 4);
    TestData::Append(bytes, 0x10, 4);
    TestData::AppendFloat(bytes, 3);
    TestData::Append(bytes, 0x3210, 4);
    TestData::AppendFloat(bytes, 2);
    TestData::Append(bytes, 0x2010, 4);
    TestData::AppendFloat(bytes, 6);
    TestData::Append(bytes, 0x30, 4);
    Condition grouped(bytes);
    if (!grouped.Evaluate({}, false) || grouped.Evaluate({}, true)) throw std::runtime_error("Incorrect grouped condition evaluation");
    bytes.resize(bytes.size() - 4);
    bool rejected = false;
    try { Condition invalid(bytes); }
    catch (const OpenFmod::FormatError&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Unclosed condition group accepted");
}
