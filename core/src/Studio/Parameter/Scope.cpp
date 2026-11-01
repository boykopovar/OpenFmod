#include "OpenFmod/Studio/Parameter/Scope.hpp"
#include <cmath>
#include <limits>
namespace OpenFmod::Studio {
ParameterScope::ParameterScope(const Library& library) : global(true) {
    Refresh(library);
}
ParameterScope::ParameterScope(const Library& library, const Event& event, ParameterScope& globals) : global(false) {
    if (!globals.global) throw FormatError("Event requires a global parameter scope");
    globals.Refresh(library);
    for (const auto& id : event.ParameterLayouts) {
        add(library.Retain<Parameter>(library.Require<ParameterLayout>(id).ParameterId), &globals);
    }
}
void ParameterScope::Refresh(const Library& library) {
    if (!global) throw FormatError("Only the global parameter scope can be refreshed");
    if (refreshedLibrary == &library && refreshedRevision == library.GetRevision()) return;
    auto updated = *this;
    updated.entries.clear();
    updated.names.clear();
    for (auto model : library.GetAll<Parameter>()) {
        if ((model->Flags & 1) == 0) continue;
        if (!updated.names.emplace(model->Name, model->Id).second) throw FormatError("Ambiguous global parameter name");
        const auto found = entries.find(model->Id);
        if (found != entries.end()) updated.entries.emplace(model->Id, found->second);
        else {
            if (model->Default == std::numeric_limits<float>::max()) throw FormatError("Parameter requires an explicit initial value");
            updated.entries.emplace(model->Id, Entry{std::make_shared<ParameterState>(model, model->Default), true});
        }
    }
    entries.swap(updated.entries);
    names.swap(updated.names);
    refreshedLibrary = &library;
    refreshedRevision = library.GetRevision();
}
void ParameterScope::add(std::shared_ptr<const Parameter> model, ParameterScope* globals) {
    if (entries.count(model->Id)) return;
    if (names.count(model->Name)) throw FormatError("Ambiguous parameter name in scope");
    Entry entry;
    if (globals && (model->Flags & 1) != 0) {
        entry.State = globals->require(model->Id).State;
    } else {
        if (model->Default == std::numeric_limits<float>::max()) throw FormatError("Parameter requires an explicit initial value");
        entry.State = std::make_shared<ParameterState>(model, model->Default);
        entry.Owned = true;
    }
    names.emplace(model->Name, model->Id);
    entries.emplace(model->Id, std::move(entry));
}
const ParameterScope::Entry& ParameterScope::require(const Bank::Guid& id) const {
    const auto found = entries.find(id);
    if (found == entries.end()) throw FormatError("Parameter does not belong to this scope");
    return found->second;
}
float ParameterScope::GetValue(const Bank::Guid& id) const { return require(id).State->GetValue(); }
float ParameterScope::GetTarget(const Bank::Guid& id) const { return require(id).State->GetTarget(); }
Bank::Guid ParameterScope::FindId(const std::string& name) const {
    const auto found = names.find(name);
    if (found == names.end()) throw FormatError("Parameter name not found in scope");
    return found->second;
}
void ParameterScope::SetValue(const Bank::Guid& id, float value, bool immediate) {
    auto& entry = require(id);
    if (!entry.Owned) throw FormatError("Global parameter cannot be set through an event");
    entry.State->SetValue(value, immediate);
}
void ParameterScope::SetAutomaticValue(const Bank::Guid& id, float value, bool immediate) {
    auto& entry = require(id);
    if (!entry.Owned) throw FormatError("Global parameter cannot be set through an event");
    entry.State->SetAutomaticValue(value, immediate);
}
void ParameterScope::Advance(double seconds) {
    if (!std::isfinite(seconds) || seconds < 0) throw FormatError("Invalid parameter elapsed time");
    for (auto& item : entries) {
        if (item.second.Owned) item.second.State->Advance(seconds);
    }
}
}
