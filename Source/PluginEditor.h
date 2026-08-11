#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

class FoldKnifeAudioProcessor;

class FoldKnifeAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit FoldKnifeAudioProcessorEditor(FoldKnifeAudioProcessor&);
    ~FoldKnifeAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;
    juce::String getTooltip() { return tooltipText; }

    static constexpr int defaultWidth = 960;
    static constexpr int defaultHeight = 544;
    static constexpr int minimumWidth = 720;
    static constexpr int minimumHeight = 432;

private:
    FoldKnifeAudioProcessor& ownerProcessor;
    juce::String tooltipText;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    juce::Slider driveSlider;
    juce::Slider foldSlider;
    juce::Slider biasSlider;
    juce::Slider symmetrySlider;
    juce::Slider preGainSlider;
    juce::Slider postToneSlider;
    juce::Slider mixSlider;
    juce::Slider outputSlider;
    juce::ComboBox clipModeBox;
    juce::ToggleButton aliasButton;
    juce::ToggleButton oversampleButton;
    juce::ToggleButton dcGuardButton;

    std::unique_ptr<SliderAttachment> driveAttachment;
    std::unique_ptr<SliderAttachment> foldAttachment;
    std::unique_ptr<SliderAttachment> biasAttachment;
    std::unique_ptr<SliderAttachment> symmetryAttachment;
    std::unique_ptr<SliderAttachment> preGainAttachment;
    std::unique_ptr<SliderAttachment> postToneAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;
    std::unique_ptr<ComboBoxAttachment> clipModeAttachment;
    std::unique_ptr<ButtonAttachment> aliasAttachment;
    std::unique_ptr<ButtonAttachment> oversampleAttachment;
    std::unique_ptr<ButtonAttachment> dcGuardAttachment;

    std::array<juce::Component*, 12> controls {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FoldKnifeAudioProcessorEditor)
};
