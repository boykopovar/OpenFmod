#include "OpenFmod/Studio/Project/Model.hpp"
#include <set>
namespace OpenFmod::Studio {
namespace {
template<class TVisitor>
void visitCollection(std::string_view bytes, const Bank::Chunk& collection, TVisitor visitor) {
    collection.ValidateCount(bytes);
    for (const auto& child : collection.Children) {
        if (child.Id != "LCNT") visitor(child);
    }
}
template<class TValue>
void insert(std::map<Bank::Guid, TValue>& values, Bank::Guid id, TValue value) {
    if (!values.emplace(id, std::move(value)).second) throw FormatError("Duplicate project object identity");
}
template<class TRead>
auto readPropertyModel(std::string_view bytes, const Bank::Chunk& chunk, std::string_view form, std::string_view body, TRead read) {
    if (chunk.Id != "LIST" || chunk.Form != form || chunk.Children.size() != 2) {
        throw FormatError("Invalid property object container: " + chunk.Form);
    }
    auto value = read(chunk.Require(body).Data(bytes));
    return PropertyModel<decltype(value)>{std::move(value), ReadProperties(bytes, chunk.Require("LIST", "PRPS"))};
}
TimelineResource readTimeline(std::string_view bytes, const Bank::Chunk& chunk) {
    if (chunk.Id != "LIST" || chunk.Form != "TMLN" || chunk.Children.size() != 3) {
        throw FormatError("Invalid timeline container");
    }
    TimelineResource result;
    result.Definition = ReadTimeline(chunk.Require("TLNB").Data(bytes));
    Binary::Reader controllers(chunk.Require("CTRO").Data(bytes));
    result.Controllers = Bank::ReadGuids(controllers);
    controllers.Finish();
    visitCollection(bytes, chunk.Require("LIST", "TRNS"), [&](const auto& transition) {
        result.Transitions.push_back(ReadTransition(bytes, transition));
    });
    return result;
}
}
Project ReadProject(std::string bytes) {
    Project result;
    result.Storage = std::make_shared<const std::string>(std::move(bytes));
    const std::string_view data = *result.Storage;
    result.Structure = Bank::ReadContainer(data);
    const auto& project = result.Structure.Require("LIST", "PROJ");
    Binary::Reader version(result.Structure.Require("FMT ").Data(data));
    result.Version = static_cast<std::uint32_t>(version.Unsigned(4));
    if (result.Version < 139 || result.Version > 143) throw FormatError("Unsupported project format version");
    Binary::Reader identity(project.Require("BNKI").Data(data));
    result.Id = Bank::ReadGuid(identity);
    result.Sources = Bank::ReadSources(data, result.Structure);
    const auto strings = project.Require("STDT").Data(data);
    if (!strings.empty()) result.Catalog = Bank::ReadCatalog(strings);
    std::set<std::string> encountered;
    const std::set<std::string> collections{"IBSS", "GBSS", "MBSS", "RBSS", "EVTS", "TLNS", "MODS", "MUIS", "WAIS",
        "SLNS", "EVIS", "PRIS", "CMDS", "BEFX", "SCFX", "SNAS", "VCAS", "PRMS", "PMLS", "CTRS", "CRVS", "MPGS",
        "WAVS", "PRTS", "PEFX", "SEFX", "SPIS", "EFIS", "LWVS"};
    for (const auto& collection : project.Children) {
        if (collection.Id != "LIST") continue;
        if (!collections.count(collection.Form) || !encountered.insert(collection.Form).second) {
            throw FormatError("Unexpected or duplicate project collection: " + collection.Form);
        }
        const auto& form = collection.Form;
        if (form == "WAVS") continue;
        visitCollection(data, collection, [&](const Bank::Chunk& child) {
            if (form == "IBSS" || form == "GBSS" || form == "MBSS" || form == "RBSS") {
                auto value = ReadBus(data, child);
                insert(result.Buses, value.Id, std::move(value));
            } else if (form == "EVTS") {
                auto value = readPropertyModel(data, child, "EVNT", "EVTB", ReadEvent);
                insert(result.Events, value.Definition.Id, std::move(value));
            } else if (form == "MODS") {
                auto value = readPropertyModel(data, child, "MODU", "MODB", ReadModulation);
                insert(result.Modulators, value.Definition.Id, std::move(value));
            } else if (form == "TLNS") {
                auto value = readTimeline(data, child);
                insert(result.Timelines, value.Definition.Id, std::move(value));
            } else if (form == "MUIS" || form == "WAIS" || form == "SLNS" || form == "EVIS" || form == "PRIS" || form == "CMDS") {
                auto value = ReadInstrument(data, child);
                insert(result.Instruments, value.Definition.Id, std::move(value));
            } else if (form == "BEFX") {
                auto value = ReadEffect(data, child);
                insert(result.Effects, value.Id, std::move(value));
            } else if (form == "SCFX") {
                auto value = ReadSidechain(data, child);
                insert(result.Sidechains, value.Id, std::move(value));
            } else if (form == "SNAS") {
                auto value = ReadSnapshot(data, child);
                insert(result.Snapshots, value.Id, std::move(value));
            } else if (form == "VCAS") {
                auto value = ReadVolumeControl(data, child);
                insert(result.VolumeControls, value.Id, std::move(value));
            } else if (form == "PRMS") {
                auto value = ReadParameter(data, child, result.Version);
                insert(result.Parameters, value.Id, std::move(value));
            } else if (form == "PMLS") {
                auto value = ReadParameterLayout(data, child);
                insert(result.ParameterLayouts, value.Id, std::move(value));
            } else if (form == "CTRS") {
                if (child.Id != "CTRL") throw FormatError("Invalid project controller");
                auto value = ReadController(child.Data(data));
                insert(result.Controllers, value.Id, std::move(value));
            } else if (form == "CRVS") {
                if (child.Id != "CURV") throw FormatError("Invalid project curve");
                auto value = ReadCurve(child.Data(data));
                const auto id = value.GetId();
                insert(result.Curves, id, std::move(value));
            } else if (form == "MPGS") {
                if (child.Id != "MAP ") throw FormatError("Invalid project mapping");
                auto value = ReadMapping(child.Data(data));
                const auto id = value.GetId();
                insert(result.Mappings, id, std::move(value));
            } else throw FormatError("Unsupported nonempty project collection: " + form);
        });
    }
    return result;
}
}
