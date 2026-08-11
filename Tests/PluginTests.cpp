#include "TestSupport.h"
#include "ParameterIDs.h"
#include "PluginProcessor.h"

#include <juce_events/juce_events.h>
#include <cmath>
#include <limits>

namespace
{
juce::AudioProcessor::BusesLayout layout(juce::AudioChannelSet input, juce::AudioChannelSet output)
{
    juce::AudioProcessor::BusesLayout result;
    result.inputBuses.add(input);
    result.outputBuses.add(output);
    return result;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juce;
    return test_support::run("foldknife_plugin_tests", [] {
        FoldKnifeAudioProcessor processor;
        test_support::check(processor.getName() == "FoldKnife", "product name");
        test_support::check(!processor.acceptsMidi(), "effect does not require MIDI");
        test_support::check(!processor.isMidiEffect(), "audio effect");
        test_support::check(processor.getLatencySamples() == 0, "zero latency");
        test_support::check(processor.getTailLengthSeconds() == 0.0, "zero tail");

        test_support::check(processor.isBusesLayoutSupported(layout(juce::AudioChannelSet::mono(), juce::AudioChannelSet::mono())),
                            "matched mono bus supported");
        test_support::check(processor.isBusesLayoutSupported(layout(juce::AudioChannelSet::stereo(), juce::AudioChannelSet::stereo())),
                            "matched stereo bus supported");
        test_support::check(!processor.isBusesLayoutSupported(layout(juce::AudioChannelSet::mono(), juce::AudioChannelSet::stereo())),
                            "mono to stereo rejected");
        test_support::check(!processor.isBusesLayoutSupported(layout(juce::AudioChannelSet::stereo(), juce::AudioChannelSet::mono())),
                            "stereo to mono rejected");

        const char* ids[] = {
            foldknife::parameters::drive, foldknife::parameters::fold, foldknife::parameters::clipMode,
            foldknife::parameters::bias, foldknife::parameters::symmetry, foldknife::parameters::preGain,
            foldknife::parameters::postTone, foldknife::parameters::aliasMode, foldknife::parameters::oversampleMode,
            foldknife::parameters::dcGuard, foldknife::parameters::mix, foldknife::parameters::output
        };
        for (const auto* id : ids)
        {
            auto* parameter = processor.parameters.getParameter(id);
            test_support::check(parameter != nullptr, std::string("parameter exists: ") + id);
            test_support::check(parameter->getName(64).isNotEmpty(), std::string("parameter name exists: ") + id);
        }

        auto* drive = processor.parameters.getParameter(foldknife::parameters::drive);
        auto* alias = processor.parameters.getParameter(foldknife::parameters::aliasMode);
        drive->setValueNotifyingHost(drive->convertTo0to1(0.77f));
        alias->setValueNotifyingHost(1.0f);
        juce::MemoryBlock state;
        processor.getStateInformation(state);
        drive->setValueNotifyingHost(drive->convertTo0to1(0.1f));
        alias->setValueNotifyingHost(0.0f);
        processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        test_support::check(std::abs(processor.parameters.getRawParameterValue(foldknife::parameters::drive)->load() - 0.77f) < 0.01f,
                            "float state round-trip");
        test_support::check(processor.parameters.getRawParameterValue(foldknife::parameters::aliasMode)->load() >= 0.5f,
                            "bool state round-trip");

        const char invalid[] = "not xml";
        processor.setStateInformation(invalid, static_cast<int>(sizeof(invalid)));
        test_support::check(std::isfinite(processor.parameters.getRawParameterValue(foldknife::parameters::drive)->load()),
                            "invalid state ignored safely");

        processor.prepareToPlay(48000.0, 64);
        juce::AudioBuffer<float> buffer(2, 64);
        for (int i = 0; i < 64; ++i)
        {
            buffer.setSample(0, i, i == 7 ? std::numeric_limits<float>::infinity() : std::sin(static_cast<float>(i) * 0.2f));
            buffer.setSample(1, i, i == 9 ? std::numeric_limits<float>::quiet_NaN() : 0.1f);
        }
        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                test_support::check(std::isfinite(buffer.getSample(ch, i)), "processed samples finite");
    });
}
