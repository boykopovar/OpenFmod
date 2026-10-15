#include "OpenFmod/Studio/Effect/Model.hpp"
#include "../Data/Writer.hpp"
#include <stdexcept>
void CheckEffect() {
    using namespace TestData;
    std::string bytes(16, '\0');
    Append(bytes, 18, 4);
    Append(bytes, 0, 1);
    const auto offset = bytes.size();
    Append(bytes, 4, 4);
    Append(bytes, 0, 4);
    AppendFloat(bytes, -12);
    Append(bytes, 1, 4);
    Append(bytes, 0xfffffffe, 4);
    Append(bytes, 2, 4);
    Append(bytes, 1, 1);
    Append(bytes, 3, 4);
    Append(bytes, 3, 2);
    bytes.append("abc", 3);
    Append(bytes, 1, 1);
    OpenFmod::Bank::Chunk chunk{"LIST", "BEFF", 0, bytes.size(), {
        {"BEFB", {}, 0, offset, {}}, {"PMEF", {}, offset, bytes.size() - offset, {}}, {"LIST", "PRPS", 0, 0, {}}}};
    const auto effect = OpenFmod::Studio::ReadEffect(bytes, chunk);
    if (effect.Type != 18 || std::get<float>(effect.Parameters[0]) != -12
        || std::get<std::int32_t>(effect.Parameters[1]) != -2 || !std::get<bool>(effect.Parameters[2])
        || std::get<std::string>(effect.Parameters[3]) != "abc" || !effect.ParameterFlag) {
        throw std::runtime_error("Incorrect effect parameter decoding");
    }
    bytes.back() = 2;
    try { OpenFmod::Studio::ReadEffect(bytes, chunk); }
    catch (const OpenFmod::FormatError&) { return; }
    throw std::runtime_error("Invalid effect flag accepted");
}
