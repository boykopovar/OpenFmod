#include "OpenFmod/Studio/Event/Model.hpp"
#include "../Data/Writer.hpp"
#include <stdexcept>
void CheckEvent() {
    using namespace TestData;
    std::string bytes(80, '\0');
    Append(bytes, 3, 4);
    Append(bytes, 128, 4);
    Append(bytes, 0, 1);
    Append(bytes, 1, 4);
    Append(bytes, 1, 2);
    Append(bytes, 2, 2);
    Append(bytes, 10, 2);
    Append(bytes, 4, 2);
    bytes += "test";
    AppendFloat(bytes, 12.5f);
    Append(bytes, 0, 2);
    AppendFloat(bytes, 50);
    Append(bytes, 2, 4);
    AppendFloat(bytes, 0.125f);
    Append(bytes, 1, 4);
    Append(bytes, 1, 2);
    Append(bytes, 3, 2);
    Append(bytes, 8, 2);
    Append(bytes, 123, 4);
    Append(bytes, 456, 4);
    Append(bytes, 1, 2);
    AppendFloat(bytes, 2);
    AppendFloat(bytes, 30);
    const auto event = OpenFmod::Studio::ReadEvent(bytes);
    if (event.MaximumInstances != 3 || event.DopplerPercent != 50 || event.CooldownSeconds != 0.125f
        || event.NumericProperties.at("test") != 12.5f || event.ParameterIds.at(0) != (std::uint64_t{456} << 32 | 123)
        || event.MinimumDistance != 2 || event.MaximumDistance != 30) throw std::runtime_error("Incorrect event model");
    bytes.pop_back();
    try { OpenFmod::Studio::ReadEvent(bytes); }
    catch (const OpenFmod::FormatError&) { return; }
    throw std::runtime_error("Truncated event accepted");
}
