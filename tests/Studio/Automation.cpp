#include "OpenFmod/Studio/Automation/Controller.hpp"
#include "OpenFmod/Studio/Automation/Resolver.hpp"
#include "../Data/Writer.hpp"
#include <stdexcept>
#include <limits>
void CheckAutomation() {
    using namespace OpenFmod::Studio;
    AutomationResolver average(Resolution::Average, AutomationType::Normal);
    if (average.Resolve(10, {}) != 10 || average.Resolve(0, {{4, 1}, {8, 1}}) != 6
        || average.Resolve(0, {{4, 1}, {8, 0.5f}}) != 5) throw std::runtime_error("Incorrect averaged automation");
    AutomationResolver multiply(Resolution::Multiply, AutomationType::Normal);
    if (multiply.Resolve(9, {{2, 1}, {3, 1}}) != 6) throw std::runtime_error("Incorrect multiplied automation");
    AutomationResolver volume(Resolution::Sum, AutomationType::Volume);
    if (volume.Resolve(0, {{-80, 1}, {20, 1}}) != -80) throw std::runtime_error("Silent automation contribution became audible");
    Mapping mapping({}, {{0, -10}, {1, 10}});
    AutomationResolver mapped(Resolution::Sum, AutomationType::Normal, &mapping);
    if (mapped.Resolve(0, {{0.75f, 1}, {0.5f, 1}}) != 5) throw std::runtime_error("Incorrect mapped automation origin");
    AutomationResolver replace(Resolution::Replace, AutomationType::Normal);
    if (replace.Resolve(0, {{8, 0.5f}, {4, 1}}) != 4
        || replace.Resolve(0, {{4, 1}, {8, 0.5f}}) != 6) throw std::runtime_error("Automation controller order was lost");
    if (multiply.Modulate(4, {{2, false}, {0.5f, true}}) != 3
        || multiply.Modulate(4, {{0.5f, true}, {2, false}}) != 3) throw std::runtime_error("Envelope did not scale the modulated property");
    if (mapped.Modulate(0, {{25, false}}) != 5) throw std::runtime_error("Modulation ignored mapping scale");
    if (volume.Modulate(0, {{-80, false}, {20, false}}) != std::numeric_limits<float>::lowest()) {
        throw std::runtime_error("Silent modulation contribution became audible");
    }
    std::string bytes(48, '\0');
    TestData::Append(bytes, 4, 4);
    const auto model = ReadController(bytes);
    if (model.PropertyId != 4) throw std::runtime_error("Controller property lost");
    Curve curve({}, {}, {{0, 0, 0, Interpolation::Exponential}, {100, 10, 0, Interpolation::Exponential}});
    ControllerState state(model, curve);
    const auto plain = state.EvaluateTimeline(20);
    const auto outgoing = state.EvaluateTimeline(20, {TransitionBlend::FromCurrent, 8, 0.5f, 80});
    const auto incoming = state.EvaluateTimeline(20, {TransitionBlend::FromDestination, 4, 0.5f, 80});
    const auto crossfade = state.EvaluateTimeline(20, {TransitionBlend::ToDestination, 0, 0.5f, 80});
    if (plain.Value != 2 || plain.Weight != 1 || outgoing.Value != 5 || incoming.Value != 6
        || crossfade.Value != 5) throw std::runtime_error("Incorrect timeline automation blend");
    Curve empty({}, {}, {});
    ControllerState sparse(model, empty);
    const auto inactive = sparse.EvaluateTimeline(20);
    const auto partial = sparse.EvaluateTimeline(20, {TransitionBlend::FromCurrent, 8, 0.5f, 80});
    if (inactive.Weight != 0 || partial.Value != 8 || partial.Weight != 0.5f) {
        throw std::runtime_error("Empty automation lost transition weight");
    }
    try { state.EvaluateTimeline(20, {TransitionBlend::FromCurrent, 1, 2, 0}); }
    catch (const OpenFmod::FormatError&) { return; }
    throw std::runtime_error("Invalid automation weight accepted");
}
