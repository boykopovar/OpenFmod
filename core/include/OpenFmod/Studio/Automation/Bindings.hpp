#ifndef OPENFMOD_STUDIO_AUTOMATION_BINDINGS_HPP
#define OPENFMOD_STUDIO_AUTOMATION_BINDINGS_HPP
#include "OpenFmod/Studio/Project/Library.hpp"
#include <functional>
namespace OpenFmod::Studio {
class AutomationBindings {
public:
    AutomationBindings(const Library& library, const Event& event);
    AutomationValue Evaluate(const Bank::Guid& controller, std::uint32_t tick, const std::function<float(const Bank::Guid&)>& parameters, const AutomationBlend& blend = {}) const;
private:
    struct Binding {
        std::shared_ptr<const Curve> CurveModel;
        ControllerState State;
        std::optional<Bank::Guid> Parameter;
    };
    void bind(const Library& library, const std::vector<Bank::Guid>& controllers, std::optional<Bank::Guid> parameter);
    std::map<Bank::Guid, Binding> bindings;
};
}
#endif
