#include "OpenFmod/Sound/Metadata.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <cmath>
namespace OpenFmod::Sound {
void ReadMetadata(Sample& sample, std::uint32_t type, std::string_view bytes) {
    Binary::Reader reader(bytes);
    switch (type) {
    case 1:
        sample.Channels = static_cast<std::uint32_t>(reader.Unsigned(1));
        break;
    case 2:
        sample.Rate = static_cast<std::uint32_t>(reader.Unsigned(4));
        break;
    case 3:
        sample.Repeat = Loop{static_cast<std::uint32_t>(reader.Unsigned(4)), static_cast<std::uint32_t>(reader.Unsigned(4))};
        if (sample.Repeat->Begin > sample.Repeat->End || sample.Repeat->End >= sample.Frames) {
            throw FormatError("Invalid sample loop range");
        }
        break;
    case 4:
        while (reader.Remaining()) {
            Cue cue;
            cue.Frame = static_cast<std::uint32_t>(reader.Unsigned(4));
            const auto name = reader.Read(256);
            const auto end = name.find('\0');
            if (end == std::string_view::npos) throw FormatError("Unterminated sample cue name");
            cue.Name = std::string(name.substr(0, end));
            if (cue.Frame > sample.Frames) throw FormatError("Sample cue outside sample range");
            sample.Cues.push_back(std::move(cue));
        }
        break;
    case 11: {
        sample.VorbisSetup = static_cast<std::uint32_t>(reader.Unsigned(4));
        const auto size = reader.Unsigned(4);
        if (size != reader.Remaining() || size % 8) throw FormatError("Invalid Vorbis seek table size");
        while (reader.Remaining()) {
            sample.SeekPoints.push_back({static_cast<std::uint32_t>(reader.Unsigned(4)), static_cast<std::uint32_t>(reader.Unsigned(4))});
        }
        break;
    }
    case 13:
        sample.Peak = reader.Float();
        if (!std::isfinite(sample.Peak) || sample.Peak < 0) throw FormatError("Invalid sample peak");
        break;
    default:
        throw FormatError("Unsupported sample metadata type: " + std::to_string(type));
    }
    reader.Finish();
}
}
