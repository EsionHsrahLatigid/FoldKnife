#include "TestSupport.h"
#include "dsp/FoldKnifeDSP.h"

#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
using foldknife::dsp::FoldKnifeDSP;

bool near(float a, float b, float tolerance)
{
    return std::abs(a - b) <= tolerance;
}

float renderConstantDc(bool dcGuard)
{
    FoldKnifeDSP dsp;
    FoldKnifeDSP::Parameters parameters;
    parameters.drive = 0.8f;
    parameters.fold = 0.7f;
    parameters.bias = 0.55f;
    parameters.symmetry = 0.2f;
    parameters.mix = 1.0f;
    parameters.outputDb = 0.0f;
    parameters.dcGuard = dcGuard;
    dsp.prepare(48000.0, 64, 1);
    dsp.setTargets(parameters);
    dsp.reset();

    float sum = 0.0f;
    for (int i = 0; i < 7000; ++i)
    {
        const auto y = dsp.processSampleForTest(0.25f, 0);
        if (i > 6000)
            sum += y;
    }
    return sum / 999.0f;
}

std::vector<float> renderSine(bool aliasMode)
{
    FoldKnifeDSP dsp;
    FoldKnifeDSP::Parameters parameters;
    parameters.drive = 0.95f;
    parameters.fold = 0.9f;
    parameters.bias = 0.25f;
    parameters.symmetry = 0.77f;
    parameters.mix = 1.0f;
    parameters.outputDb = 0.0f;
    parameters.aliasMode = aliasMode;
    parameters.oversampleMode = true;
    dsp.prepare(48000.0, 512, 1);
    dsp.setTargets(parameters);
    dsp.reset();

    std::vector<float> output(512);
    for (int i = 0; i < 512; ++i)
        output[static_cast<std::size_t>(i)] = dsp.processSampleForTest(std::sin(0.91f * static_cast<float>(i)) * 0.85f, 0);
    return output;
}
} // namespace

int main()
{
    return test_support::run("foldknife_dsp_tests", [] {
        for (float x : { 0.05f, 0.2f, 0.47f, 0.81f })
        {
            const auto positive = FoldKnifeDSP::foldTransfer(x, 0.68f, 0.0f, 0.5f);
            const auto negative = FoldKnifeDSP::foldTransfer(-x, 0.68f, 0.0f, 0.5f);
            test_support::check(near(positive, -negative, 0.0002f), "centered fold transfer is odd-symmetric");
        }

        const auto asymmetricPositive = FoldKnifeDSP::foldTransfer(0.42f, 0.8f, 0.35f, 0.9f);
        const auto asymmetricNegative = FoldKnifeDSP::foldTransfer(-0.42f, 0.8f, 0.35f, 0.9f);
        test_support::check(!near(asymmetricPositive, -asymmetricNegative, 0.05f), "bias and symmetry break odd symmetry");

        float previous = FoldKnifeDSP::foldTransfer(-1.25f, 1.0f, 0.0f, 0.5f);
        float previousSlope = 0.0f;
        int turningPoints = 0;
        for (int i = 1; i <= 200; ++i)
        {
            const float x = -1.25f + static_cast<float>(i) * 2.5f / 200.0f;
            const float y = FoldKnifeDSP::foldTransfer(x, 1.0f, 0.0f, 0.5f);
            const float slope = y - previous;
            if (i > 1 && slope * previousSlope < 0.0f)
                ++turningPoints;
            previousSlope = slope;
            previous = y;
        }
        test_support::check(turningPoints >= 6, "fold transfer has multiple turning points at high fold");
        test_support::check(FoldKnifeDSP::hardClipTransfer(9.0f) == 1.0f, "hard clip positive ceiling");
        test_support::check(FoldKnifeDSP::hardClipTransfer(-9.0f) == -1.0f, "hard clip negative ceiling");

        const auto guardedDc = std::abs(renderConstantDc(true));
        const auto unguardedDc = std::abs(renderConstantDc(false));
        test_support::check(guardedDc < 0.02f, "DC guard removes sustained offset");
        test_support::check(unguardedDc > guardedDc + 0.05f, "DC guard materially changes biased transfer");

        auto oversampled = renderSine(false);
        auto aliased = renderSine(true);
        float difference = 0.0f;
        for (std::size_t i = 0; i < oversampled.size(); ++i)
        {
            test_support::check(std::isfinite(oversampled[i]), "oversampled path remains finite");
            test_support::check(std::isfinite(aliased[i]), "alias path remains finite");
            difference += std::abs(oversampled[i] - aliased[i]);
        }
        test_support::check(difference > 1.0f, "deliberate alias mode differs from oversampled mode");

        FoldKnifeDSP a;
        FoldKnifeDSP b;
        FoldKnifeDSP::Parameters parameters;
        parameters.drive = 0.71f;
        parameters.fold = 0.83f;
        parameters.symmetry = 0.12f;
        parameters.aliasMode = false;
        a.prepare(44100.0, 128, 2);
        b.prepare(44100.0, 128, 2);
        a.setTargets(parameters);
        b.setTargets(parameters);
        a.reset();
        b.reset();
        for (int i = 0; i < 256; ++i)
        {
            const float input = std::sin(static_cast<float>(i) * 0.37f);
            test_support::check(a.processSampleForTest(input, i & 1) == b.processSampleForTest(input, i & 1), "deterministic render");
        }

        FoldKnifeDSP edge;
        edge.prepare(44100.0, 0, 1);
        edge.setTargets(parameters);
        test_support::check(edge.preparedChannels() == 1, "mono prepare accepted");
        test_support::check(std::isfinite(edge.processSampleForTest(std::numeric_limits<float>::quiet_NaN(), 0)), "NaN input sanitized");
        for (int i = 0; i < 64; ++i)
            test_support::check(std::isfinite(edge.processSampleForTest(0.0f, 0)), "silence remains finite");
    });
}
