#ifndef OPENFMOD_STUDIO_PLAYBACK_IPLAYBACK_HPP
#define OPENFMOD_STUDIO_PLAYBACK_IPLAYBACK_HPP
#include "OpenFmod/Studio/Automation/Property.hpp"
namespace OpenFmod::Studio {
struct SoundStart {
    std::uint32_t OffsetTicks{};
    float OffsetPercent{};
    std::int32_t Repeats{};
};
class IPlayback {
public:
    virtual ~IPlayback() = default;
    virtual void Start(const SoundStart& start, const AutomationInput& input) = 0;
    virtual void Update(const AutomationInput& input, double milliseconds, float parentGain = 1, float parentPitch = 1) = 0;
    virtual void Stop(bool immediate, double milliseconds) = 0;
    virtual bool IsPlaying() const = 0;
};
}
#endif
