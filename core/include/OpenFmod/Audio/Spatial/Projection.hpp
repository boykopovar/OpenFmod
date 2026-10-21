#ifndef OPENFMOD_AUDIO_SPATIAL_PROJECTION_HPP
#define OPENFMOD_AUDIO_SPATIAL_PROJECTION_HPP
#include <array>
namespace OpenFmod::Audio {
enum class Rolloff { LinearSquared, Linear, Inverse, InverseTapered, Custom };
enum class ExtentMode { Automatic, User, Off };
std::array<float, 2> StereoBalance(float position);
std::array<float, 2> StereoProjection(float directionDegrees, float extentDegrees);
float DistanceAttenuation(Rolloff mode, float distance, float minimum, float maximum, float scale = 1);
float AngularExtent(ExtentMode mode, float horizontalDistance, float minimumDistance, float soundSize, float minimumExtent);
}
#endif
