#ifndef OPENFMOD_AUDIO_SPATIAL_LAYOUT_HPP
#define OPENFMOD_AUDIO_SPATIAL_LAYOUT_HPP
#include <cstddef>
#include <optional>
#include <vector>
namespace OpenFmod::Audio {
struct PlanarSpeaker {
    std::size_t Channel{};
    float Azimuth{};
};
struct PlanarLayout {
    std::size_t Channels{};
    std::vector<PlanarSpeaker> Speakers;
    std::optional<std::size_t> LowFrequency;
};
class PlanarPanner {
public:
    explicit PlanarPanner(PlanarLayout layout);
    std::vector<float> Project(float directionDegrees, float extentDegrees) const;
    std::vector<float> StereoDownmix() const;
private:
    void addPair(std::vector<float>& output, std::size_t index, float direction, float weight) const;
    void normalize(std::vector<float>& output) const;
    PlanarLayout layout;
};
}
#endif
