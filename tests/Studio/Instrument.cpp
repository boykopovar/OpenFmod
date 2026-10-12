#include "OpenFmod/Studio/Instrument/Settings.hpp"
#include "../Data/Writer.hpp"
#include <limits>
#include <stdexcept>
void CheckInstrument() {
    using namespace TestData;
    std::string bytes(16, '\0');
    AppendFloat(bytes, -6);
    AppendFloat(bytes, 3);
    Append(bytes, 0xffffffff, 4);
    Append(bytes, 1, 4);
    AppendFloat(bytes, 0);
    AppendFloat(bytes, 75);
    AppendFloat(bytes, 10);
    AppendFloat(bytes, 20);
    Append(bytes, 1, 4);
    Append(bytes, 2, 4);
    bytes.append(16, '\0');
    AppendFloat(bytes, 1);
    AppendFloat(bytes, 0);
    Append(bytes, 3, 4);
    Append(bytes, 24, 2);
    bytes.append(16, '\0');
    Append(bytes, 2, 4);
    Append(bytes, 0xffffffff, 4);
    Append(bytes, 2, 4);
    Append(bytes, 4800, 4);
    AppendFloat(bytes, 0);
    AppendFloat(bytes, 0);
    Append(bytes, 0, 4);
    const auto result = OpenFmod::Studio::ReadInstrumentSettings(bytes);
    if (result.LoopCount != -1 || result.Properties.at(8) != 75 || result.MaximumDelayMilliseconds != 20
        || result.SourceOffsetTicks != 4800 || result.Routing.ChannelLayout != 2 || result.MaximumInstances != 3) {
        throw std::runtime_error("Incorrect instrument settings");
    }
    std::string signedBytes(8, '\0');
    signedBytes.back() = static_cast<char>(0x80);
    OpenFmod::Binary::Reader reader(signedBytes);
    if (reader.Signed(8) != std::numeric_limits<std::int64_t>::min()) throw std::runtime_error("Incorrect signed integer boundary");
    bytes.pop_back();
    try { OpenFmod::Studio::ReadInstrumentSettings(bytes); }
    catch (const OpenFmod::FormatError&) { return; }
    throw std::runtime_error("Truncated instrument accepted");
}
