#include "OpenFmod/Studio/Project/Library.hpp"
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
    const auto retained = library.GetOwner<Curve>(id);
    library.Unload(first);
    if (library.Require<Curve>(id).Evaluate(0) != 3 || retained->Curves.at(id).Evaluate(0) != 3) {
        throw std::runtime_error("Unloading a shared resource invalidated its owners");
    }
    bool rejected = false;
    try { library.Load(project(3, 5)); }
    catch (const OpenFmod::FormatError&) { rejected = true; }
    if (!rejected || library.Require<Curve>(id).Evaluate(0) != 3) {
        throw std::runtime_error("Conflicting bank load changed the resource library");
    }
    library.Unload(second);
    try { library.Require<Curve>(id); }
    catch (const OpenFmod::FormatError&) { return; }
    throw std::runtime_error("Unloaded resource remains available");
}
