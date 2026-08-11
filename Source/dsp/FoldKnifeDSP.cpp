#include "dsp/FoldKnifeDSP.h"

#include <algorithm>
#include <cmath>

namespace foldknife::dsp
{
void FoldKnifeDSP::prepare(double sampleRate, int maxBlockSize, int channels)
{
    sampleRate_ = std::isfinite(sampleRate) && sampleRate > 1000.0 ? sampleRate : 44100.0;
    channels_ = static_cast<int>(clamp(static_cast<float>(channels), 0.0f, static_cast<float>(maxChannels)));
    maxBlockSize_ = static_cast<int>(clamp(static_cast<float>(maxBlockSize), 0.0f, 262144.0f));
    substepScratch_.assign(static_cast<std::size_t>(std::max(1, maxBlockSize_)) * substepFactor, 0.0f);
    reset();
}

void FoldKnifeDSP::reset() noexcept
{
    for (auto& channel : state_)
        channel = {};
    current_ = target_;
}

void FoldKnifeDSP::setTargets(const Parameters& parameters) noexcept
{
    target_.drive = clamp(sanitize(parameters.drive), 0.0f, 1.0f);
    target_.fold = clamp(sanitize(parameters.fold), 0.0f, 1.0f);
    target_.clipMode = parameters.clipMode;
    target_.bias = clamp(sanitize(parameters.bias), -1.0f, 1.0f);
    target_.symmetry = clamp(sanitize(parameters.symmetry), 0.0f, 1.0f);
    target_.preGainDb = clamp(sanitize(parameters.preGainDb), -24.0f, 36.0f);
    target_.postTone = clamp(sanitize(parameters.postTone), 0.0f, 1.0f);
    target_.aliasMode = parameters.aliasMode;
    target_.substepMode = parameters.substepMode;
    target_.dcGuard = parameters.dcGuard;
    target_.mix = clamp(sanitize(parameters.mix), 0.0f, 1.0f);
    target_.outputDb = clamp(sanitize(parameters.outputDb), -36.0f, 12.0f);
}

void FoldKnifeDSP::processBlock(float* const* channelData, int numChannels, int numSamples) noexcept
{
    if (channelData == nullptr || numChannels <= 0 || numSamples <= 0)
        return;

    const int boundedChannels = std::min(numChannels, maxChannels);
    for (int sample = 0; sample < numSamples; ++sample)
    {
        advanceSmoothers();
        const auto frameParameters = current_;
        for (int channel = 0; channel < boundedChannels; ++channel)
        {
            auto* samples = channelData[channel];
            if (samples != nullptr)
                samples[sample] = processSampleWithParameters(samples[sample], channel, frameParameters);
        }
    }
}

float FoldKnifeDSP::processSampleForTest(float input, int channel) noexcept
{
    advanceSmoothers();
    return processSampleWithParameters(input, channel, current_);
}

float FoldKnifeDSP::foldTransfer(float input, float fold, float bias, float symmetry) noexcept
{
    const float amount = 1.0f + clamp(fold, 0.0f, 1.0f) * 7.0f;
    const float skew = (clamp(symmetry, 0.0f, 1.0f) - 0.5f) * 1.6f;
    float x = sanitize(input) * amount + clamp(bias, -1.0f, 1.0f) * 1.5f;
    x *= x >= 0.0f ? (1.0f + skew) : (1.0f - skew);

    constexpr float width = 2.0f;
    x = std::fmod(x + 1.0f, width * 2.0f);
    if (x < 0.0f)
        x += width * 2.0f;
    const float folded = x <= width ? x - 1.0f : 3.0f - x;
    return clamp(folded, -1.0f, 1.0f);
}

float FoldKnifeDSP::hardClipTransfer(float input) noexcept
{
    return clamp(sanitize(input), -1.0f, 1.0f);
}

float FoldKnifeDSP::sanitize(float value) noexcept
{
    return std::isfinite(value) ? value : 0.0f;
}

float FoldKnifeDSP::clamp(float value, float lo, float hi) noexcept
{
    return value < lo ? lo : (value > hi ? hi : value);
}

float FoldKnifeDSP::dbToGain(float db) noexcept
{
    return std::pow(10.0f, clamp(db, -60.0f, 24.0f) / 20.0f);
}

float FoldKnifeDSP::smooth(float current, float target) noexcept
{
    return current + (target - current) * 0.004f;
}

float FoldKnifeDSP::shape(float input, const Parameters& parameters) noexcept
{
    switch (parameters.clipMode)
    {
        case ClipMode::hardClip:
            return hardClipTransfer(input);
        case ClipMode::foldThenClip:
            return hardClipTransfer(foldTransfer(input, parameters.fold, parameters.bias, parameters.symmetry) * 1.45f);
        case ClipMode::folded:
        default:
            return foldTransfer(input, parameters.fold, parameters.bias, parameters.symmetry);
    }
}

float FoldKnifeDSP::applyDcBlock(float input, ChannelState& state) noexcept
{
    constexpr float r = 0.995f;
    const float y = input - state.dcX1 + r * state.dcY1;
    state.dcX1 = input;
    state.dcY1 = sanitize(y);
    return state.dcY1;
}

void FoldKnifeDSP::advanceSmoothers() noexcept
{
    current_.drive = smooth(current_.drive, target_.drive);
    current_.fold = smooth(current_.fold, target_.fold);
    current_.bias = smooth(current_.bias, target_.bias);
    current_.symmetry = smooth(current_.symmetry, target_.symmetry);
    current_.preGainDb = smooth(current_.preGainDb, target_.preGainDb);
    current_.postTone = smooth(current_.postTone, target_.postTone);
    current_.mix = smooth(current_.mix, target_.mix);
    current_.outputDb = smooth(current_.outputDb, target_.outputDb);
    current_.clipMode = target_.clipMode;
    current_.aliasMode = target_.aliasMode;
    current_.substepMode = target_.substepMode;
    current_.dcGuard = target_.dcGuard;
}

float FoldKnifeDSP::processSampleWithParameters(float input, int channel, const Parameters& parameters) noexcept
{
    const int stateIndex = channel <= 0 ? 0 : 1;
    input = clamp(sanitize(input), -16.0f, 16.0f);
    const float dry = input;
    float wet = 0.0f;

    if (parameters.substepMode && !parameters.aliasMode)
    {
        const float previous = state_[stateIndex].lastInput;
        for (int i = 0; i < substepFactor; ++i)
        {
            const float t = static_cast<float>(i + 1) / static_cast<float>(substepFactor);
            const float upsampled = previous + (input - previous) * t;
            wet += processAtRate(upsampled, stateIndex, parameters);
        }
        wet /= static_cast<float>(substepFactor);
        state_[stateIndex].lastInput = input;
    }
    else
    {
        wet = processAtRate(input, stateIndex, parameters);
        state_[stateIndex].lastInput = input;
    }

    const float out = (dry + (wet - dry) * parameters.mix) * dbToGain(parameters.outputDb);
    return clamp(sanitize(out), -1.25f, 1.25f);
}

float FoldKnifeDSP::processAtRate(float input, int channel, const Parameters& parameters) noexcept
{
    auto& state = state_[channel <= 0 ? 0 : 1];
    const float drive = 1.0f + parameters.drive * 23.0f;
    const float pre = clamp(input * drive * dbToGain(parameters.preGainDb), -64.0f, 64.0f);
    float wet = shape(pre, parameters);

    const float toneCoefficient = 0.03f + parameters.postTone * 0.42f;
    state.tone += (wet - state.tone) * toneCoefficient;
    wet = state.tone + (wet - state.tone) * (0.35f + parameters.postTone * 1.3f);

    if (parameters.dcGuard)
        wet = applyDcBlock(wet, state);

    return clamp(sanitize(wet), -1.0f, 1.0f);
}
} // namespace foldknife::dsp
