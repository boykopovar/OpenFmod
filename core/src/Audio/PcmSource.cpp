#include "OpenFmod/Audio/PcmSource.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
#include <limits>
namespace OpenFmod::Audio {
PcmSource::PcmSource(std::shared_ptr<const PcmData> data) : data(std::move(data)) {
    if (!this->data) throw FormatError("Missing PCM data");
    const auto& layout = this->data->Layout;
    if (!layout.Rate || !layout.Channels || layout.Frames > std::numeric_limits<std::size_t>::max() / layout.Channels
        || this->data->Samples.size() != layout.Frames * layout.Channels) throw FormatError("Invalid PCM layout");
}
Format PcmSource::GetFormat() const { return data->Layout; }
std::size_t PcmSource::Read(float* output, std::size_t frames) {
    if (!output && frames) throw FormatError("Missing PCM output buffer");
    const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(frames, data->Layout.Frames - position));
    if (count) std::copy_n(data->Samples.data() + position * data->Layout.Channels, count * data->Layout.Channels, output);
    position += count;
    return count;
}
void PcmSource::Seek(std::uint64_t frame) {
    if (frame > data->Layout.Frames) throw FormatError("PCM seek exceeds sample duration");
    position = frame;
}
}
