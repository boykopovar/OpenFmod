#include "OpenFmod/Studio/Project/Library.hpp"
#include "OpenFmod/Studio/Bus/Graph.hpp"
#include "../Data/Writer.hpp"
#include <stdexcept>
namespace {
std::string chunk(const std::string& id, const std::string& data) {
    std::string result = id;
    TestData::Append(result, data.size(), 4);
    result += data;
    if (data.size() % 2) result += '\0';
    return result;
}
std::string bus(unsigned char identity, unsigned char parent, const std::string& kind) {
    using namespace TestData;
    std::string header(16, '\0');
    header[0] = static_cast<char>(identity);
    Append(header, 24, 2);
    std::string route(16, '\0');
    route[0] = static_cast<char>(parent);
    header += route;
    Append(header, 0, 8);
    std::string values;
    Append(values, 0, 8);
    Append(values, 0, 4);
    Append(values, 8, 2);
    AppendFloat(values, 0);
    AppendFloat(values, 0);
    Append(values, 0, 2);
    Append(values, 32, 4);
    Append(values, 0, 4);
    Append(values, 0, 4);
    Append(values, 0xffffffff, 4);
    Append(values, 0, 4);
    return chunk("LIST", kind + "US" + chunk(kind + "SB", header) + chunk("BUS ", values)
        + chunk("LIST", "PRPS" + chunk("LCNT", std::string(4, '\0'))));
}
std::string project(unsigned char identity, float value) {
    using namespace TestData;
    std::string version;
    Append(version, 142, 4);
    std::string count;
    Append(count, 1, 4);
    std::string curve(32, '\0');
    curve[0] = 7;
    Append(curve, 3, 2);
    Append(curve, 16, 2);
    Append(curve, 0, 4);
    AppendFloat(curve, value);
    AppendFloat(curve, 0);
    Append(curve, 3, 4);
    std::string controller(52, '\0');
    controller[0] = 7;
    controller[32] = 7;
    std::string id(16, '\0');
    id[0] = static_cast<char>(identity);
    std::string body = "PROJ" + chunk("BNKI", id) + chunk("SNDH", {}) + chunk("STDT", {});
    body += chunk("LIST", "WAVS");
    body += chunk("LIST", "CRVS" + chunk("LCNT", count) + chunk("CURV", curve));
    body += chunk("LIST", "CTRS" + chunk("LCNT", count) + chunk("CTRL", controller));
    std::string masters;
    Append(masters, 2, 4);
    body += chunk("LIST", "MBSS" + chunk("LCNT", masters) + bus(8, 0, "MB") + bus(9, 0, "MB"));
    body += chunk("LIST", "GBSS" + chunk("LCNT", count) + bus(10, 9, "GB"));
    return chunk("RIFF", "FEV " + chunk("FMT ", version) + chunk("LIST", body));
}
}
void CheckProject() {
    using namespace OpenFmod::Studio;
    Library library;
    const auto first = library.Load(project(1, 3));
    const auto second = library.Load(project(2, 3));
    OpenFmod::Bank::Guid id{};
    id[0] = 7;
    if (library.Require<Curve>(id).Evaluate(0) != 3 || library.Require<Controller>(id).CurveId != id) {
        throw std::runtime_error("Resource identities must be scoped by type");
    }
    if (!library.Contains<Curve>(id) || library.GetAll<Curve>().size() != 1) throw std::runtime_error("Incorrect typed resource enumeration");
    Event event;
    event.InputBus[0] = 8;
    event.MasterBus[0] = 9;
    OpenFmod::Bank::Guid group{};
    group[0] = 10;
    event.Groups.push_back(group);
    BusGraph graph(library, 48000);
    const auto scope1 = graph.CreateScope(event);
    const auto scope2 = graph.CreateScope(event);
    if (&graph.GetMixer(group, scope1) == &graph.GetMixer(group, scope2)
        || &graph.GetMixer(event.InputBus, scope1) != &graph.GetMixer(event.InputBus, scope2)) {
        throw std::runtime_error("Local and shared bus scope isolation failed");
    }
    std::array<float, 32> silent{};
    graph.Render(silent.data(), 16);
    graph.RemoveScope(scope1);
    graph.RemoveScope(scope2);
    const auto retained = library.Retain<Curve>(id);
    library.Unload(first);
    if (library.Require<Curve>(id).Evaluate(0) != 3 || retained->Evaluate(0) != 3) {
        throw std::runtime_error("Unloading a shared resource invalidated its owners");
    }
    bool rejected = false;
    try { library.Load(project(3, 5)); }
    catch (const OpenFmod::FormatError&) { rejected = true; }
    if (!rejected || library.Require<Curve>(id).Evaluate(0) != 3) {
        throw std::runtime_error("Conflicting bank load changed the resource library");
    }
    library.Unload(second);
    if (retained->Evaluate(0) != 3) throw std::runtime_error("Active resource did not survive bank unload");
    try { library.Require<Curve>(id); }
    catch (const OpenFmod::FormatError&) { return; }
    throw std::runtime_error("Unloaded resource remains available");
}
