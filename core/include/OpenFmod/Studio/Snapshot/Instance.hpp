#ifndef OPENFMOD_STUDIO_SNAPSHOT_INSTANCE_HPP
#define OPENFMOD_STUDIO_SNAPSHOT_INSTANCE_HPP
#include "OpenFmod/Studio/Automation/PropertySet.hpp"
#include "OpenFmod/Studio/Snapshot/Resolver.hpp"
namespace OpenFmod::Studio {
struct SnapshotLayer {
    Bank::Guid Target{};
    std::uint32_t Property{};
    SnapshotContribution Contribution;
};
class SnapshotInstance {
public:
    SnapshotInstance(const Library& library, const Bank::Guid& id, const AutomationBindings& bindings, const std::function<float()>& random);
    void Update(const AutomationInput& input, double milliseconds);
    void SetIntensity(float percent);
    void Stop(bool immediate, double milliseconds);
    bool IsPlaying() const;
    const std::vector<SnapshotLayer>& GetLayers() const;
private:
    static std::map<std::uint32_t, float> baseProperties(const Snapshot& model);
    void updateLayers();
    std::shared_ptr<const Snapshot> model;
    PropertySet properties;
    std::vector<SnapshotLayer> layers;
    float intensity{100};
    double lastUpdate{};
    bool releasing{};
    bool finished{};
};
}
#endif
