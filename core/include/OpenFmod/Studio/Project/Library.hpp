#ifndef OPENFMOD_STUDIO_PROJECT_LIBRARY_HPP
#define OPENFMOD_STUDIO_PROJECT_LIBRARY_HPP
#include "OpenFmod/Studio/Project/Model.hpp"
#include <variant>
namespace OpenFmod::Studio {
using Resource = std::variant<const PropertyModel<Event>*, const TimelineResource*, const Instrument*, const Bus*,
    const Effect*, const Sidechain*, const Snapshot*, const VolumeControl*, const Parameter*, const ParameterLayout*,
    const Controller*, const Curve*, const Mapping*, const PropertyModel<Modulation>*, const Sound::Sample*>;
class Library {
public:
    Bank::Guid Load(std::string bytes);
    void Unload(const Bank::Guid& id);
    Bank::Guid FindId(const std::string& path) const;
    template<class TValue>
    bool Contains(const Bank::Guid& id) const {
        return objects.count(ResourceKey{typeid(TValue), id}) != 0;
    }
    template<class TValue>
    std::vector<std::shared_ptr<const TValue>> GetAll() const {
        std::vector<std::shared_ptr<const TValue>> result;
        const std::type_index type(typeid(TValue));
        for (auto item = objects.lower_bound({type, {}}); item != objects.end() && item->first.first == type; ++item) {
            result.emplace_back(item->second.Owner, std::get<const TValue*>(item->second.Value));
        }
        return result;
    }
    template<class TValue>
    std::shared_ptr<const Project> GetOwner(const Bank::Guid& id) const {
        return requireEntry(ResourceKey{typeid(TValue), id}).Owner;
    }
    template<class TValue>
    std::shared_ptr<const TValue> Retain(const Bank::Guid& id) const {
        const auto& value = Require<TValue>(id);
        return std::shared_ptr<const TValue>(GetOwner<TValue>(id), &value);
    }
    template<class TValue>
    const TValue& Require(const Bank::Guid& id) const {
        const auto& entry = requireEntry(ResourceKey{typeid(TValue), id});
        const auto* value = std::get_if<const TValue*>(&entry.Value);
        if (!value) throw FormatError("Project resource has unexpected type");
        return **value;
    }
private:
    struct Entry {
        Resource Value;
        std::shared_ptr<const Project> Owner;
    };
    const Entry& requireEntry(const ResourceKey& key) const;
    void add(const std::shared_ptr<const Project>& project);
    std::map<Bank::Guid, std::shared_ptr<const Project>> projects;
    std::map<ResourceKey, Entry> objects;
    std::map<std::string, Bank::Guid> paths;
};
}
#endif
