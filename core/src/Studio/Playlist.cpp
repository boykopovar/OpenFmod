#include "OpenFmod/Studio/Playlist.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace OpenFmod::Studio {
Playlist ReadPlaylist(std::string_view bytes) {
    Binary::Reader reader(bytes);
    Playlist result;
    result.PlayMode = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.SelectionMode = static_cast<std::uint32_t>(reader.Unsigned(4));
    if (result.PlayMode > 4 || result.SelectionMode > 1) throw FormatError("Invalid playlist mode");
    for (const auto bytes : reader.Elements(20)) {
        Binary::Reader entry(bytes);
        const auto id = Bank::ReadGuid(entry);
        const auto weight = entry.Float();
        if (!std::isfinite(weight) || weight < 0) throw FormatError("Invalid playlist weight");
        result.Entries.push_back({id, weight});
    }
    reader.Finish();
    return result;
}
PlaylistCursor::PlaylistCursor(Playlist playlist, std::vector<bool> silence) : playlist(std::move(playlist)), silence(std::move(silence)) {
    if (this->playlist.PlayMode > 4 || this->playlist.SelectionMode > 1) throw FormatError("Invalid playlist mode");
    if (this->silence.size() != this->playlist.Entries.size()) throw FormatError("Missing playlist entry types");
    minimum = std::numeric_limits<float>::max();
    for (std::size_t index = 0; index < this->playlist.Entries.size(); ++index) {
        const auto weight = this->playlist.Entries[index].Weight;
        if (!std::isfinite(weight) || weight < 0) throw FormatError("Invalid playlist weight");
        minimum = std::min(minimum, weight);
        equalWeights = equalWeights && weight == this->playlist.Entries.front().Weight;
        soundCount += !this->silence[index];
        weights.push_back({weight, 0});
    }
}
float PlaylistCursor::prepareWeights() {
    float sum = 0;
    for (std::size_t index = 0; index < weights.size(); ++index) {
        auto& weight = weights[index];
        if (index != previous && index != previousSound && weight.Deferred > 0) {
            weight.Available += weight.Deferred;
            weight.Deferred = 0;
        }
        sum += std::max(0.0f, weight.Available);
    }
    if (sum != 0) return sum;
    for (std::size_t index = 0; index < weights.size(); ++index) {
        auto& weight = weights[index];
        weight.Available += playlist.Entries[index].Weight;
        if (index == previous && weights.size() != 1) weight.Available -= minimum;
        else if (index == previousSound && equalWeights && soundCount > 1) {
            weight.Deferred = weight.Available;
            weight.Available = 0;
        }
        sum += std::max(0.0f, weight.Available);
    }
    return sum;
}
std::size_t PlaylistCursor::choose(float random) {
    float sum = 0;
    if (playlist.PlayMode == 2) sum = prepareWeights();
    else for (const auto& entry : playlist.Entries) sum += entry.Weight;
    if (!std::isfinite(sum) || sum <= 0) throw FormatError("Playlist has no positive selection weight");
    const auto target = static_cast<double>(random) * sum;
    double accumulated = 0;
    for (std::size_t index = 0; index < weights.size(); ++index) {
        const auto weight = playlist.PlayMode == 2 ? weights[index].Available : playlist.Entries[index].Weight;
        accumulated += std::max(0.0f, weight);
        if (accumulated > target) {
            if (playlist.PlayMode == 2) weights[index].Available -= minimum;
            return index;
        }
    }
    throw FormatError("Playlist selection exceeded its weight sum");
}
std::optional<std::size_t> PlaylistCursor::Next(float random) {
    if (!std::isfinite(random) || random < 0 || random >= 1) throw FormatError("Invalid playlist random value");
    if (playlist.Entries.empty()) return std::nullopt;
    if (playlist.SelectionMode == 0 && previous) return previous;
    std::size_t selected;
    if (playlist.PlayMode == 1 || playlist.PlayMode == 2) selected = choose(random);
    else selected = previous ? (*previous + 1) % playlist.Entries.size() : 0;
    previous = selected;
    if (!silence[selected]) previousSound = selected;
    return selected;
}
}
