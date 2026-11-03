#include "OpenFmod/Studio/Bus/Graph.hpp"
#include "OpenFmod/Audio/PcmSource.hpp"
#include "../../Data/Bank.hpp"
#include <cmath>
#include <stdexcept>
namespace {
std::string identity(unsigned char value) {
    std::string bytes(16, '\0');
    bytes[0] = static_cast<char>(value);
    return bytes;
}
std::string followerBank() {
    using namespace TestData;
    const auto empty = Chunk("LIST", "PRPS" + Chunk("LCNT", std::string(4, '\0')));
    std::string count;
    Append(count, 1, 4);
    auto collection = [&](const std::string& name, const std::string& bytes) { return Chunk("LIST", name + Chunk("LCNT", count) + bytes); };
    auto sidechain = identity(2);
    Append(sidechain, 0, 1);
    Append(sidechain, 0, 2);
    Append(sidechain, 3, 2);
    Append(sidechain, 16, 2);
    sidechain += identity(4);
    AppendFloat(sidechain, 0);
    auto property = std::string{};
    Append(property, 1004, 4);
    Append(property, 0, 4);
    property += identity(0);
    Append(property, 0, 2);
    Append(property, 3, 2);
    Append(property, 16, 2);
    property += identity(4);
    auto effectHeader = identity(3);
    Append(effectHeader, 18, 4);
    Append(effectHeader, 0, 1);
    std::string parameters;
    Append(parameters, 7, 4);
    for (float value : {0.f, 1.f, 20.f, 200.f, 0.f}) { Append(parameters, 0, 4); AppendFloat(parameters, value); }
    Append(parameters, 3, 4);
    Append(parameters, 0, 2);
    Append(parameters, 2, 4);
    Append(parameters, 1, 1);
    Append(parameters, 0, 1);
    auto effect = Chunk("LIST", "BEFF" + Chunk("BEFB", effectHeader) + Chunk("PMEF", parameters)
        + collection("PRPS", Chunk("PROP", property)));
    auto modulationHeader = identity(4) + identity(3);
    Append(modulationHeader, 1004, 4);
    Append(modulationHeader, 2, 4);
    Append(modulationHeader, 0, 4);
    std::string modulation;
    Append(modulation, modulationHeader.size(), 2);
    modulation += modulationHeader;
    for (float value : {-12.f, -60.f, 0.f, 0.f, 0.f}) AppendFloat(modulation, value);
    Append(modulation, 0, 1);
    auto project = std::string("PROJ") + Chunk("BNKI", identity(5)) + Chunk("SNDH", {}) + Chunk("STDT", {}) + Chunk("LIST", "WAVS");
    project += collection("MBSS", Bus(1, 0, "MB", {2, 3}));
    project += collection("SCFX", Chunk("LIST", "SCFF" + Chunk("SCEF", sidechain) + empty));
    project += collection("BEFX", effect);
    project += collection("MODS", Chunk("LIST", "MODU" + Chunk("MODB", modulation) + empty));
    std::string version;
    Append(version, 142, 4);
    return Chunk("RIFF", "FEV " + Chunk("FMT ", version) + Chunk("LIST", project));
}
}
void CheckEffectFollower() {
    using namespace OpenFmod;
    Studio::Library library;
    library.Load(followerBank());
    Studio::BusGraph graph(library, 48000);
    auto pcm = std::make_shared<Audio::PcmData>();
    pcm->Layout = {48000, 2, 128};
    pcm->Samples.assign(256, 1);
    graph.GetMixer(Bank::Guid{1}).Play(std::make_unique<Audio::PcmSource>(pcm));
    std::array<float, 128> output{};
    graph.Render(output.data(), 64);
    if (std::abs(output.back() - 1) > 0.00001f) throw std::runtime_error("Follower used audio before its first meter block");
    graph.Render(output.data(), 64);
    if (std::abs(output.back() - std::pow(10.f, -12.f / 20)) > 0.00001f) throw std::runtime_error("Sidechain follower did not automate its effect parameter");
}
