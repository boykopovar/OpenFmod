#ifndef OPENFMOD_STUDIO_SNAPSHOT_RESOLVER_HPP
#define OPENFMOD_STUDIO_SNAPSHOT_RESOLVER_HPP
#include "OpenFmod/Studio/Snapshot/Model.hpp"
#include "OpenFmod/Studio/Automation/Resolver.hpp"
namespace OpenFmod::Studio {
struct SnapshotContribution {
    std::int32_t Priority{};
    float Value{};
    float Intensity{};
    bool Additive{};
    SnapshotResolution Resolution{SnapshotResolution::Average};
    bool operator==(const SnapshotContribution& other) const;
};
class SnapshotResolver {
public:
    explicit SnapshotResolver(AutomationType type, const Mapping* mapping = nullptr);
    float Resolve(float base, const std::vector<SnapshotContribution>& values) const;
private:
    float toLinear(float value) const;
    float fromLinear(float value) const;
    AutomationType type;
    const Mapping* mapping;
};
}
#endif
