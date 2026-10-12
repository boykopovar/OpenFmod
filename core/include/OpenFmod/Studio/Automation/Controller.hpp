#ifndef OPENFMOD_STUDIO_AUTOMATION_CONTROLLER_HPP
#define OPENFMOD_STUDIO_AUTOMATION_CONTROLLER_HPP
#include "OpenFmod/Studio/Modulation/Curve.hpp"
namespace OpenFmod::Studio {
struct Controller {
    Bank::Guid Id{};
    Bank::Guid Owner{};
    Bank::Guid CurveId{};
    std::uint32_t PropertyId{};
};
Controller ReadController(std::string_view bytes);
enum class TransitionBlend { None, FromCurrent, FromDestination, ToDestination, Override };
struct AutomationBlend {
    TransitionBlend Mode{};
    float Value{};
    float Fraction{};
    std::uint32_t DestinationTick{};
};
struct AutomationValue {
    float Value{};
    float Weight{};
};
class ControllerState {
public:
    ControllerState(Controller controller, const Curve& curve);
    const Controller& GetController() const;
    AutomationValue EvaluateParameter(float input) const;
    AutomationValue EvaluateTimeline(std::uint32_t tick, const AutomationBlend& blend = {}) const;
private:
    Controller controller;
    const Curve& curve;
};
}
#endif
