#include "OpenFmod/Studio/Bus/Graph.hpp"
namespace OpenFmod::Studio {
void BusGraph::Refresh() {
    if (registryRevision == library.GetRevision()) return;
    std::map<Bank::Guid, std::shared_ptr<const Bus>> buses;
    std::map<Bank::Guid, std::shared_ptr<const Sidechain>> sidechains;
    for (auto model : library.GetAll<Bus>()) buses.emplace(model->Id, std::move(model));
    for (auto model : library.GetAll<Sidechain>()) sidechains.emplace(model->Id, std::move(model));
    for (const auto& [identity, node] : nodes) {
        buses.emplace(node.Model->Id, node.Model);
        sidechains.insert(node.Sidechains.begin(), node.Sidechains.end());
        static_cast<void>(identity);
    }
    decltype(providers) nextProviders;
    decltype(controls) nextControls;
    for (const auto& [id, bus] : buses) {
        for (const auto* chain : {&bus->PreEffects, &bus->PostEffects}) {
            for (const auto& effect : *chain) {
                if (sidechains.count(effect.Id)) nextProviders[effect.Id].push_back(id);
            }
        }
    }
    for (const auto& [id, control] : sidechains) {
        for (const auto& effect : control->Effects) nextControls[effect].push_back(id);
        for (const auto& follower : control->Followers) nextControls[follower].push_back(id);
    }
    providers.swap(nextProviders);
    controls.swap(nextControls);
    registryRevision = library.GetRevision();
    connectionsDirty = true;
}
}
