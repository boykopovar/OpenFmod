#ifndef OPENFMOD_AUDIO_VOICE_HPP
#define OPENFMOD_AUDIO_VOICE_HPP
#include "OpenFmod/Audio/IAudioSource.hpp"
#include "OpenFmod/Audio/Spatial/MixMatrix.hpp"
#include <array>
#include <memory>
#include <optional>
namespace OpenFmod::Audio {
struct VoiceSettings {
    float Gain{1};
    float Pitch{1};
    std::uint32_t GainRampFrames{};
    std::uint32_t FadeInFrames{};
    std::uint64_t DelayFrames{};
    std::uint64_t StartFrame{};
    std::uint64_t LoopBegin{};
    std::uint64_t LoopEnd{};
    std::int32_t Repeats{};
    std::optional<std::uint64_t> EndFrame;
};
struct VoiceProgress {
    std::size_t Frames{};
    bool Playing{};
};
class Voice {
public:
    Voice(std::unique_ptr<IAudioSource> source, VoiceSettings settings, std::uint32_t outputRate, SpeakerMode outputLayout = SpeakerMode::Stereo);
    VoiceProgress Mix(float* stereo, std::size_t frames, float masterGain, float masterPitch = 1);
    void SetGain(float value);
    void SetPitch(float value);
    void Finish();
    std::optional<std::uint64_t> FramesUntilEnd(float masterPitch = 1) const;
private:
    struct CachedFrame {
        std::uint64_t Position{};
        std::array<float, 8> Value{};
        bool Valid{};
    };
    std::array<float, 8> readFrame(std::uint64_t frame);
    double rateStep(float masterPitch) const;
    void advanceLoop();
    std::unique_ptr<IAudioSource> source;
    VoiceSettings settings;
    Format format;
    std::optional<MixMatrix> matrix;
    std::size_t outputChannels{};
    double rateRatio;
    double position{};
    float currentGain{};
    float gainIncrement{};
    std::uint32_t gainRemaining{};
    std::uint32_t fadePosition{};
    std::uint64_t endFrame{};
    std::array<float, 4096> buffer{};
    std::uint64_t bufferBegin{};
    std::size_t buffered{};
    std::array<CachedFrame, 2> recent{};
    std::size_t recentIndex{};
};
}
#endif
