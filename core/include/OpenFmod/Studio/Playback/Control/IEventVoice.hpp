#ifndef OPENFMOD_STUDIO_PLAYBACK_CONTROL_IEVENTVOICE_HPP
#define OPENFMOD_STUDIO_PLAYBACK_CONTROL_IEVENTVOICE_HPP
#include "OpenFmod/Studio/Playback/IPlayback.hpp"
#include "OpenFmod/Bank/Identity.hpp"
#include <functional>
#include <memory>
namespace OpenFmod::Studio {
class IEventVoice {
public:
    virtual ~IEventVoice() = default;
    virtual void Start(const SoundStart& position) = 0;
    virtual void Stop(bool immediate) = 0;
    virtual void End(bool cutOff) = 0;
    virtual bool IsPlaying() const = 0;
    virtual void SetMix(float gain, float pitch) = 0;
    virtual void SetParameter(const Bank::Guid& parameter, float value) = 0;
    virtual void SetSnapshotIntensity(float percent) = 0;
};
using EventVoiceFactory = std::function<std::unique_ptr<IEventVoice>(const Bank::Guid&)>;
}
#endif
