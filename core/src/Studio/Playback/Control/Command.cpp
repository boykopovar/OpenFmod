#include "OpenFmod/Studio/Playback/Control/Command.hpp"
#include <cmath>
#include <algorithm>
namespace OpenFmod::Studio {
void ExecuteCommand(const CommandInstrument& command, float value, IEventControl& target) {
    if (!std::isfinite(value)) throw FormatError("Nonfinite command value");
    if (command.Type == 2 || command.Type == 3) {
        const auto& parameter = target.GetParameterModel(command.Target);
        if ((parameter.Flags & 8) != 0) {
            const auto maximum = command.Type == 3 ? parameter.Maximum - parameter.Minimum : parameter.Maximum;
            const auto minimum = command.Type == 3 ? -maximum : parameter.Minimum;
            if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum > maximum) throw FormatError("Invalid command parameter range");
            if (minimum == maximum) value = maximum;
            else {
                const auto mapped = (value - minimum) / (maximum - minimum) * (maximum - minimum + 1) + minimum;
                if (!std::isfinite(mapped)) throw FormatError("Command parameter mapping overflow");
                value = std::min(std::floor(mapped), maximum);
            }
        }
    }
    switch (command.Type) {
    case 0: target.StopEvents(command.Target, false); break;
    case 1: target.StopEvents(command.Target, true); break;
    case 2: target.SetParameter(command.Target, value); break;
    case 3: {
        const auto result = target.GetParameter(command.Target) + value;
        if (!std::isfinite(result)) throw FormatError("Command parameter increment overflow");
        target.SetParameter(command.Target, result);
        break;
    }
    case 4: target.StartEvent(command.Target); break;
    default: throw FormatError("Unknown event command type");
    }
}
CommandPlayback::CommandPlayback(const Library& library, const Bank::Guid& instrument, const AutomationBindings& bindings, const std::function<float()>& random, IEventControl& target)
    : model(library.Retain<Instrument>(instrument)), controls(library, *model, bindings, random), target(target) {
    if (!std::holds_alternative<CommandInstrument>(model->Definition.Source)) throw FormatError("Command playback requires a command instrument");
}
void CommandPlayback::Start(const SoundStart& start, const AutomationInput& input) {
    if (started) throw FormatError("Command playback has already started");
    if (start.Repeats < -1 || !std::isfinite(start.OffsetPercent) || start.OffsetPercent < 0 || start.OffsetPercent > 100) throw FormatError("Invalid command playback start");
    controls.Update(input, 0);
    started = true;
    ExecuteCommand(std::get<CommandInstrument>(model->Definition.Source), controls.Get(2), target);
}
void CommandPlayback::Update(const AutomationInput&, double milliseconds, float parentGain, float parentPitch) {
    if (!started) throw FormatError("Command playback has not started");
    if (!std::isfinite(milliseconds) || milliseconds < 0 || !std::isfinite(parentGain) || parentGain < 0
        || !std::isfinite(parentPitch) || parentPitch < 0) throw FormatError("Invalid command playback update");
}
void CommandPlayback::Stop(bool, double milliseconds) { Update({}, milliseconds); }
bool CommandPlayback::IsPlaying() const { return false; }
}
