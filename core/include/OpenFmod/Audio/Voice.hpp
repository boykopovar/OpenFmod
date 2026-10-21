#ifndef OPENFMOD_AUDIO_VOICE_HPP
#define OPENFMOD_AUDIO_VOICE_HPP
#include "OpenFmod/Audio/IAudioSource.hpp"
#include <array>
#include <memory>
namespace OpenFmod::Audio {
struct VoiceSettings {
    float Gain{1};
    float Pitch{1};
    std::uint64_t DelayFrames{};
    std::uint64_t StartFrame{};
    std::uint64_t LoopBegin{};
    std::uint64_t LoopEnd{};
    std::int32_t Repeats{};
};
class Voice {
public:
    Voice(std::unique_ptr<IAudioSource> source, VoiceSettings settings, std::uint32_t outputRate);
    bool Mix(float* stereo, std::size_t frames, float masterGain);
    void SetGain(float value);
    void SetPitch(float value);
private:
    struct CachedFrame {
        std::uint64_t Position{};
        std::array<float, 2> Value{};
        bool Valid{};
    };
    std::array<float, 2> readFrame(std::uint64_t frame);
    std::unique_ptr<IAudioSource> source;
    VoiceSettings settings;
    Format format;
    double rateRatio;
    double position{};
    std::array<float, 1024> buffer{};
    std::uint64_t bufferBegin{};
    std::size_t buffered{};
    std::array<CachedFrame, 2> recent{};
    std::size_t recentIndex{};
};
}
#endif
