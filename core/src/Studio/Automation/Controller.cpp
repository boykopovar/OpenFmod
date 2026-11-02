#include "OpenFmod/Studio/Automation/Controller.hpp"
#include <cmath>
#include <algorithm>
namespace OpenFmod::Studio {
Controller ReadController(std::string_view bytes) {
    Binary::Reader reader(bytes);
    Controller result;
    result.Id = Bank::ReadGuid(reader);
    result.Owner = Bank::ReadGuid(reader);
    result.CurveId = Bank::ReadGuid(reader);
    result.PropertyId = static_cast<std::uint32_t>(reader.Unsigned(4));
    reader.Finish();
    return result;
}
AutomationBlend BlendTransitionCurve(const Controller& controller, const Curve& curve, std::uint32_t position, std::uint32_t duration, std::uint32_t destinationTick) {
    if (!duration || position > duration) throw FormatError("Invalid automation transition position");
    const auto range = curve.GetTickRange();
    AutomationBlend result{TransitionBlend::FromCurrent, curve.EvaluateTick(position), 1, destinationTick};
    if (position < range.first) {
        result.Fraction = controller.PropertyId == 4 ? (std::abs(curve.EvaluateTick(range.first)) < 0.00000011920929f ? 1.0f : 0.0f)
            : static_cast<float>(position) / range.first;
    } else if (position > range.second) {
        result.Mode = TransitionBlend::FromDestination;
        result.Fraction = range.second < duration ? std::clamp(static_cast<float>(duration - position) / (duration - range.second), 0.0f, 1.0f) : 0;
    }
    return result;
}
ControllerState::ControllerState(Controller controller, const Curve& curve) : controller(controller), curve(curve) {
    if (controller.CurveId != curve.GetId()) throw FormatError("Controller curve identity mismatch");
}
const Controller& ControllerState::GetController() const { return controller; }
AutomationValue ControllerState::EvaluateParameter(float input) const {
    if (!std::isfinite(input)) throw FormatError("Nonfinite automation input");
    return {curve.Evaluate(input), curve.IsEmpty() ? 0.0f : 1.0f};
}
AutomationValue ControllerState::EvaluateTimeline(std::uint32_t tick, const AutomationBlend& blend) const {
    if (blend.Mode < TransitionBlend::None || blend.Mode > TransitionBlend::Override
        || !std::isfinite(blend.Value) || !std::isfinite(blend.Fraction)
        || blend.Fraction < 0 || blend.Fraction > 1) throw FormatError("Invalid automation transition blend");
    if (curve.IsEmpty()) {
        if (blend.Mode == TransitionBlend::None || blend.Mode == TransitionBlend::ToDestination) return {0, 0};
        return {blend.Value, blend.Fraction};
    }
    const auto current = curve.EvaluateTick(tick);
    switch (blend.Mode) {
        case TransitionBlend::None: return {current, 1};
        case TransitionBlend::FromCurrent: return {current + (blend.Value - current) * blend.Fraction, 1};
        case TransitionBlend::FromDestination: {
            const auto destination = curve.EvaluateTick(blend.DestinationTick);
            return {destination + (blend.Value - destination) * blend.Fraction, 1};
        }
        case TransitionBlend::ToDestination: {
            const auto destination = curve.EvaluateTick(blend.DestinationTick);
            return {current + (destination - current) * blend.Fraction, 1};
        }
        case TransitionBlend::Override: return {blend.Value, 1};
    }
    throw FormatError("Invalid automation transition mode");
}
}
