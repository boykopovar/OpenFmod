#include "OpenFmod/Studio/Snapshot/Resolver.hpp"
#include "../Data/Writer.hpp"
#include <cmath>
#include <stdexcept>
void CheckSnapshot() {
    using namespace OpenFmod::Studio;
    using namespace TestData;
    std::string bytes(16, '\0');
    Append(bytes, 2, 4);
    Append(bytes, 3, 2);
    Append(bytes, 28, 2);
    Append(bytes, 17, 4);
    bytes.append(16, '\0');
    Append(bytes, 1, 4);
    AppendFloat(bytes, -12);
    Append(bytes, 1, 1);
    Append(bytes, 3, 4);
    AppendFloat(bytes, 50);
    OpenFmod::Bank::Chunk chunk{"LIST", "SNAP", 0, bytes.size(), {
        {"SNAB", {}, 0, bytes.size(), {}}, {"LIST", "PRPS", 0, 0, {}}}};
    const auto model = ReadSnapshot(bytes, chunk);
    if (model.Priority != 2 || model.Values.size() != 1 || model.Values[0].Value != -12
        || !model.Additive || model.Intensity != 50) throw std::runtime_error("Incorrect snapshot decoding");
    SnapshotResolver normal(AutomationType::Normal);
    const auto mixed = normal.Resolve(10, {{0, 20, 50, false}, {0, 40, 100, false}});
    if (std::abs(mixed - 100.0f / 3) > 0.0001f) throw std::runtime_error("Incorrect weighted snapshot average");
    const auto layered = normal.Resolve(10, {{2, 100, 50, false}, {1, 40, 100, false}});
    if (layered != 70) throw std::runtime_error("Incorrect snapshot priority");
    SnapshotResolver volume(AutomationType::Volume);
    const auto attenuated = volume.Resolve(-6, {{0, -12, 100, true}});
    if (std::abs(attenuated + 18) > 0.0001f) throw std::runtime_error("Incorrect additive volume snapshot");
    const auto muted = volume.Resolve(0, {{0, -80, 50, false}});
    if (std::abs(muted + 6.0206f) > 0.0001f) throw std::runtime_error("Volume snapshots must blend linear amplitudes");
    const auto replaced = normal.Resolve(10, {{0, 20, 50, false, SnapshotResolution::Replace},
        {0, 40, 50, false, SnapshotResolution::Replace}});
    if (replaced != 27.5f) throw std::runtime_error("Incorrect ordered snapshot replacement");
}
