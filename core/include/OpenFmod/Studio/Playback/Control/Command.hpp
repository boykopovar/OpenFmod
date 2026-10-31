#ifndef OPENFMOD_STUDIO_PLAYBACK_COMMAND_HPP
#define OPENFMOD_STUDIO_PLAYBACK_COMMAND_HPP
#include "OpenFmod/Studio/Playback/Control/IEventControl.hpp"
#include "OpenFmod/Studio/Playback/Controls.hpp"
#include "OpenFmod/Studio/Playback/IPlayback.hpp"
namespace OpenFmod::Studio {
void ExecuteCommand(const CommandInstrument& command, float value, IEventControl& target);
class CommandPlayback final : public IPlayback {
public:
    CommandPlayback(const Library& library, const Bank::Guid& instrument, const AutomationBindings& bindings, const std::function<float()>& random, IEventControl& target);
    void Start(const SoundStart& start, const AutomationInput& input) override;
    void Update(const AutomationInput& input, double milliseconds, float parentGain = 1, float parentPitch = 1) override;
    void Stop(bool immediate, double milliseconds) override;
    void End(bool cutOff, double milliseconds) override;
    bool IsPlaying() const override;
private:
    std::shared_ptr<const Instrument> model;
    InstrumentControls controls;
    IEventControl& target;
    bool started{};
};
}
#endif
