#ifndef OPENFMOD_AUDIO_DELAY_LINE_HPP
#define OPENFMOD_AUDIO_DELAY_LINE_HPP
#include "OpenFmod/Binary/Reader.hpp"
#include "OpenFmod/Audio/Signal/Precision.hpp"
#include <algorithm>
#include <vector>
namespace OpenFmod::Audio {
class DelayLine {
public:
    explicit DelayLine(std::size_t maximumDelay) {
        std::size_t size = 1;
        while (size <= maximumDelay) {
            if (size > std::vector<float>().max_size() / 2) throw FormatError("Audio delay is too large");
            size *= 2;
        }
        values.resize(size);
    }
    void Advance() { position = (position - 1) & (values.size() - 1); }
    void Write(float value) { values[position] = FlushSubnormal(value); }
    float Read(std::size_t delay) const {
        if (delay >= values.size()) throw FormatError("Audio delay exceeds capacity");
        return values[(position + delay) & (values.size() - 1)];
    }
    void Reset() { std::fill(values.begin(), values.end(), 0); position = 0; }
private:
    std::vector<float> values;
    std::size_t position{};
};
}
#endif
