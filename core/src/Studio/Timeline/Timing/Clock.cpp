#include "OpenFmod/Studio/Timeline/Timing/Clock.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <limits>
namespace OpenFmod::Studio {
TimelineClock::TimelineClock(std::uint32_t sampleRate) : sampleRate(sampleRate) {
    if (!sampleRate) throw FormatError("Timeline sample rate is zero");
}
std::uint64_t TimelineClock::GetTick() const { return tick; }
double TimelineClock::GetMilliseconds() const { return (static_cast<double>(tick) + static_cast<double>(fraction) / sampleRate) / 48; }
std::uint64_t TimelineClock::FramesUntil(std::uint64_t destination) const {
    if (destination <= tick) return 0;
    const auto distance = destination - tick;
    if (distance > std::numeric_limits<std::uint64_t>::max() / sampleRate) throw FormatError("Timeline frame distance overflow");
    const auto numerator = distance * sampleRate - fraction;
    return numerator / 48000 + (numerator % 48000 != 0);
}
void TimelineClock::Advance(std::uint64_t frames) {
    const auto seconds = frames / sampleRate;
    if (seconds > (std::numeric_limits<std::uint64_t>::max() - tick) / 48000) throw FormatError("Timeline position overflow");
    const auto remainder = (frames % sampleRate) * 48000 + fraction;
    const auto whole = tick + seconds * 48000;
    const auto carry = remainder / sampleRate;
    if (carry > std::numeric_limits<std::uint64_t>::max() - whole) throw FormatError("Timeline position overflow");
    tick = whole + carry;
    fraction = remainder % sampleRate;
}
void TimelineClock::Seek(std::uint64_t position) {
    tick = position;
    fraction = 0;
}
}
