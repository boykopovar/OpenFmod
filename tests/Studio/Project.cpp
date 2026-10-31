#include "OpenFmod/Studio/Project/Library.hpp"
#include "OpenFmod/Studio/Bus/Graph.hpp"
#include "../Data/Writer.hpp"
#include "../Data/Bank.hpp"
#include <stdexcept>
#include <cmath>
#include "OpenFmod/Audio/PcmSource.hpp"
namespace {
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
    std::string body = "PROJ" + TestData::Chunk("BNKI", id) + TestData::Chunk("SNDH", {}) + TestData::Chunk("STDT", {});
    body += TestData::Chunk("LIST", "WAVS");
    body += TestData::Chunk("LIST", "CRVS" + TestData::Chunk("LCNT", count) + TestData::Chunk("CURV", curve));
    body += TestData::Chunk("LIST", "CTRS" + TestData::Chunk("LCNT", count) + TestData::Chunk("CTRL", controller));
    std::string masters;
    Append(masters, 2, 4);
    body += TestData::Chunk("LIST", "MBSS" + TestData::Chunk("LCNT", masters) + Bus(8, 0, "MB") + Bus(9, 0, "MB"));
    body += TestData::Chunk("LIST", "GBSS" + TestData::Chunk("LCNT", count) + Bus(10, 9, "GB"));
    return TestData::Chunk("RIFF", "FEV " + TestData::Chunk("FMT ", version) + TestData::Chunk("LIST", body));
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
    auto pcm = std::make_shared<OpenFmod::Audio::PcmData>();
    pcm->Layout = {48000, 2, 32};
    pcm->Samples.assign(64, 1);
    graph.GetMixer(group, scope1).Play(std::make_unique<OpenFmod::Audio::PcmSource>(pcm));
    graph.SetVolume(group, 0.5f, scope1);
    graph.SetSnapshots({{group, 0, {0, -80, 50, false}}});
    graph.Render(silent.data(), 16);
    for (const auto value : silent) if (std::abs(value - 0.25f) > 0.0001f) throw std::runtime_error("Snapshot did not preserve user bus volume");
    graph.SetSnapshots({});
    graph.Render(silent.data(), 16);
    for (const auto value : silent) if (std::abs(value - 0.5f) > 0.0001f) throw std::runtime_error("Removing snapshot did not restore bus volume");
    const auto nested = graph.CreateScope(event, scope1, group);
    graph.GetMixer(group, nested).Play(std::make_unique<OpenFmod::Audio::PcmSource>(pcm));
    graph.SetVolume(group, 0.25f, nested);
    graph.Render(silent.data(), 16);
    for (const auto value : silent) if (std::abs(value - 0.125f) > 0.0001f) throw std::runtime_error("Nested event bypassed its parent track");
    bool parentRejected = false;
    try { graph.RemoveScope(scope1); }
    catch (const OpenFmod::FormatError&) { parentRejected = true; }
    if (!parentRejected) throw std::runtime_error("Removing parent scope invalidated a nested event");
    graph.RemoveScope(nested);
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
