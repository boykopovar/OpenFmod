#ifndef OPENFMOD_STUDIO_PARAMETER_SCOPE_HPP
#define OPENFMOD_STUDIO_PARAMETER_SCOPE_HPP
#include "OpenFmod/Studio/Parameter/State.hpp"
#include "OpenFmod/Studio/Project/Library.hpp"
namespace OpenFmod::Studio {
class ParameterScope {
public:
    explicit ParameterScope(const Library& library);
    ParameterScope(const Library& library, const Event& event, ParameterScope& globals);
    float GetValue(const Bank::Guid& id) const;
    float GetTarget(const Bank::Guid& id) const;
    Bank::Guid FindId(const std::string& name) const;
    void SetValue(const Bank::Guid& id, float value, bool immediate = false);
    void SetAutomaticValue(const Bank::Guid& id, float value, bool immediate = false);
    void Advance(double seconds);
private:
    struct Entry {
        std::shared_ptr<ParameterState> State;
        bool Owned{};
    };
    const Entry& require(const Bank::Guid& id) const;
    void add(std::shared_ptr<const Parameter> model, ParameterScope* globals);
    std::map<Bank::Guid, Entry> entries;
    std::map<std::string, Bank::Guid> names;
    bool global;
};
}
#endif
