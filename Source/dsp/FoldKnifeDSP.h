#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace foldknife::dsp
{
class FoldKnifeDSP
{
public:
    enum class ClipMode
    {
        folded = 0,
        hardClip = 1,
        foldThenClip = 2
    };

    struct Parameters
    {
        float drive { 0.45f };
        float fold { 0.55f };
        ClipMode clipMode { ClipMode::foldThenClip };
        float bias { 0.0f };
        float symmetry { 0.5f };
        float preGainDb { 0.0f };
        float postTone { 0.55f };
        bool aliasMode { false };
        bool oversampleMode { true };
        bool dcGuard { true };
        float mix { 1.0f };
        float outputDb { -3.0f };
    };

    void prepare(double sampleRate, int maxBlockSize, int channels);
    void reset() noexcept;
    void setTargets(const Parameters& parameters) noexcept;
    void processBlock(float* const* channels, int numChannels, int numSamples) noexcept;

    float processSampleForTest(float input, int channel) noexcept;
    int preparedChannels() const noexcept { return channels_; }
    int preparedBlockSize() const noexcept { return maxBlockSize_; }

    static float foldTransfer(float input, float fold, float bias, float symmetry) noexcept;
    static float hardClipTransfer(float input) noexcept;

private:
    struct ChannelState
    {
        float dcX1 { 0.0f };
        float dcY1 { 0.0f };
        float tone { 0.0f };
        float lastInput { 0.0f };
    };

    static constexpr int maxChannels = 2;
    static constexpr int oversampleFactor = 4;

    static float sanitize(float value) noexcept;
    static float clamp(float value, float lo, float hi) noexcept;
    static float dbToGain(float db) noexcept;
    static float smooth(float current, float target) noexcept;
    static float shape(float input, const Parameters& parameters) noexcept;
    static float applyDcBlock(float input, ChannelState& state) noexcept;
    float processAtRate(float input, int channel, const Parameters& parameters) noexcept;

    double sampleRate_ { 44100.0 };
    int channels_ { 0 };
    int maxBlockSize_ { 0 };
    Parameters current_ {};
    Parameters target_ {};
    std::array<ChannelState, maxChannels> state_ {};
    std::vector<float> oversampleScratch_;
};
} // namespace foldknife::dsp
