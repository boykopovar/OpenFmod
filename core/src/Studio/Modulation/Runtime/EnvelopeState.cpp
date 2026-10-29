#include "OpenFmod/Studio/Modulation/Runtime/EnvelopeState.hpp"
#include "OpenFmod/Studio/Modulation/Curve.hpp"
#include <cmath>
namespace OpenFmod::Studio {
namespace {
void validateTime(double milliseconds) {
    if (!std::isfinite(milliseconds) || milliseconds < 0) throw FormatError("Invalid envelope time");
}
}
EnvelopeState::EnvelopeState(Envelope envelope) : envelope(envelope) {
    for (const auto value : {envelope.Initial, envelope.Peak, envelope.Sustain, envelope.Final}) {
        if (!std::isfinite(value)) throw FormatError("Nonfinite envelope level");
    }
    for (const auto duration : {envelope.AttackMilliseconds, envelope.HoldMilliseconds,
        envelope.DecayMilliseconds, envelope.ReleaseMilliseconds}) validateTime(duration);
    for (const auto curvature : envelope.Curvature) {
        if (!std::isfinite(curvature)) throw FormatError("Nonfinite envelope curvature");
    }
}
float EnvelopeState::evaluateHeld(double milliseconds) const {
    if (milliseconds < envelope.AttackMilliseconds) {
        return Interpolate(envelope.Initial, envelope.Peak,
            static_cast<float>(milliseconds / envelope.AttackMilliseconds), envelope.Curvature[0], Interpolation::Exponential);
    }
    milliseconds -= envelope.AttackMilliseconds;
    if (milliseconds < envelope.HoldMilliseconds) return envelope.Peak;
    milliseconds -= envelope.HoldMilliseconds;
    if (milliseconds < envelope.DecayMilliseconds) {
        return Interpolate(envelope.Peak, envelope.Sustain,
            static_cast<float>(milliseconds / envelope.DecayMilliseconds), envelope.Curvature[1], Interpolation::Exponential);
    }
    return envelope.Sustain;
}
float EnvelopeState::Evaluate(double milliseconds) const {
    validateTime(milliseconds);
    if (!released || milliseconds < *released) return evaluateHeld(milliseconds);
    const auto elapsed = milliseconds - *released;
    if (elapsed >= envelope.ReleaseMilliseconds) return envelope.Final;
    return Interpolate(releaseValue, envelope.Final,
        static_cast<float>(elapsed / envelope.ReleaseMilliseconds), envelope.Curvature[2], Interpolation::Exponential);
}
void EnvelopeState::Release(double milliseconds) {
    validateTime(milliseconds);
    if (released) throw FormatError("Envelope has already been released");
    releaseValue = evaluateHeld(milliseconds);
    released = milliseconds;
}
bool EnvelopeState::IsFinished(double milliseconds) const {
    validateTime(milliseconds);
    return released && milliseconds >= *released + envelope.ReleaseMilliseconds;
}
}
