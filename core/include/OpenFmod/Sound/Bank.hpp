#ifndef OPENFMOD_SOUND_BANK_HPP
#define OPENFMOD_SOUND_BANK_HPP
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
namespace OpenFmod::Sound {
struct Loop {
    std::uint32_t Begin;
    std::uint32_t End;
};
struct Cue {
    std::uint32_t Frame;
    std::string Name;
};
struct SeekPoint {
    std::uint32_t Frame;
    std::uint32_t Offset;
};
struct Sample {
    std::string Name;
    std::uint32_t Rate{};
    std::uint32_t Channels{};
    std::uint32_t Frames{};
    float Peak = 1;
    std::optional<Loop> Repeat;
    std::optional<std::uint32_t> VorbisSetup;
    std::vector<Cue> Cues;
    std::vector<SeekPoint> SeekPoints;
    std::string_view Data;
};
struct Bank {
    std::uint32_t Codec{};
    std::uint32_t DataVersion{};
    std::vector<Sample> Samples;
};
Bank ReadBank(std::string_view bytes);
}
#endif
