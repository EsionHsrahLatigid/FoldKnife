#pragma once

#include <ehl/juce_design/EhlDesign.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

class FoldKnifeAudioProcessor;

class FoldKnifeAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit FoldKnifeAudioProcessorEditor(FoldKnifeAudioProcessor&);
    ~FoldKnifeAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    juce::String getTooltip() { return tooltipText; }

    static constexpr int defaultWidth = ehl::juce_design::Metrics::defaultWidth;
    static constexpr int defaultHeight = ehl::juce_design::Metrics::defaultHeight;
    static constexpr int minimumWidth = ehl::juce_design::Metrics::minimumWidth;
    static constexpr int minimumHeight = ehl::juce_design::Metrics::minimumHeight;

private:
    friend struct EditorTestAccess;

    void timerCallback() override;

    FoldKnifeAudioProcessor& ownerProcessor;
    ehl::juce_design::LookAndFeel lookAndFeel;
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
    juce::ToggleButton substepButton;
    juce::ToggleButton dcGuardButton;
    ehl::juce_design::ParameterDisplay parameterDisplay { ehl::juce_design::DisplayKind::distortion };
    std::array<juce::Label, 12> labels;

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
    std::unique_ptr<ButtonAttachment> substepAttachment;
    std::unique_ptr<ButtonAttachment> dcGuardAttachment;

    std::array<juce::Component*, 12> controls {};

    float normalizeControlValue(const char* parameterID, double value) const;
    float normalizeSlider(const char* parameterID, const juce::Slider& slider) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FoldKnifeAudioProcessorEditor)
};
