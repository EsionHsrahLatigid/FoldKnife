#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "ParameterIDs.h"

namespace
{
void styleSlider(juce::Slider& slider, const juce::String& name, const juce::String& id, const juce::String& tooltip)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 88, 24);
    slider.setName(name);
    slider.setTitle(name);
    slider.setDescription(tooltip);
    slider.setComponentID(id);
    slider.setTooltip(tooltip);
    slider.setWantsKeyboardFocus(true);
    slider.setColour(juce::Slider::trackColourId, juce::Colour(0xfff2f2f0));
    slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xfff2f2f0));
    slider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff2a2a2a));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff2f2f0));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff050505));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff8a8a86));
}

void styleButton(juce::ToggleButton& button, const juce::String& name, const juce::String& id, const juce::String& tooltip)
{
    button.setButtonText(name);
    button.setName(name);
    button.setTitle(name);
    button.setDescription(tooltip);
    button.setComponentID(id);
    button.setTooltip(tooltip);
    button.setWantsKeyboardFocus(true);
    button.setColour(juce::ToggleButton::textColourId, juce::Colour(0xfff2f2f0));
    button.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xfff2f2f0));
    button.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(0xff8a8a86));
}
} // namespace

FoldKnifeAudioProcessorEditor::FoldKnifeAudioProcessorEditor(FoldKnifeAudioProcessor& p)
    : AudioProcessorEditor(&p), ownerProcessor(p),
      tooltipText("FoldKnife exposes drive, fold shape, clipping, bias, symmetry, gain, substep integration, DC guard, mix, and output.")
{
    setResizeLimits(minimumWidth, minimumHeight, defaultWidth * 2, defaultHeight * 2);
    setResizable(true, true);
    setName("FoldKnife editor");
    setComponentID("foldknife-editor");
    setTitle("FoldKnife");
    setDescription("FoldKnife monochrome 8-bit custom editor");
    setWantsKeyboardFocus(true);

    styleSlider(driveSlider, "Drive", "foldknife-drive-control", "Input drive before the wavefolder.");
    styleSlider(foldSlider, "Fold", "foldknife-fold-control", "Wavefold depth and turning-point density.");
    styleSlider(biasSlider, "Bias", "foldknife-bias-control", "DC bias into the asymmetric folding curve.");
    styleSlider(symmetrySlider, "Symmetry", "foldknife-symmetry-control", "Positive and negative fold balance.");
    styleSlider(preGainSlider, "Pre Gain", "foldknife-pre-gain-control", "Additional gain before clipping in dB.");
    styleSlider(postToneSlider, "Post Tone", "foldknife-post-tone-control", "Post-folder low/high contour.");
    styleSlider(mixSlider, "Wet Dry", "foldknife-mix-control", "Blend between dry input and folded signal.");
    styleSlider(outputSlider, "Output", "foldknife-output-control", "Final output gain in dB.");

    clipModeBox.addItem("Fold", 1);
    clipModeBox.addItem("Hard Clip", 2);
    clipModeBox.addItem("Fold Clip", 3);
    clipModeBox.setName("Clip Mode");
    clipModeBox.setTitle("Clip Mode");
    clipModeBox.setDescription("Select folded transfer, hard clip, or folded hard clip.");
    clipModeBox.setComponentID("foldknife-clip-mode-control");
    clipModeBox.setTooltip("Select folded transfer, hard clip, or folded hard clip.");
    clipModeBox.setWantsKeyboardFocus(true);
    clipModeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff050505));
    clipModeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xfff2f2f0));
    clipModeBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff8a8a86));

    styleButton(aliasButton, "Alias", "foldknife-alias-control", "Bypass 4-substep integration for deliberate DHN aliasing.");
    styleButton(substepButton, "4x Step", "foldknife-substep-control", "Enable four interpolated nonlinear substeps per sample.");
    styleButton(dcGuardButton, "DC Guard", "foldknife-dc-guard-control", "Remove low-frequency bias after asymmetric folding.");

    controls = { &driveSlider, &foldSlider, &clipModeBox, &biasSlider, &symmetrySlider, &preGainSlider,
                 &postToneSlider, &aliasButton, &substepButton, &dcGuardButton, &mixSlider, &outputSlider };
    for (auto* control : controls)
        addAndMakeVisible(control);

    driveAttachment = std::make_unique<SliderAttachment>(ownerProcessor.parameters, foldknife::parameters::drive, driveSlider);
    foldAttachment = std::make_unique<SliderAttachment>(ownerProcessor.parameters, foldknife::parameters::fold, foldSlider);
    biasAttachment = std::make_unique<SliderAttachment>(ownerProcessor.parameters, foldknife::parameters::bias, biasSlider);
    symmetryAttachment = std::make_unique<SliderAttachment>(ownerProcessor.parameters, foldknife::parameters::symmetry, symmetrySlider);
    preGainAttachment = std::make_unique<SliderAttachment>(ownerProcessor.parameters, foldknife::parameters::preGain, preGainSlider);
    postToneAttachment = std::make_unique<SliderAttachment>(ownerProcessor.parameters, foldknife::parameters::postTone, postToneSlider);
    mixAttachment = std::make_unique<SliderAttachment>(ownerProcessor.parameters, foldknife::parameters::mix, mixSlider);
    outputAttachment = std::make_unique<SliderAttachment>(ownerProcessor.parameters, foldknife::parameters::output, outputSlider);
    clipModeAttachment = std::make_unique<ComboBoxAttachment>(ownerProcessor.parameters, foldknife::parameters::clipMode, clipModeBox);
    aliasAttachment = std::make_unique<ButtonAttachment>(ownerProcessor.parameters, foldknife::parameters::aliasMode, aliasButton);
    substepAttachment = std::make_unique<ButtonAttachment>(ownerProcessor.parameters, foldknife::parameters::substepMode, substepButton);
    dcGuardAttachment = std::make_unique<ButtonAttachment>(ownerProcessor.parameters, foldknife::parameters::dcGuard, dcGuardButton);

    setSize(defaultWidth, defaultHeight);
}

void FoldKnifeAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto area = getLocalBounds();
    g.fillAll(juce::Colour(0xff050505));

    g.setColour(juce::Colour(0xfff2f2f0));
    g.setFont(juce::FontOptions(24.0f));
    g.drawText("FoldKnife", 32, 16, area.getWidth() - 64, 32, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff8a8a86));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("DISTORTION", 32, 48, area.getWidth() - 64, 16, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawHorizontalLine(72, 32.0f, static_cast<float>(area.getWidth() - 32));
}

void FoldKnifeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(32);
    area.removeFromTop(48);

    constexpr int columns = 2;
    constexpr int rows = 6;
    const int columnGap = 24;
    const int rowGap = 8;
    const int columnWidth = (area.getWidth() - columnGap) / columns;
    const int rowHeight = juce::jmax(32, juce::jmin(44, (area.getHeight() - rowGap * (rows - 1)) / rows));

    for (std::size_t i = 0; i < controls.size(); ++i)
    {
        auto* control = controls[i];
        if (control == nullptr)
            continue;
        const int column = static_cast<int>(i / rows);
        const int row = static_cast<int>(i % rows);
        control->setBounds(area.getX() + column * (columnWidth + columnGap),
                           area.getY() + row * (rowHeight + rowGap),
                           columnWidth,
                           rowHeight);
    }
}
