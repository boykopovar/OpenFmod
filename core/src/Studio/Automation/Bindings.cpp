#include "OpenFmod/Studio/Automation/Bindings.hpp"
namespace OpenFmod::Studio {
AutomationBindings::AutomationBindings(const Library& library, const Event& event) {
    if (event.Timeline != Bank::Guid{}) bind(library, library.Require<TimelineResource>(event.Timeline).Controllers, {});
    for (const auto& id : event.ParameterLayouts) {
        const auto& layout = library.Require<ParameterLayout>(id);
        library.Require<Parameter>(layout.ParameterId);
        bind(library, layout.Controllers, layout.ParameterId);
    }
}
void AutomationBindings::bind(const Library& library, const std::vector<Bank::Guid>& controllers, std::optional<Bank::Guid> parameter) {
    for (const auto& id : controllers) {
        const auto& controller = library.Require<Controller>(id);
        auto curve = library.Retain<Curve>(controller.CurveId);
        Binding binding{curve, ControllerState(controller, *curve), parameter};
        const auto found = bindings.find(id);
        if (found != bindings.end()) {
            if (found->second.Parameter != parameter) throw FormatError("Controller has conflicting automation inputs");
        } else {
            bindings.emplace(id, std::move(binding));
        }
    }
}
AutomationValue AutomationBindings::Evaluate(const Bank::Guid& controller, std::uint32_t tick, const std::function<float(const Bank::Guid&)>& parameters, const AutomationBlend& blend) const {
    const auto found = bindings.find(controller);
    if (found == bindings.end()) throw FormatError("Controller has no automation input");
    const auto& binding = found->second;
    if (!binding.Parameter) return binding.State.EvaluateTimeline(tick, blend);
    if (!parameters) throw FormatError("Missing automation parameter provider");
    return binding.State.EvaluateParameter(parameters(*binding.Parameter));
}
}
