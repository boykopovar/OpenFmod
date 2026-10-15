#include "OpenFmod/Studio/Parameter/Model.hpp"
#include "../Data/Writer.hpp"
#include <stdexcept>
void CheckParameter() {
    using namespace TestData;
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
