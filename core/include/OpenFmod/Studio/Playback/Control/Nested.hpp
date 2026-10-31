#ifndef OPENFMOD_STUDIO_PLAYBACK_CONTROL_NESTED_HPP
#define OPENFMOD_STUDIO_PLAYBACK_CONTROL_NESTED_HPP
#include "OpenFmod/Studio/Playback/Control/IEventVoice.hpp"
#include "OpenFmod/Studio/Playback/Controls.hpp"
namespace OpenFmod::Studio {
class NestedPlayback final : public IPlayback {
public:
    NestedPlayback(const Library& library, const Bank::Guid& instrument, const AutomationBindings& bindings, const std::function<float()>& random, const EventVoiceFactory& factory);
    void Start(const SoundStart& start, const AutomationInput& input) override;
    void Update(const AutomationInput& input, double milliseconds, float parentGain = 1, float parentPitch = 1) override;
    void Stop(bool immediate, double milliseconds) override;
    bool IsPlaying() const override;
private:
    void apply(const AutomationInput& input, float gain, float pitch);
    std::shared_ptr<const Instrument> model;
    InstrumentControls controls;
    std::unique_ptr<IEventVoice> child;
    std::map<std::uint32_t, float> previous;
    bool snapshot{};
    bool started{};
    bool releasing{};
    double lastUpdate{};
};
}
#endif
