#include "OpenFmod/Studio/Bus/Graph.hpp"
#include <algorithm>
namespace OpenFmod::Studio {
std::uint64_t BusGraph::CreateScope(const Event& event) {
    if (!nextScope) throw FormatError("Bus scope identifier overflow");
    const auto id = nextScope++;
    Scope scope{event.MasterBus, event.InputBus, {event.MasterBus}};
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
    std::vector<std::uint64_t> removed;
    for (const auto& [identity, node] : nodes) if (identity.first == scope) removed.push_back(node.AudioId);
    for (auto edge = connections.begin(); edge != connections.end();) {
        if (std::find(removed.begin(), removed.end(), edge->first) != removed.end()
            || std::find(removed.begin(), removed.end(), edge->second) != removed.end()) {
            graph.RemoveDependency(edge->first, edge->second);
            edge = connections.erase(edge);
        } else ++edge;
    }
    std::sort(removed.begin(), removed.end(), std::greater<std::uint64_t>{});
    for (const auto id : removed) graph.RemoveBus(id);
    for (auto node = nodes.begin(); node != nodes.end();) {
        if (node->first.first == scope) node = nodes.erase(node);
        else ++node;
    }
    scopes.erase(scope);
    connect();
}
}
