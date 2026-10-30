#ifndef OPENFMOD_STUDIO_TIMELINE_CLOCK_HPP
#define OPENFMOD_STUDIO_TIMELINE_CLOCK_HPP
#include <cstdint>
namespace OpenFmod::Studio {
class TimelineClock {
public:
    explicit TimelineClock(std::uint32_t sampleRate);
    std::uint64_t GetTick() const;
    double GetMilliseconds() const;
    std::uint64_t FramesUntil(std::uint64_t tick) const;
    void Advance(std::uint64_t frames);
    void Seek(std::uint64_t tick);
private:
    std::uint32_t sampleRate;
    std::uint64_t tick{};
    std::uint64_t fraction{};
};
}
#endif
