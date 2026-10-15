#include "OpenFmod/Studio/Parameter/Model.hpp"
#include "../Data/Writer.hpp"
#include <stdexcept>
#include "OpenFmod/Studio/Modulation/SlewState.hpp"
void CheckParameter() {
    using namespace TestData;
    OpenFmod::Studio::SlewState slew(2, 1);
    slew.Reset(0);
    if (slew.Process(10, 1) != 2 || slew.Process(0, 1) != 1 || slew.Process(0, 4) != 0) {
        throw std::runtime_error("Incorrect directional slew speed");
    }
    OpenFmod::Studio::SlewState discrete(1, 1, nullptr, true);
    discrete.Reset(0);
    if (discrete.Process(2, 0.5) != 0 || discrete.Process(2, 0.5) != 1
        || discrete.Process(0, 0.5) != 1 || discrete.Process(0, 0.5) != 0) {
        throw std::runtime_error("Slew lost fractional progress");
    }
    OpenFmod::Studio::Mapping mapping({}, {{0, 0}, {0.5f, 2}, {1, 10}});
    OpenFmod::Studio::SlewState mapped(0.5f, 1, &mapping);
    mapped.Reset(0);
    if (mapped.Process(10, 1) != 2 || mapped.Process(10, 1) != 10) {
        throw std::runtime_error("Slew ignored property mapping");
    }
    std::string bytes(16, '\0');
    Append(bytes, 4, 4);
    Append(bytes, 0, 4);
    Append(bytes, 4, 2);
    bytes += "wind";
    for (float value : {0.f, 10.f, 2.f, 0.f, 3.f, 1.f}) AppendFloat(bytes, value);
    Append(bytes, 0, 2);
    OpenFmod::Bank::Chunk chunk{"LIST", "PARM", 0, bytes.size(), {
        {"PRMB", {}, 0, bytes.size(), {}}, {"LIST", "PRPS", 0, 0, {}}}};
    const auto model = OpenFmod::Studio::ReadParameter(bytes, chunk, 142);
    if (model.Name != "wind" || model.Default != 2 || model.SeekSpeed != 3 || model.DescendingSeekSpeed != 1) {
        throw std::runtime_error("Incorrect parameter model");
    }
    try { OpenFmod::Studio::ReadParameter(bytes, chunk, 143); }
    catch (const OpenFmod::FormatError&) { return; }
    throw std::runtime_error("Parameter version mismatch accepted");
}
