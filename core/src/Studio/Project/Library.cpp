#include "OpenFmod/Studio/Project/Library.hpp"
#include <type_traits>
namespace OpenFmod::Studio {
namespace {
bool sameSample(const Sound::Sample& a, const Sound::Sample& b) {
    if (a.Name != b.Name || a.Rate != b.Rate || a.Channels != b.Channels || a.Frames != b.Frames
        || a.Peak != b.Peak || a.VorbisSetup != b.VorbisSetup || a.Data != b.Data
        || a.Repeat.has_value() != b.Repeat.has_value() || a.Cues.size() != b.Cues.size()
        || a.SeekPoints.size() != b.SeekPoints.size()) return false;
    if (a.Repeat && (a.Repeat->Begin != b.Repeat->Begin || a.Repeat->End != b.Repeat->End)) return false;
    for (std::size_t index = 0; index < a.Cues.size(); ++index) {
        if (a.Cues[index].Frame != b.Cues[index].Frame || a.Cues[index].Name != b.Cues[index].Name) return false;
    }
    for (std::size_t index = 0; index < a.SeekPoints.size(); ++index) {
        if (a.SeekPoints[index].Frame != b.SeekPoints[index].Frame || a.SeekPoints[index].Offset != b.SeekPoints[index].Offset) return false;
    }
    return true;
}
}

void Library::add(const std::shared_ptr<const Project>& project) {
    if (!projects.emplace(project->Id, project).second) throw FormatError("Project is already loaded");
    const auto store = [&](Bank::Guid id, const auto* resource) {
        const ResourceKey key{typeid(*resource), id};
        const auto found = objects.find(key);
        if (found != objects.end()) {
            bool same = Resource(resource).index() == found->second.Value.index();
            if constexpr (std::is_same_v<std::decay_t<decltype(*resource)>, Sound::Sample>) {
                same = same && sameSample(*resource, *std::get<const Sound::Sample*>(found->second.Value));
            } else same = same && project->EncodedObjects.at(key) == found->second.Owner->EncodedObjects.at(key);
            if (!same) throw FormatError(std::string("Loaded banks contain conflicting resource definitions: ") + typeid(*resource).name());
        } else objects.emplace(key, Entry{resource, project});
    };
    const auto collect = [&](const auto& resources) {
        for (const auto& [id, value] : resources) store(id, &value);
    };
    collect(project->Events);
    collect(project->Timelines);
    collect(project->Instruments);
    collect(project->Buses);
    collect(project->Effects);
    collect(project->Sidechains);
    collect(project->Snapshots);
    collect(project->VolumeControls);
    collect(project->Parameters);
    collect(project->ParameterLayouts);
    collect(project->Controllers);
    collect(project->Curves);
    collect(project->Mappings);
    collect(project->Modulators);
    for (const auto& [id, wave] : project->Sources.Waves) {
        store(id, &project->Sources.Require(id));
        static_cast<void>(wave);
    }
    for (const auto& item : project->Catalog) {
        const auto [existing, inserted] = paths.emplace(item.Path, item.Id);
        if (!inserted && existing->second != item.Id) throw FormatError("Conflicting bank resource path");
    }
}
Bank::Guid Library::Load(std::string bytes) {
    const auto project = std::make_shared<const Project>(ReadProject(std::move(bytes)));
    Library staged = *this;
    staged.add(project);
    *this = std::move(staged);
    return project->Id;
}
void Library::Unload(const Bank::Guid& id) {
    if (!projects.count(id)) throw FormatError("Cannot unload an unknown project");
    Library staged;
    for (const auto& [key, project] : projects) if (key != id) staged.add(project);
    *this = std::move(staged);
}
Bank::Guid Library::FindId(const std::string& path) const {
    const auto found = paths.find(path);
    if (found == paths.end()) throw FormatError("Unknown bank resource path: " + path);
    return found->second;
}
std::string Library::GetPath(const Bank::Guid& id) const {
    for (const auto& [path, value] : paths) if (value == id) return path;
    throw FormatError("Resource path is not present in loaded string banks");
}
const Library::Entry& Library::requireEntry(const ResourceKey& key) const {
    const auto found = objects.find(key);
    if (found == objects.end()) throw FormatError("Missing project resource owner");
    return found->second;
}
}

