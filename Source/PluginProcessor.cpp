#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace
{
std::unique_ptr<juce::AudioParameterFloat> makeFloat(const char* id,
                                                     const char* name,
                                                     juce::NormalisableRange<float> range,
                                                     float defaultValue)
{
    return std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { id, 1 }, name, range, defaultValue);
}

std::unique_ptr<juce::AudioParameterBool> makeBool(const char* id, const char* name, bool defaultValue)
{
    return std::make_unique<juce::AudioParameterBool>(juce::ParameterID { id, 1 }, name, defaultValue);
}

std::unique_ptr<juce::AudioParameterChoice> makeChoice(const char* id,
                                                       const char* name,
                                                       juce::StringArray choices,
                                                       int defaultIndex)
{
    return std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { id, 1 }, name, choices, defaultIndex);
}
} // namespace

FoldKnifeAudioProcessor::FoldKnifeAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout FoldKnifeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.reserve(12);
    params.push_back(makeFloat(foldknife::parameters::drive, "KNIFE Drive", { 0.0f, 1.0f, 0.001f }, 0.45f));
    params.push_back(makeFloat(foldknife::parameters::fold, "KNIFE Fold", { 0.0f, 1.0f, 0.001f }, 0.62f));
    params.push_back(makeChoice(foldknife::parameters::clipMode, "KNIFE Clip Mode", { "Fold", "Hard Clip", "Fold Clip" }, 2));
    params.push_back(makeFloat(foldknife::parameters::bias, "KNIFE Bias", { -1.0f, 1.0f, 0.001f }, 0.0f));
    params.push_back(makeFloat(foldknife::parameters::symmetry, "KNIFE Symmetry", { 0.0f, 1.0f, 0.001f }, 0.5f));
    params.push_back(makeFloat(foldknife::parameters::preGain, "GAIN Pre (dB)", { -24.0f, 36.0f, 0.1f }, 0.0f));
    params.push_back(makeFloat(foldknife::parameters::postTone, "TONE Post", { 0.0f, 1.0f, 0.001f }, 0.55f));
    params.push_back(makeBool(foldknife::parameters::aliasMode, "EDGE Deliberate Alias", false));
    params.push_back(makeBool(foldknife::parameters::oversampleMode, "EDGE 4x Oversample", true));
    params.push_back(makeBool(foldknife::parameters::dcGuard, "EDGE DC Guard", true));
    params.push_back(makeFloat(foldknife::parameters::mix, "OUTPUT Wet Dry", { 0.0f, 1.0f, 0.001f }, 1.0f));
    params.push_back(makeFloat(foldknife::parameters::output, "OUTPUT Gain (dB)", { -36.0f, 12.0f, 0.1f }, -3.0f));
    return { params.begin(), params.end() };
}

void FoldKnifeAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    dsp.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    dsp.setTargets(readDspParameters());
    dsp.reset();
}

bool FoldKnifeAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();
    return (mainIn == juce::AudioChannelSet::mono() && mainOut == juce::AudioChannelSet::mono())
        || (mainIn == juce::AudioChannelSet::stereo() && mainOut == juce::AudioChannelSet::stereo());
}

void FoldKnifeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    dsp.setTargets(readDspParameters());

    const int totalIn = getTotalNumInputChannels();
    const int totalOut = getTotalNumOutputChannels();
    for (int channel = totalIn; channel < totalOut; ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    std::array<float*, 2> channels { nullptr, nullptr };
    for (int channel = 0; channel < std::min(totalOut, 2); ++channel)
        channels[static_cast<std::size_t>(channel)] = buffer.getWritePointer(channel);
    dsp.processBlock(channels.data(), std::min(totalOut, 2), buffer.getNumSamples());
}

foldknife::dsp::FoldKnifeDSP::Parameters FoldKnifeAudioProcessor::readDspParameters() const noexcept
{
    foldknife::dsp::FoldKnifeDSP::Parameters values;
    values.drive = parameters.getRawParameterValue(foldknife::parameters::drive)->load();
    values.fold = parameters.getRawParameterValue(foldknife::parameters::fold)->load();
    values.clipMode = static_cast<foldknife::dsp::FoldKnifeDSP::ClipMode>(
        std::clamp(static_cast<int>(parameters.getRawParameterValue(foldknife::parameters::clipMode)->load()), 0, 2));
    values.bias = parameters.getRawParameterValue(foldknife::parameters::bias)->load();
    values.symmetry = parameters.getRawParameterValue(foldknife::parameters::symmetry)->load();
    values.preGainDb = parameters.getRawParameterValue(foldknife::parameters::preGain)->load();
    values.postTone = parameters.getRawParameterValue(foldknife::parameters::postTone)->load();
    values.aliasMode = parameters.getRawParameterValue(foldknife::parameters::aliasMode)->load() >= 0.5f;
    values.oversampleMode = parameters.getRawParameterValue(foldknife::parameters::oversampleMode)->load() >= 0.5f;
    values.dcGuard = parameters.getRawParameterValue(foldknife::parameters::dcGuard)->load() >= 0.5f;
    values.mix = parameters.getRawParameterValue(foldknife::parameters::mix)->load();
    values.outputDb = parameters.getRawParameterValue(foldknife::parameters::output)->load();
    return values;
}

juce::AudioProcessorEditor* FoldKnifeAudioProcessor::createEditor()
{
    return new FoldKnifeAudioProcessorEditor(*this);
}

void FoldKnifeAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState().createXml())
        copyXmlToBinary(*state, destData);
}

void FoldKnifeAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FoldKnifeAudioProcessor();
}
