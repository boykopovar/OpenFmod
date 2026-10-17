#include "OpenFmod/Audio/Filters/EqualizerDesign.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <algorithm>
#include <cmath>
namespace OpenFmod::Audio {
std::size_t FilterStages(FilterType type) {
    switch (type) {
        case FilterType::LowPass48: case FilterType::HighPass48: return 4;
        case FilterType::LowPass24: case FilterType::HighPass24: return 2;
        case FilterType::Disabled: return 0;
        default:
            if (type < FilterType::Disabled || type > FilterType::HighPass6) throw FormatError("Invalid equalizer filter type");
            return 1;
    }
}
BiquadCoefficients DesignEqualizerFilter(std::uint32_t sampleRate, EqualizerBand band) {
    const auto stages = FilterStages(band.Type);
    if (sampleRate < 8000 || !std::isfinite(band.Frequency) || band.Frequency <= 0
        || !std::isfinite(band.Quality) || band.Quality <= 0 || !std::isfinite(band.GainDecibels)) {
        throw FormatError("Invalid equalizer band settings");
    }
    if (!stages) return {1, 0, 0, 0, 0};
    const float angle = 3.14159265358979323846f * std::clamp(2 * band.Frequency / sampleRate, 0.001f, 0.95f);
    const float quality = stages == 4 ? std::sqrt(std::sqrt(band.Quality)) : stages == 2 ? std::sqrt(band.Quality) : band.Quality;
    if (band.Type >= FilterType::LowPass12 && band.Type <= FilterType::HighPass48) {
        return DesignPassFilterAtAngle(angle, quality, band.Type >= FilterType::HighPass12);
    }
    const float sine = std::sin(angle);
    const float cosine = std::cos(angle);
    const float alpha = sine / (2 * quality);
    const float amplitude = std::pow(10.0f, band.GainDecibels / 40);
    float denominator = 1 + alpha;
    BiquadCoefficients result{0, 0, 0, -2 * cosine, 1 - alpha};
    switch (band.Type) {
        case FilterType::LowShelf: case FilterType::HighShelf: {
            const float sign = band.Type == FilterType::LowShelf ? 1.0f : -1.0f;
            const float edge = std::sqrt(2 * amplitude) * sine;
            const float difference = amplitude - 1;
            const float sum = amplitude + 1;
            result = {amplitude * (sum - sign * difference * cosine + edge),
                2 * sign * amplitude * (difference - sign * sum * cosine),
                amplitude * (sum - sign * difference * cosine - edge),
                -2 * sign * (difference + sign * sum * cosine), sum + sign * difference * cosine - edge};
            denominator = sum + sign * difference * cosine + edge;
            break;
        }
        case FilterType::Peaking:
            result[0] = 1 + alpha * amplitude;
            result[1] = -2 * cosine;
            result[2] = 1 - alpha * amplitude;
            result[4] = 1 - alpha / amplitude;
            denominator = 1 + alpha / amplitude;
            break;
        case FilterType::BandPass: result[0] = alpha; result[2] = -alpha; break;
        case FilterType::Notch: result[0] = result[2] = 1; result[1] = -2 * cosine; break;
        case FilterType::AllPass: result[0] = 1 - alpha; result[1] = -2 * cosine; result[2] = 1 + alpha; break;
        case FilterType::LowPass6: {
            const float pole = std::exp(-angle);
            return {1 - pole, 0, 0, -pole, 0};
        }
        case FilterType::HighPass6: {
            const float scale = 1 / (1 + angle);
            return {scale, -scale, 0, (angle - 1) * scale, 0};
        }
        default: throw FormatError("Invalid active equalizer filter type");
    }
    for (auto& coefficient : result) coefficient /= denominator;
    return result;
}
}
