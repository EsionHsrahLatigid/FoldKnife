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
    slider.setColour(juce::Slider::trackColourId, juce::Colour(0xffd8d8d8));
    slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffffffff));
    slider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff202020));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff0f0f0));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff080808));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff707070));
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
    button.setColour(juce::ToggleButton::textColourId, juce::Colour(0xfff0f0f0));
    button.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffffffff));
    button.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(0xff505050));
}
} // namespace

FoldKnifeAudioProcessorEditor::FoldKnifeAudioProcessorEditor(FoldKnifeAudioProcessor& p)
    : AudioProcessorEditor(&p), ownerProcessor(p),
      tooltipText("FoldKnife exposes drive, fold shape, clipping, bias, symmetry, gain, antialias controls, DC guard, mix, and output.")
{
    setSize(defaultWidth, defaultHeight);
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
    clipModeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff080808));
    clipModeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xfff0f0f0));
    clipModeBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff707070));

    styleButton(aliasButton, "Alias", "foldknife-alias-control", "Bypass oversampled smoothing for deliberate DHN aliasing.");
    styleButton(oversampleButton, "4x OS", "foldknife-oversample-control", "Enable the default 4x oversampled nonlinear path.");
    styleButton(dcGuardButton, "DC Guard", "foldknife-dc-guard-control", "Remove low-frequency bias after asymmetric folding.");

    controls = { &driveSlider, &foldSlider, &clipModeBox, &biasSlider, &symmetrySlider, &preGainSlider,
                 &postToneSlider, &aliasButton, &oversampleButton, &dcGuardButton, &mixSlider, &outputSlider };
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
    oversampleAttachment = std::make_unique<ButtonAttachment>(ownerProcessor.parameters, foldknife::parameters::oversampleMode, oversampleButton);
    dcGuardAttachment = std::make_unique<ButtonAttachment>(ownerProcessor.parameters, foldknife::parameters::dcGuard, dcGuardButton);
}

void FoldKnifeAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto area = getLocalBounds();
    g.fillAll(juce::Colour(0xff050505));

    const auto grid = 8;
    g.setColour(juce::Colour(0xff202020));
    for (int x = 0; x < area.getWidth(); x += grid)
        g.drawVerticalLine(x, 0.0f, static_cast<float>(area.getHeight()));
    for (int y = 0; y < area.getHeight(); y += grid)
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(area.getWidth()));

    g.setColour(juce::Colour(0xffe8e8e8));
    g.setFont(juce::FontOptions(32.0f, juce::Font::bold));
    g.drawText("FoldKnife", 32, 24, area.getWidth() - 64, 48, juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(16.0f));
    g.drawText("jp.ehl.foldknife / FdKn", 34, 74, area.getWidth() - 68, 24, juce::Justification::centredLeft);

    const auto motif = area.reduced(32).removeFromRight(area.getWidth() / 3);
    g.setColour(juce::Colour(0xffd8d8d8));
    g.drawRect(motif, 2);

    const auto fold = ownerProcessor.parameters.getRawParameterValue(foldknife::parameters::fold)->load();
    const auto bias = ownerProcessor.parameters.getRawParameterValue(foldknife::parameters::bias)->load();
    const auto symmetry = ownerProcessor.parameters.getRawParameterValue(foldknife::parameters::symmetry)->load();
    juce::Path curve;
    for (int x = 0; x < motif.getWidth(); x += 8)
    {
        const float normalized = static_cast<float>(x) / static_cast<float>(juce::jmax(1, motif.getWidth() - 1));
        const float in = normalized * 2.0f - 1.0f;
        const float out = foldknife::dsp::FoldKnifeDSP::foldTransfer(in, fold, bias, symmetry);
        const float px = static_cast<float>(motif.getX() + x);
        const float py = static_cast<float>(motif.getCentreY()) - out * static_cast<float>(motif.getHeight()) * 0.42f;
        if (x == 0)
            curve.startNewSubPath(px, py);
        else
            curve.lineTo(px, py);
    }
    g.strokePath(curve, juce::PathStrokeType(3.0f));

    g.setColour(juce::Colour(0xff707070));
    for (int x = motif.getX(); x < motif.getRight(); x += 24)
    {
        const int h = 16 + ((x / 24) % 9) * 8;
        g.fillRect(x, motif.getBottom() - 16 - h, 8, h);
    }
}

void FoldKnifeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(32);
    area.removeFromTop(96);
    area.removeFromRight(area.getWidth() / 3 + 24);

    const int rowHeight = 32;
    const int rowGap = 8;
    for (auto* control : controls)
    {
        if (control == nullptr)
            continue;
        auto row = area.removeFromTop(rowHeight);
        control->setBounds(row);
        area.removeFromTop(rowGap);
    }
}
