#include "OpenFmod/Studio/Bus/Graph.hpp"
#include <algorithm>
namespace OpenFmod::Studio {
std::uint64_t BusGraph::CreateScope(const Event& event, std::uint64_t parentScope, std::optional<Bank::Guid> input) {
    if (!nextScope) throw FormatError("Bus scope identifier overflow");
    if (parentScope && !scopes.count(parentScope)) throw FormatError("Unknown parent bus scope");
    Refresh();
    const auto id = nextScope++;
    Scope scope{event.MasterBus, input.value_or(event.InputBus), {event.MasterBus}, parentScope};
    scope.Local.insert(event.Groups.begin(), event.Groups.end());
    scopes.emplace(id, std::move(scope));
    try {
        resolve(event.MasterBus, id);
        for (const auto& group : event.Groups) resolve(group, id);
        connect();
    } catch (...) {
        RemoveScope(id);
        throw;
    }
    return id;
}
void BusGraph::RemoveScope(std::uint64_t scope) {
    if (!scopes.count(scope)) throw FormatError("Unknown bus scope");
    for (const auto& [id, child] : scopes) {
        if (child.Parent == scope) throw FormatError("Cannot remove a bus scope with nested events");
        static_cast<void>(id);
    }
    NodeRemoval removed;
    for (const auto& [identity, node] : nodes) if (identity.first == scope) removed.emplace(node.AudioId, identity);
    removeNodes(removed);
    scopes.erase(scope);
    connectionsDirty = true;
    prune();
    connect();
}
void BusGraph::prune() {
    if (std::none_of(nodes.begin(), nodes.end(), [&](const auto& entry) {
        return !entry.first.first && !library.Contains<Bus>(entry.second.Model->Id);
    })) return;
    std::set<std::uint64_t> retained;
    for (const auto& [identity, node] : nodes) {
        if (identity.first || library.Contains<Bus>(node.Model->Id)) retained.insert(node.AudioId);
    }
    std::size_t previous;
    do {
        previous = retained.size();
        std::set<Bank::Guid> modulators;
        for (const auto& [identity, node] : nodes) if (retained.count(node.AudioId)) {
            retained.insert(node.ParentAudioId);
            for (const auto& [id, effect] : node.EffectModels) for (const auto& [property, settings] : effect->Properties) {
                modulators.insert(settings.Modulators.begin(), settings.Modulators.end());
                static_cast<void>(id);
                static_cast<void>(property);
            }
            static_cast<void>(identity);
        }
        for (const auto& edge : connections) if (retained.count(edge.second)) retained.insert(edge.first);
        for (const auto& [identity, node] : nodes) for (const auto& [id, sidechain] : node.Sidechains) {
            for (const auto& follower : sidechain->Followers) if (modulators.count(follower)) retained.insert(node.AudioId);
            static_cast<void>(identity);
            static_cast<void>(id);
        }
    } while (retained.size() != previous);
    NodeRemoval removed;
    for (const auto& [identity, node] : nodes) if (!retained.count(node.AudioId)) removed.emplace(node.AudioId, identity);
    removeNodes(removed);
}
void BusGraph::removeNodes(const NodeRemoval& removed) {
    if (removed.empty()) return;
    for (auto edge = connections.begin(); edge != connections.end();) {
        if (removed.count(edge->first) || removed.count(edge->second)) {
            graph.RemoveDependency(edge->first, edge->second);
            edge = connections.erase(edge);
        } else ++edge;
    }
    for (const auto& [id, identity] : removed) {
        const auto& node = nodes.at(identity);
        if (!library.Contains<Bus>(node.Model->Id)) registryRevision.reset();
        for (const auto& [sidechain, model] : node.Sidechains) {
            if (!library.Contains<Sidechain>(sidechain)) registryRevision.reset();
            static_cast<void>(model);
        }
        graph.RemoveBus(id);
        nodes.erase(identity);
    }
    connectionsDirty = true;
}
}
