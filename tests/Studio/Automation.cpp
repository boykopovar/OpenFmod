#include "OpenFmod/Studio/Automation/Controller.hpp"
#include "../Data/Writer.hpp"
#include <stdexcept>
void CheckAutomation() {
    using namespace OpenFmod::Studio;
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
