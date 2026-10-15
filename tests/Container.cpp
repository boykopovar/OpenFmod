#include "OpenFmod/Bank/Container.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <stdexcept>
#include <string>
void CheckCatalog();
void CheckSound();
void CheckVorbis();
void CheckPlaylist();
void CheckModulation();
void CheckCondition();
void CheckMixer();
void CheckTimeline();
void CheckEvent();
void CheckAutomation();
void CheckInstrument();
void CheckParameter();
int main() {
    CheckCatalog();
    CheckSound();
    CheckVorbis();
    CheckPlaylist();
    CheckModulation();
    CheckCondition();
    CheckMixer();
    CheckTimeline();
    CheckEvent();
    CheckAutomation();
    CheckInstrument();
    CheckParameter();
    std::string bytes = "RIFF";
    bytes.append("\x10\0\0\0", 4);
    bytes += "FEV LIST";
    bytes.append("\x04\0\0\0", 4);
    bytes += "PROJ";
    const auto bank = OpenFmod::Bank::ReadContainer(bytes);
    if (bank.Require("LIST", "PROJ").Offset != 20) throw std::runtime_error("Wrong nested chunk offset");
    bytes.pop_back();
    try { OpenFmod::Bank::ReadContainer(bytes); }
    catch (const OpenFmod::FormatError&) { return 0; }
    throw std::runtime_error("Truncated bank accepted");
}
