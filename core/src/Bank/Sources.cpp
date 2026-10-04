#include "OpenFmod/Bank/Sources.hpp"
namespace OpenFmod::Bank {
namespace {
std::string_view payload(std::string_view bytes, const Chunk& chunk) {
    if (chunk.Offset > bytes.size() || chunk.Size > bytes.size() - chunk.Offset) {
        throw FormatError("Bank chunk outside source bytes");
    }
    return bytes.substr(chunk.Offset, chunk.Size);
}
}
const Sound::Sample& Sources::Require(const Guid& id) const {
    const auto found = Waves.find(id);
    if (found == Waves.end()) throw FormatError("Unknown wave identity");
    const auto& wave = found->second;
    return Banks.at(wave.BankIndex).Samples.at(wave.SampleIndex);
}
Sources ReadSources(std::string_view bytes, const Chunk& container) {
    Sources result;
    const auto& project = container.Require("LIST", "PROJ");
    Binary::Reader ranges(payload(bytes, project.Require("SNDH")));
    if (ranges.Remaining()) {
        const auto& data = container.Require("SND ");
        std::size_t previousEnd = data.Offset;
        for (const auto element : ranges.Elements(8)) {
            Binary::Reader range(element);
            const auto offset = static_cast<std::size_t>(range.Unsigned(4));
            const auto size = static_cast<std::size_t>(range.Unsigned(4));
            if (offset < previousEnd || offset < data.Offset || offset - data.Offset > data.Size || size > data.Size - (offset - data.Offset)) {
                throw FormatError("Invalid embedded sound bank range");
            }
            if (offset > bytes.size() || size > bytes.size() - offset) throw FormatError("Embedded sound bank outside source bytes");
            result.Banks.push_back(Sound::ReadBank(bytes.substr(offset, size)));
            previousEnd = offset + size;
        }
    }
    ranges.Finish();
    const auto& collection = project.Require("LIST", "WAVS");
    if (collection.Children.empty()) return result;
    Binary::Reader count(payload(bytes, collection.Require("LCNT")));
    const auto expected = count.Unsigned(4);
    count.Finish();
    for (const auto& chunk : collection.Children) {
        if (chunk.Id == "LCNT") continue;
        if (chunk.Id != "WAV ") throw FormatError("Unexpected wave collection entry");
        Binary::Reader reader(payload(bytes, chunk));
        const auto id = ReadGuid(reader);
        const auto size = static_cast<std::size_t>(reader.Unsigned(2));
        Binary::Reader fields(reader.Read(size));
        reader.Finish();
        Wave wave;
        wave.BankIndex = static_cast<std::uint32_t>(fields.Unsigned(4));
        wave.SampleIndex = static_cast<std::uint32_t>(fields.Unsigned(4));
        const auto mode = fields.Unsigned(4);
        fields.Finish();
        if (mode > 2) throw FormatError("Unsupported wave loading mode");
        wave.Mode = static_cast<LoadingMode>(mode);
        if (wave.BankIndex >= result.Banks.size() || wave.SampleIndex >= result.Banks[wave.BankIndex].Samples.size()) {
            throw FormatError("Wave references a missing sound bank or sample");
        }
        if (!result.Waves.emplace(id, wave).second) throw FormatError("Duplicate wave identity");
    }
    if (expected != result.Waves.size()) throw FormatError("Wave collection count mismatch");
    return result;
}
}
