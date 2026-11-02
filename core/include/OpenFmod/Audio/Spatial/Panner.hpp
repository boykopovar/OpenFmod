#ifndef OPENFMOD_AUDIO_SPATIAL_PANNER_HPP
#define OPENFMOD_AUDIO_SPATIAL_PANNER_HPP
#include "OpenFmod/Audio/Processing/IAudioProcessor.hpp"
#include "OpenFmod/Audio/Spatial/Speakers.hpp"
#include "OpenFmod/Audio/Spatial/Projection.hpp"
#include <array>
#include <cstdint>
namespace OpenFmod::Audio {
struct SpatialListener {
    float Weight{1};
    float Direction{};
    float Rotation{};
    float Distance{};
    float HorizontalDistance{};
};
struct PannerSettings {
    std::int32_t Mode{1};
    float Balance{};
    float Direction{};
    float Extent{360};
    float Rotation{};
    float LowFrequencyDecibels{};
    bool Discrete{true};
    float Separation{60};
    float Axis{};
    std::uint32_t EnabledSpeakers{4095};
    Rolloff Attenuation{Rolloff::LinearSquared};
    float MinimumDistance{1};
    float MaximumDistance{20};
    ExtentMode ExtentControl{ExtentMode::Automatic};
    float SoundSize{};
    float MinimumExtent{};
    float SpatialBlend{};
    bool LowFrequencyUpmix{};
    SpeakerMode Layout{SpeakerMode::Stereo};
    float HeightBlend{};
    std::vector<SpatialListener> Listeners{SpatialListener{}};
};
std::array<float, 4> PannerMatrix(const PannerSettings& settings);
class Panner final : public IAudioProcessor {
public:
    explicit Panner(const PannerSettings& settings);
    void Configure(const PannerSettings& settings, std::uint32_t rampFrames = 64);
    void Process(float* stereo, std::size_t frames) override;
private:
    std::array<float, 4> matrix;
    std::array<float, 4> target;
    std::array<float, 4> increment{};
    std::uint32_t remaining{};
};
}
#endif
