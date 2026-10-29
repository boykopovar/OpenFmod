#ifndef OPENFMOD_STUDIO_PROJECT_MODEL_HPP
#define OPENFMOD_STUDIO_PROJECT_MODEL_HPP
#include "OpenFmod/Bank/Catalog.hpp"
#include "OpenFmod/Bank/Sources.hpp"
#include "OpenFmod/Studio/Automation/Controller.hpp"
#include "OpenFmod/Studio/Bus/Model.hpp"
#include "OpenFmod/Studio/Effect/Model.hpp"
#include "OpenFmod/Studio/Event/Model.hpp"
#include "OpenFmod/Studio/Instrument/Model.hpp"
#include "OpenFmod/Studio/Modulation/Mapping.hpp"
#include "OpenFmod/Studio/Modulation/Model.hpp"
#include "OpenFmod/Studio/Parameter/Model.hpp"
#include "OpenFmod/Studio/Snapshot/Model.hpp"
#include "OpenFmod/Studio/Timeline/Resource.hpp"
#include <memory>
#include <typeindex>
namespace OpenFmod::Studio {
using ResourceKey = std::pair<std::type_index, Bank::Guid>;
template<class TDefinition>
struct PropertyModel {
    TDefinition Definition;
    std::map<std::uint32_t, Property> Properties;
};
struct Project {
    std::shared_ptr<const std::string> Storage;
    Bank::Chunk Structure;
    Bank::Guid Id{};
    std::uint32_t Version{};
    Bank::Sources Sources;
    std::map<ResourceKey, std::string_view> EncodedObjects;
    std::vector<Bank::CatalogEntry> Catalog;
    std::map<Bank::Guid, PropertyModel<Event>> Events;
    std::map<Bank::Guid, TimelineResource> Timelines;
    std::map<Bank::Guid, Instrument> Instruments;
    std::map<Bank::Guid, Bus> Buses;
    std::map<Bank::Guid, Effect> Effects;
    std::map<Bank::Guid, Sidechain> Sidechains;
    std::map<Bank::Guid, Snapshot> Snapshots;
    std::map<Bank::Guid, VolumeControl> VolumeControls;
    std::map<Bank::Guid, Parameter> Parameters;
    std::map<Bank::Guid, ParameterLayout> ParameterLayouts;
    std::map<Bank::Guid, Controller> Controllers;
    std::map<Bank::Guid, Curve> Curves;
    std::map<Bank::Guid, Mapping> Mappings;
    std::map<Bank::Guid, PropertyModel<Modulation>> Modulators;
};
Project ReadProject(std::string bytes);
}
#endif
