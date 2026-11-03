#include "OpenFmod/Studio/Modulation/Runtime/Instance.hpp"
#include <cmath>
namespace OpenFmod::Studio {
ModulationInstance::ModulationInstance(Modulation model, std::shared_ptr<const Mapping> mapping, bool discrete, bool angular, float random)
    : model(std::move(model)), mapping(std::move(mapping)), state(create(discrete, angular, random)) {}
ModulationInstance::State ModulationInstance::create(bool discrete, bool angular, float random) const {
    if (model.ValueMode > 1) throw FormatError("Invalid continuous modulation value mode");
    if (const auto* envelope = std::get_if<Envelope>(&model.Parameters)) {
        auto configured = *envelope;
        if (model.ValueMode == 1 && mapping) {
            configured.Initial = mapping->Inverse(configured.Initial);
            configured.Peak = mapping->Inverse(configured.Peak);
            configured.Sustain = mapping->Inverse(configured.Sustain);
            configured.Final = mapping->Inverse(configured.Final);
        }
        return EnvelopeState(configured);
    }
    if (const auto* parameters = std::get_if<RandomModulation>(&model.Parameters)) {
        RandomState result(parameters->Amount, model.ValueMode == 0 ? RandomRange::Symmetric : RandomRange::Negative,
            mapping.get(), discrete, angular);
        result.Start(random);
        return result;
    }
    return FollowerState(std::get<EnvelopeFollower>(model.Parameters));
}
ModulationValue ModulationInstance::Evaluate(double milliseconds) const {
    if (!std::isfinite(milliseconds) || milliseconds < 0) throw FormatError("Invalid modulation time");
    if (const auto* envelope = std::get_if<EnvelopeState>(&state)) return {envelope->Evaluate(milliseconds), true};
    if (const auto* random = std::get_if<RandomState>(&state)) return {random->GetValue(), false};
    const auto amount = std::get<EnvelopeFollower>(model.Parameters).Amount;
    float minimum = 0;
    float maximum = amount;
    if (model.ValueMode == 1) {
        minimum = mapping ? mapping->Inverse(0) : 0;
        maximum = mapping ? mapping->Inverse(-amount) : -amount;
    }
    return {minimum + (maximum - minimum) * std::get<FollowerState>(state).GetLevel(), false};
}
void ModulationInstance::ProcessAudio(const float* samples, std::size_t count, double milliseconds) {
    auto* follower = std::get_if<FollowerState>(&state);
    if (!follower) throw FormatError("Audio input requires an envelope follower");
    follower->Process(samples, count, milliseconds);
}
void ModulationInstance::ProcessMeter(float peak, float energy, double milliseconds) {
    auto* follower = std::get_if<FollowerState>(&state);
    if (!follower) throw FormatError("Meter input requires an envelope follower");
    if (!std::isfinite(peak) || peak < 0 || !std::isfinite(energy) || energy < 0) throw FormatError("Invalid modulation meter");
    follower->ProcessLevel(std::get<EnvelopeFollower>(model.Parameters).RootMeanSquare ? std::sqrt(energy) : peak, milliseconds);
}
void ModulationInstance::Release(double milliseconds) {
    if (!std::isfinite(milliseconds) || milliseconds < 0) throw FormatError("Invalid modulation release time");
    if (released) throw FormatError("Modulation instance has already been released");
    if (auto* envelope = std::get_if<EnvelopeState>(&state)) envelope->Release(milliseconds);
    released = true;
}
bool ModulationInstance::IsFinished(double milliseconds) const {
    if (!std::isfinite(milliseconds) || milliseconds < 0) throw FormatError("Invalid modulation time");
    if (const auto* envelope = std::get_if<EnvelopeState>(&state)) return envelope->IsFinished(milliseconds);
    return released;
}
}
