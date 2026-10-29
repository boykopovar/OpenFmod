#include "OpenFmod/Audio/SilenceSource.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
#include <limits>
namespace OpenFmod::Audio {
SilenceSource::SilenceSource(Format format) : format(format) {
    if (!format.Rate || !format.Channels) throw FormatError("Invalid silence format");
}
Format SilenceSource::GetFormat() const { return format; }
std::size_t SilenceSource::Read(float* output, std::size_t frames) {
    if (!output && frames) throw FormatError("Missing silence output buffer");
    const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(frames, format.Frames - position));
    if (count > std::numeric_limits<std::size_t>::max() / format.Channels) throw FormatError("Silence buffer size overflow");
    if (count) std::fill_n(output, count * format.Channels, 0.0f);
    position += count;
    return count;
}
void SilenceSource::Seek(std::uint64_t frame) {
    if (frame > format.Frames) throw FormatError("Silence seek exceeds duration");
    position = frame;
}
}
