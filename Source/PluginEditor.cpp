#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "ParameterIDs.h"

namespace
{
namespace design = ehl::juce_design;

void styleSlider(juce::Slider& slider, const juce::String& name, const juce::String& id, const juce::String& tooltip)
{
    design::styleSlider(slider);
    slider.setName(name);
    slider.setTitle(name);
    slider.setDescription(tooltip);
    slider.setComponentID(id);
    slider.setTooltip(tooltip);
}

void styleButton(juce::ToggleButton& button, const juce::String& name, const juce::String& id, const juce::String& tooltip)
{
    design::styleToggle(button);
    button.setButtonText(name);
    button.setName(name);
    button.setTitle(name);
    button.setDescription(tooltip);
    button.setComponentID(id);
    button.setTooltip(tooltip);
}

void styleLabel(juce::Label& label, const juce::String& name)
{
    design::styleLabel(label);
    label.setText(name.toUpperCase(), juce::dontSendNotification);
    label.setName(name);
    label.setInterceptsMouseClicks(false, false);
}
} // namespace

FoldKnifeAudioProcessorEditor::FoldKnifeAudioProcessorEditor(FoldKnifeAudioProcessor& p)
    : AudioProcessorEditor(&p), ownerProcessor(p),
      tooltipText("FoldKnife exposes drive, fold shape, clipping, bias, symmetry, gain, substep integration, DC guard, mix, and output.")
{
    setLookAndFeel(&lookAndFeel);
    setResizeLimits(minimumWidth, minimumHeight, design::Metrics::maximumWidth, design::Metrics::maximumHeight);
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
    design::styleComboBox(clipModeBox);
    clipModeBox.setName("Clip Mode");
    clipModeBox.setTitle("Clip Mode");
    clipModeBox.setDescription("Select folded transfer, hard clip, or folded hard clip.");
    clipModeBox.setComponentID("foldknife-clip-mode-control");
    clipModeBox.setTooltip("Select folded transfer, hard clip, or folded hard clip.");

    styleButton(aliasButton, "Alias", "foldknife-alias-control", "Bypass 4-substep integration for deliberate aliasing.");
    styleButton(substepButton, "4x Step", "foldknife-substep-control", "Enable four interpolated nonlinear substeps per sample.");
    styleButton(dcGuardButton, "DC Guard", "foldknife-dc-guard-control", "Remove low-frequency bias after asymmetric folding.");

    parameterDisplay.setComponentID("foldknife-parameter-display");
    parameterDisplay.setName("FoldKnife parameter display");
    parameterDisplay.setTitle("FoldKnife parameter display");
    parameterDisplay.setDescription("Quantized display of drive, fold, bias, and symmetry parameter state.");
    addAndMakeVisible(parameterDisplay);

    controls = { &driveSlider, &foldSlider, &clipModeBox, &biasSlider, &symmetrySlider, &preGainSlider,
                 &postToneSlider, &aliasButton, &substepButton, &dcGuardButton, &mixSlider, &outputSlider };
    for (std::size_t i = 0; i < controls.size(); ++i)
    {
        jassert(controls[i] != nullptr);
        styleLabel(labels[i], controls[i]->getName());
        addAndMakeVisible(labels[i]);
        addAndMakeVisible(controls[i]);
    }

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

    timerCallback();
    startTimerHz(30);
    setSize(defaultWidth, defaultHeight);
}

FoldKnifeAudioProcessorEditor::~FoldKnifeAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void FoldKnifeAudioProcessorEditor::paint(juce::Graphics& g)
{
    design::paintEditorChrome(g, getLocalBounds(), "FoldKnife", "DISTORTION");
}

void FoldKnifeAudioProcessorEditor::resized()
{
    parameterDisplay.setBounds(design::parameterDisplayArea(getLocalBounds()));
    for (std::size_t i = 0; i < controls.size(); ++i)
    {
        if (controls[i] != nullptr)
            design::layoutLabelledControl(labels[i], *controls[i], design::controlCell(getLocalBounds(), i));
    }
}

void FoldKnifeAudioProcessorEditor::timerCallback()
{
    parameterDisplay.setValues({
        normalizeSlider(foldknife::parameters::drive, driveSlider),
        normalizeSlider(foldknife::parameters::fold, foldSlider),
        normalizeSlider(foldknife::parameters::bias, biasSlider),
        normalizeSlider(foldknife::parameters::symmetry, symmetrySlider)
    });
}

float FoldKnifeAudioProcessorEditor::normalizeControlValue(const char* parameterID, double value) const
{
    if (auto* parameter = ownerProcessor.parameters.getParameter(parameterID))
        return juce::jlimit(0.0f, 1.0f, parameter->convertTo0to1(static_cast<float>(value)));
    return 0.0f;
}

float FoldKnifeAudioProcessorEditor::normalizeSlider(const char* parameterID, const juce::Slider& slider) const
{
    return normalizeControlValue(parameterID, slider.getValue());
}
