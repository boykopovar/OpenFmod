#include "OpenFmod/Studio/Instrument/Settings.hpp"
#include "OpenFmod/Studio/Instrument/Definition.hpp"
#include "../Data/Writer.hpp"
#include <limits>
#include <stdexcept>
void CheckInstrument() {
    using namespace TestData;
    std::string nested(32, '\0');
    AppendFloat(nested, 120);
    Append(nested, 3, 2);
    Append(nested, 24, 2);
    Append(nested, 2, 4);
    nested.append(16, '\1');
    AppendFloat(nested, 7);
    const auto definition = OpenFmod::Studio::ReadInstrumentDefinition("EVIB", nested);
    const auto& event = std::get<OpenFmod::Studio::EventInstrument>(definition.Source);
    if (event.TimelinePositionMilliseconds != 120 || event.Parameters.size() != 1
        || event.Parameters[0].Value != 7 || event.Parameters[0].PropertyId != 2) {
        throw std::runtime_error("Incorrect nested event definition");
    }
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
