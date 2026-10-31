#ifndef OPENFMOD_STUDIO_SNAPSHOT_MIX_HPP
#define OPENFMOD_STUDIO_SNAPSHOT_MIX_HPP
#include "OpenFmod/Studio/Snapshot/Instance.hpp"
namespace OpenFmod::Studio {
class SnapshotMix {
public:
    bool SetLayers(const std::vector<SnapshotLayer>& layers);
    float Resolve(const Bank::Guid& target, std::uint32_t property, float base, AutomationType type, const Mapping* mapping = nullptr) const;
    void ValidateTarget(const Bank::Guid& target, const std::function<bool(std::uint32_t)>& accepts) const;
private:
    std::map<Bank::Guid, std::map<std::uint32_t, std::vector<SnapshotContribution>>> values;
};
}
#endif
