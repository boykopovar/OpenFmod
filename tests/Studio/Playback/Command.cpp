#include "OpenFmod/Studio/Playback/Control/Command.hpp"
#include <limits>
#include <stdexcept>
namespace {
class Target final : public OpenFmod::Studio::IEventControl {
public:
    void StartEvent(const OpenFmod::Bank::Guid& event) override { Started = event; }
    void StopEvents(const OpenFmod::Bank::Guid& event, bool immediate) override { Stopped = event; Immediate = immediate; }
    float GetParameter(const OpenFmod::Bank::Guid&) const override { return Value; }
    const OpenFmod::Studio::Parameter& GetParameterModel(const OpenFmod::Bank::Guid&) const override { return Model; }
    void SetParameter(const OpenFmod::Bank::Guid&, float value) override { Value = value; }
    OpenFmod::Studio::Parameter Model;
    OpenFmod::Bank::Guid Started{};
    OpenFmod::Bank::Guid Stopped{};
    float Value{};
    bool Immediate{};
};
}
void CheckPlaybackCommands() {
    using namespace OpenFmod::Studio;
    Target target;
    CommandInstrument command;
    command.Target[0] = 7;
    command.Type = 4;
    ExecuteCommand(command, 0, target);
    if (target.Started != command.Target) throw std::runtime_error("Command started the wrong event");
    command.Type = 1;
    ExecuteCommand(command, 0, target);
    if (!target.Immediate || target.Stopped != command.Target) throw std::runtime_error("Command lost immediate stop mode");
    command.Type = 0;
    ExecuteCommand(command, 0, target);
    if (target.Immediate) throw std::runtime_error("Command lost fading stop mode");
    command.Type = 2;
    ExecuteCommand(command, 2.9f, target);
    if (target.Value != 2.9f) throw std::runtime_error("Continuous command rounded its value");
    target.Model.Flags = 8;
    target.Model.Minimum = 0;
    target.Model.Maximum = 3;
    ExecuteCommand(command, 2.9f, target);
    if (target.Value != 3) throw std::runtime_error("Discrete command did not account for inclusive range");
    target.Value = 1;
    command.Type = 3;
    ExecuteCommand(command, 1.8f, target);
    if (target.Value != 3) throw std::runtime_error("Discrete increment used an absolute parameter range");
    target.Model.Flags = 0;
    target.Value = std::numeric_limits<float>::max();
    bool rejected = false;
    try { ExecuteCommand(command, target.Value, target); }
    catch (const OpenFmod::FormatError&) { rejected = true; }
    if (!rejected || target.Value != std::numeric_limits<float>::max()) throw std::runtime_error("Overflowing command changed its target");
    command.Type = 5;
    try { ExecuteCommand(command, 0, target); }
    catch (const OpenFmod::FormatError&) { return; }
    throw std::runtime_error("Unknown command was ignored");
}
