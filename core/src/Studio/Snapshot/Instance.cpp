#include "OpenFmod/Studio/Snapshot/Instance.hpp"
#include <cmath>
namespace OpenFmod::Studio {
std::map<std::uint32_t, float> SnapshotInstance::baseProperties(const Snapshot& model) {
    std::map<std::uint32_t, float> values{{3, model.Intensity}};
    for (const auto& value : model.Values) {
        if (value.Id < 1000 || !values.emplace(value.Id, value.Value).second) throw FormatError("Invalid snapshot value identity");
    }
    return values;
}
SnapshotInstance::SnapshotInstance(const Library& library, const Bank::Guid& id, const AutomationBindings& bindings, const std::function<float()>& random)
    : model(library.Retain<Snapshot>(id)), properties(library, baseProperties(*model), model->Properties, bindings, random) {
    layers.reserve(model->Values.size());
    for (const auto& value : model->Values) {
        layers.push_back({value.Target, value.PropertyId, {model->Priority, value.Value, 0, model->Additive, model->Resolution}});
    }
}
void SnapshotInstance::updateLayers() {
    const auto weight = finished ? 0 : intensity * properties.Get(3) * 0.01f;
    if (!std::isfinite(weight) || weight < 0 || weight > 100) throw FormatError("Invalid animated snapshot intensity");
    for (std::size_t i = 0; i < layers.size(); ++i) {
        layers[i].Contribution.Value = properties.Get(model->Values[i].Id);
        layers[i].Contribution.Intensity = weight;
    }
}
void SnapshotInstance::Update(const AutomationInput& input, double milliseconds) {
    if (!std::isfinite(milliseconds) || milliseconds < lastUpdate) throw FormatError("Invalid snapshot playback time");
    lastUpdate = milliseconds;
    if (!finished) {
        properties.Update(input, milliseconds);
        if (releasing && properties.IsFinished(milliseconds)) finished = true;
    }
    updateLayers();
}
void SnapshotInstance::SetIntensity(float percent) {
    if (!std::isfinite(percent) || percent < 0 || percent > 100) throw FormatError("Invalid snapshot intensity");
    intensity = percent;
    updateLayers();
}
void SnapshotInstance::Stop(bool immediate, double milliseconds) {
    if (!std::isfinite(milliseconds) || milliseconds < lastUpdate) throw FormatError("Invalid snapshot stop time");
    if (immediate) finished = true;
    else if (!releasing && !finished) {
        properties.Release(milliseconds);
        releasing = true;
        finished = properties.IsFinished(milliseconds);
    }
    updateLayers();
}
bool SnapshotInstance::IsPlaying() const { return !finished; }
const std::vector<SnapshotLayer>& SnapshotInstance::GetLayers() const { return layers; }
}
