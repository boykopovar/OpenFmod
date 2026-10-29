#include "OpenFmod/Studio/Modulation/Curve.hpp"
#include "OpenFmod/Studio/Modulation/Mapping.hpp"
#include "OpenFmod/Studio/Modulation/Runtime/EnvelopeState.hpp"
#include "OpenFmod/Studio/Modulation/Runtime/FollowerState.hpp"
#include "OpenFmod/Studio/Modulation/Runtime/RandomState.hpp"
#include "OpenFmod/Studio/Modulation/Runtime/Instance.hpp"
#include <cmath>
#include <stdexcept>
void CheckModulation() {
    using namespace OpenFmod::Studio;
    Envelope envelope;
    envelope.Initial = -8;
    envelope.Peak = 4;
    envelope.Sustain = 2;
    envelope.Final = -4;
    envelope.AttackMilliseconds = 120;
    envelope.HoldMilliseconds = 30;
    envelope.DecayMilliseconds = 60;
    envelope.ReleaseMilliseconds = 90;
    EnvelopeState held(envelope);
    if (held.Evaluate(60) != -2 || held.Evaluate(130) != 4 || held.Evaluate(180) != 3
        || held.Evaluate(500) != 2 || held.IsFinished(500)) throw std::runtime_error("Incorrect held envelope");
    held.Release(60);
    if (held.Evaluate(105) != -3 || held.Evaluate(150) != -4 || !held.IsFinished(150)) {
        throw std::runtime_error("Release did not preserve interrupted attack level");
    }
    auto scale = std::make_shared<Mapping>(OpenFmod::Bank::Guid{}, std::vector<MappingPoint>{{0, -10}, {100, 10}});
    Modulation model;
    model.ValueMode = 1;
    model.Parameters = envelope;
    ModulationInstance instance(model, scale, false, false, 0.5f);
    instance.Release(60);
    if (!instance.Evaluate(105).IsEnvelope || instance.Evaluate(105).Value != 35 || !instance.IsFinished(150)) {
        throw std::runtime_error("Mapped envelope instance lost its release state");
    }
    EnvelopeState instant(Envelope{});
    instant.Release(0);
    if (instant.Evaluate(0) != 0 || !instant.IsFinished(0)) throw std::runtime_error("Zero duration envelope failed");
    EnvelopeFollower followerParameters;
    followerParameters.MinimumDecibels = -60;
    followerParameters.MaximumDecibels = 0;
    followerParameters.AttackMilliseconds = 100;
    followerParameters.ReleaseMilliseconds = 200;
    FollowerState follower(followerParameters);
    const float loud[] = {1, -1};
    const float quiet[] = {0, 0};
    if (follower.Process(loud, 2, 25) != 0.25f || follower.Process(loud, 2, 75) != 1
        || follower.Process(quiet, 2, 100) != 0.5f || follower.Process(quiet, 2, 100) != 0) {
        throw std::runtime_error("Incorrect follower attack or release");
    }
    followerParameters.RootMeanSquare = true;
    followerParameters.AttackMilliseconds = 0;
    FollowerState rms(followerParameters);
    const float sparse[] = {1, 0, 0, 0};
    const auto expectedRms = (20 * std::log10(0.5f) + 60) / 60;
    if (std::abs(rms.Process(sparse, 4, 1) - expectedRms) > 0.000001f) throw std::runtime_error("Incorrect follower rms");
    Mapping mapping({}, {{-1, -30}, {0, 0}, {1, 30}});
    if (mapping.Transform(0.5f) != 15 || mapping.Inverse(-7.5f) != -0.25f) throw std::runtime_error("Incorrect property mapping");
    if (mapping.Transform(-2) != -30 || mapping.Inverse(50) != 1) throw std::runtime_error("Mapping exceeded boundary");
    RandomState random(12, RandomRange::Symmetric);
    bool rejected = false;
    try { random.GetValue(); } catch (const OpenFmod::FormatError&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Random modulation allowed an uninitialized sample");
    random.Start(0.75f);
    if (random.GetValue() != 3) throw std::runtime_error("Incorrect symmetric random modulation");
    random.SetAmount(24);
    if (random.GetValue() != 6) throw std::runtime_error("Changing amount rerolled modulation");
    RandomState negative(30, RandomRange::Negative, &mapping);
    negative.Start(0.5f);
    if (negative.GetValue() != -0.5f) throw std::runtime_error("Random modulation ignored property mapping");
    Mapping steps({}, {{0, 0}, {1, 4}});
    RandomState discrete(1, RandomRange::Symmetric, &steps, true);
    discrete.Start(1);
    if (discrete.GetValue() != 0.75f) throw std::runtime_error("Discrete random range omitted its last step");
    RandomState angular(100, RandomRange::Symmetric, nullptr, false, true);
    angular.Start(0.75f);
    if (std::abs(angular.GetValue() - 16.666666f) > 0.00001f) throw std::runtime_error("Incorrect angular random distribution");
    Curve curve({}, {}, {{0, 0, 0, Interpolation::Exponential}, {100, 1, 0, Interpolation::Constant}, {200, 3, 0, Interpolation::Exponential}});
    if (curve.EvaluateTick(50) != 0.5f || curve.EvaluateTick(150) != 1 || curve.EvaluateTick(200) != 3) {
        throw std::runtime_error("Incorrect timeline curve evaluation");
    }
    for (const auto type : {Interpolation::Exponential, Interpolation::Symmetric, Interpolation::Power}) {
        if (Interpolate(-500, 500, 0, 0.7f, type) != -500 || Interpolate(-500, 500, 1, 0.7f, type) != 500) {
            throw std::runtime_error("Interpolation changed endpoints");
        }
        for (const float tension : {-1.0f, 0.0f, 1.0f}) {
            float previous = 0;
            for (int index = 0; index <= 100; ++index) {
                const auto value = Interpolate(0, 1, index / 100.0f, tension, type);
                if (!std::isfinite(value) || value < previous || value > 1) throw std::runtime_error("Invalid interpolation shape");
                previous = value;
            }
        }
    }
    const auto power = Interpolate(0, 1, 0.5f, 0, Interpolation::Power);
    if (std::abs(power - std::log2(2.5f) / 2) > 0.000001f) throw std::runtime_error("Incorrect power interpolation");
}
