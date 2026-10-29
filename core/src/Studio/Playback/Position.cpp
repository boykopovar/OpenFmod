#include "OpenFmod/Studio/Playback/Sound.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace OpenFmod::Studio {
Audio::VoiceSettings SoundPosition(const Sound::Sample& sample, const SoundStart& start) {
    if (!sample.Rate || !sample.Frames || start.Repeats < -1 || !std::isfinite(start.OffsetPercent)
        || start.OffsetPercent < 0 || start.OffsetPercent > 100) throw FormatError("Invalid sound playback position");
    Audio::VoiceSettings result;
    result.Repeats = start.Repeats;
    result.LoopBegin = sample.Repeat ? sample.Repeat->Begin : 0;
    result.LoopEnd = sample.Repeat ? std::uint64_t{sample.Repeat->End} + 1 : sample.Frames;
    if (result.LoopBegin >= result.LoopEnd || result.LoopEnd > sample.Frames) throw FormatError("Invalid sample loop points");
    const auto offset = static_cast<long double>(start.OffsetTicks) * sample.Rate / 48000
        + static_cast<long double>(sample.Frames) * start.OffsetPercent / 100;
    if (offset > std::numeric_limits<std::uint64_t>::max()) throw FormatError("Sound offset overflow");
    auto frame = static_cast<std::uint64_t>(offset);
    if (result.Repeats && frame > result.LoopBegin) {
        const auto length = result.LoopEnd - result.LoopBegin;
        const auto relative = frame - result.LoopBegin;
        const auto cycles = relative / length;
        if (result.Repeats == -1 || cycles <= static_cast<std::uint64_t>(result.Repeats)) {
            frame = result.LoopBegin + relative % length;
            if (result.Repeats != -1) result.Repeats -= static_cast<std::int32_t>(cycles);
        } else {
            frame -= length * static_cast<std::uint32_t>(result.Repeats);
            result.Repeats = 0;
        }
    }
    result.StartFrame = std::min<std::uint64_t>(frame, sample.Frames - 1);
    return result;
}
}
