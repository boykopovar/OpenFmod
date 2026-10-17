#ifndef OPENFMOD_STUDIO_SNAPSHOT_MODEL_HPP
#define OPENFMOD_STUDIO_SNAPSHOT_MODEL_HPP
#include "OpenFmod/Studio/Modulation/Property.hpp"
namespace OpenFmod::Studio {
enum class SnapshotResolution : std::uint32_t { Average = 3, Replace = 5 };
struct SnapshotValue {
    std::uint32_t Id{};
    Bank::Guid Target{};
    std::uint32_t PropertyId{};
    float Value{};
};
struct Snapshot {
    Bank::Guid Id{};
    std::int32_t Priority{};
    std::vector<SnapshotValue> Values;
    bool Additive{};
    SnapshotResolution Resolution{};
    float Intensity{};
    std::map<std::uint32_t, Property> Properties;
};
Snapshot ReadSnapshot(std::string_view bytes, const Bank::Chunk& chunk);
}
#endif
