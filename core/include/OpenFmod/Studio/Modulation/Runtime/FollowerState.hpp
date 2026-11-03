#ifndef OPENFMOD_STUDIO_MODULATION_FOLLOWERSTATE_HPP
#define OPENFMOD_STUDIO_MODULATION_FOLLOWERSTATE_HPP
#include "OpenFmod/Studio/Modulation/Model.hpp"
namespace OpenFmod::Studio {
class FollowerState {
public:
    explicit FollowerState(EnvelopeFollower parameters);
    float Process(const float* samples, std::size_t count, double milliseconds);
    float ProcessLevel(double amplitude, double milliseconds);
    float GetLevel() const;
private:
    EnvelopeFollower parameters;
    float level{};
    float releaseStart{};
};
}
#endif
