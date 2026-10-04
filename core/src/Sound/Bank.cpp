#include "OpenFmod/Sound/Bank.hpp"
#include "OpenFmod/Sound/Metadata.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <array>
namespace OpenFmod::Sound {
Bank ReadBank(std::string_view bytes) {
    Binary::Reader reader(bytes);
    if (reader.Read(4) != "FSB5") throw FormatError("Expected FSB5 sound bank");
    if (reader.Unsigned(4) != 1) throw FormatError("Unsupported sound bank version");
    const auto count = reader.Unsigned(4);
    const auto headerSize = reader.Unsigned(4);
    const auto nameSize = reader.Unsigned(4);
    const auto dataSize = reader.Unsigned(4);
    Bank result;
    result.Codec = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.DataVersion = static_cast<std::uint32_t>(reader.Unsigned(4));
    if (reader.Unsigned(4)) throw FormatError("Unsupported sound bank flags");
    reader.Read(24);
    if (result.Codec != 15 || result.DataVersion != 1) throw FormatError("Unsupported sound bank codec or data version");
    if (!count || count > headerSize / 8 || !dataSize) throw FormatError("Invalid sound bank section sizes");
    Binary::Reader headers(reader.Read(static_cast<std::size_t>(headerSize)));
    const auto names = reader.Read(static_cast<std::size_t>(nameSize));
    const auto data = reader.Read(static_cast<std::size_t>(dataSize));
    reader.Finish();
    constexpr std::array<std::uint32_t, 11> rates{4000, 8000, 11000, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 96000};
    constexpr std::array<std::uint32_t, 4> channels{1, 2, 6, 8};
    std::vector<std::size_t> offsets;
    result.Samples.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t index = 0; index < count; ++index) {
        const auto bits = headers.Unsigned(8);
        const auto rate = (bits >> 1) & 15;
        if (rate >= rates.size()) throw FormatError("Invalid sample rate index");
        Sample sample;
        sample.Rate = rates[rate];
        sample.Channels = channels[(bits >> 5) & 3];
        sample.Frames = static_cast<std::uint32_t>(bits >> 34);
        offsets.push_back(static_cast<std::size_t>(((bits >> 7) & 0x7ffffff) * 32));
        bool more = (bits & 1) != 0;
        std::array<bool, 128> seen{};
        while (more) {
            const auto metadata = headers.Unsigned(4);
            more = (metadata & 1) != 0;
            const auto type = static_cast<std::uint32_t>(metadata >> 25);
            if (seen[type]) throw FormatError("Duplicate sample metadata");
            seen[type] = true;
            ReadMetadata(sample, type, headers.Read(static_cast<std::size_t>((metadata >> 1) & 0xffffff)));
        }
        if (!sample.Rate || !sample.Channels || !sample.VorbisSetup) throw FormatError("Missing sample decoding parameters");
        result.Samples.push_back(std::move(sample));
    }
    headers.Finish();
    offsets.push_back(data.size());
    Binary::Reader nameOffsets(names);
    if (!names.empty() && count > names.size() / 4) throw FormatError("Truncated sample name table");
    for (std::size_t index = 0; index < result.Samples.size(); ++index) {
        auto& sample = result.Samples[index];
        if (offsets[index] > offsets[index + 1] || offsets[index + 1] > data.size()) throw FormatError("Invalid sample data range");
        sample.Data = data.substr(offsets[index], offsets[index + 1] - offsets[index]);
        for (const auto& point : sample.SeekPoints) {
            if (point.Frame > sample.Frames || point.Offset > sample.Data.size()) throw FormatError("Invalid sample seek point");
        }
        if (!names.empty()) {
            const auto offset = static_cast<std::size_t>(nameOffsets.Unsigned(4));
            if (offset < count * 4 || offset >= names.size()) throw FormatError("Invalid sample name offset");
            const auto end = names.find('\0', offset);
            if (end == std::string_view::npos) throw FormatError("Unterminated sample name");
            sample.Name = std::string(names.substr(offset, end - offset));
        }
    }
    return result;
}
}
