#pragma once

#include "dsp/FoldKnifeDSP.h"

#include <juce_audio_processors/juce_audio_processors.h>

class FoldKnifeAudioProcessor final : public juce::AudioProcessor
{
public:
    FoldKnifeAudioProcessor();
    ~FoldKnifeAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    foldknife::dsp::FoldKnifeDSP::Parameters readDspParameters() const noexcept;

    juce::AudioProcessorValueTreeState parameters;
    foldknife::dsp::FoldKnifeDSP dsp;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FoldKnifeAudioProcessor)
};
