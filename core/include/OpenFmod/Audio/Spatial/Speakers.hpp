#ifndef OPENFMOD_AUDIO_SPATIAL_SPEAKERS_HPP
#define OPENFMOD_AUDIO_SPATIAL_SPEAKERS_HPP
#include "OpenFmod/Audio/Spatial/Layout.hpp"
namespace OpenFmod::Audio {
enum class SpeakerMode { Mono = 2, Stereo, Quad, Surround, Surround51, Surround71 };
class Speakers {
public:
    explicit Speakers(SpeakerMode mode);
    std::size_t GetChannels() const;
    std::vector<float> Project(float direction, float extent, bool normalized = true) const;
    std::vector<float> ProjectStereo(float direction, float extent, float rotation) const;
    std::vector<float> StereoDownmix() const;
private:
    std::vector<float> projectArc(float begin, float end, float direction, float scale) const;
    SpeakerMode mode;
    std::optional<PlanarPanner> planar;
    std::size_t channels;
};
}
#endif
