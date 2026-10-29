#include "OpenFmod/Studio/Parameter/State.hpp"
#include <algorithm>
#include <cmath>
namespace OpenFmod::Studio {
namespace {
std::shared_ptr<const Parameter> RequireModel(std::shared_ptr<const Parameter> model) {
    if (!model) throw FormatError("Missing parameter model");
    if (!std::isfinite(model->Minimum) || !std::isfinite(model->Maximum)
        || model->Minimum > model->Maximum) throw FormatError("Invalid parameter range");
    return model;
}
}
ParameterState::ParameterState(std::shared_ptr<const Parameter> model, float initialValue)
    : model(RequireModel(std::move(model))), slew(this->model->SeekSpeed,
        (this->model->Flags & 4) ? this->model->DescendingSeekSpeed : this->model->SeekSpeed,
        nullptr, (this->model->Flags & 8) != 0) {
    assign(initialValue, true);
}
const Parameter& ParameterState::GetModel() const { return *model; }
bool ParameterState::IsGlobal() const { return (model->Flags & 1) != 0; }
bool ParameterState::IsReadOnly() const { return (model->Flags & 16) != 0; }
float ParameterState::GetTarget() const { return target; }
float ParameterState::GetValue() const { return slew.GetValue(); }
void ParameterState::SetValue(float value, bool immediate) {
    if (IsReadOnly()) throw FormatError("Parameter is read only");
    assign(value, immediate);
}
void ParameterState::SetAutomaticValue(float value, bool immediate) {
    if (!IsReadOnly()) throw FormatError("Parameter is not automatic");
    assign(value, immediate);
}
void ParameterState::assign(float value, bool immediate) {
    if (!std::isfinite(value)) throw FormatError("Nonfinite parameter value");
    value = std::clamp(value, model->Minimum, model->Maximum);
    target = (model->Flags & 8) ? std::floor(value) : value;
    if (immediate) slew.Reset(target);
}
void ParameterState::Advance(double seconds) { slew.Process(target, seconds); }
}
