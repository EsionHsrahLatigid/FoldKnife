#include "TestSupport.h"
#include "ParameterIDs.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <juce_events/juce_events.h>
#include <cmath>
#include <string>

struct EditorTestAccess
{
    static void refresh(FoldKnifeAudioProcessorEditor& editor) { editor.timerCallback(); }
};

namespace
{
void checkPaintContract(juce::AudioProcessorEditor& editor, int width, int height)
{
    juce::Image image(juce::Image::RGB, width, height, true);
    editor.setBounds(0, 0, width, height);
    {
        juce::Graphics g(image);
        editor.paint(g);
    }

    const auto background = ehl::juce_design::Palette::ink();
    const auto divider = ehl::juce_design::Palette::low();
    bool headerTextHasInk = false;
    bool bodyIsPlain = true;
    bool dividerIsPresent = false;
    int maxChannelSpread = 0;

    for (int y = 0; y < image.getHeight(); ++y)
    {
        for (int x = 0; x < image.getWidth(); ++x)
        {
            const auto pixel = image.getPixelAt(x, y);
            headerTextHasInk = headerTextHasInk || (y < ehl::juce_design::Metrics::headerHeight && pixel != background);
            bodyIsPlain = bodyIsPlain && (y < ehl::juce_design::Metrics::headerHeight || pixel == background);
            dividerIsPresent = dividerIsPresent
                || (y == ehl::juce_design::Metrics::dividerY
                    && x >= ehl::juce_design::Metrics::margin
                    && x < image.getWidth() - ehl::juce_design::Metrics::margin
                    && pixel == divider);

            const int red = pixel.getRed();
            const int green = pixel.getGreen();
            const int blue = pixel.getBlue();
            const int high = juce::jmax(juce::jmax(red, green), blue);
            const int low = juce::jmin(juce::jmin(red, green), blue);
            maxChannelSpread = juce::jmax(maxChannelSpread, high - low);
        }
    }

    test_support::check(headerTextHasInk, "shared header paints product text at " + std::to_string(width) + "x" + std::to_string(height));
    test_support::check(dividerIsPresent, "shared divider is present at " + std::to_string(width) + "x" + std::to_string(height));
    test_support::check(bodyIsPlain, "paint leaves common body plain at " + std::to_string(width) + "x" + std::to_string(height));
    test_support::check(maxChannelSpread <= 4,
                        "paint stays inside EHL monochrome palette tolerance at "
                            + std::to_string(width) + "x" + std::to_string(height));
}

bool nearlyEqual(float lhs, float rhs)
{
    return std::abs(lhs - rhs) < 0.0001f;
}

float parameterNormalizedValue(FoldKnifeAudioProcessor& processor, const char* parameterID, float plainValue)
{
    auto* parameter = processor.parameters.getParameter(parameterID);
    test_support::check(parameter != nullptr, std::string("parameter exists: ") + parameterID);
    return parameter->convertTo0to1(plainValue);
}

juce::Slider& requireSlider(juce::AudioProcessorEditor& editor, const juce::String& componentID)
{
    auto* slider = dynamic_cast<juce::Slider*>(editor.findChildWithID(componentID));
    test_support::check(slider != nullptr, "slider exists: " + componentID.toStdString());
    test_support::check(slider->getSliderStyle() == juce::Slider::RotaryHorizontalVerticalDrag,
                        "slider uses shared rotary style: " + componentID.toStdString());
    test_support::check(slider->getTextBoxPosition() == juce::Slider::TextBoxBelow,
                        "slider text box is below: " + componentID.toStdString());
    return *slider;
}

void setSliderToNormalized(FoldKnifeAudioProcessor& processor, juce::Slider& slider, const char* parameterID, float normalized)
{
    auto* parameter = processor.parameters.getParameter(parameterID);
    test_support::check(parameter != nullptr, std::string("parameter exists: ") + parameterID);
    slider.setValue(parameter->convertFrom0to1(normalized), juce::sendNotificationSync);
}

void dispatchEditorTimer(FoldKnifeAudioProcessorEditor& editor)
{
    EditorTestAccess::refresh(editor);
}

void checkMaximumLayout(juce::AudioProcessorEditor& editor)
{
    editor.setBounds(0, 0, ehl::juce_design::Metrics::maximumWidth,
                     ehl::juce_design::Metrics::maximumHeight);
    editor.resized();

    auto* display = editor.findChildWithID("foldknife-parameter-display");
    test_support::check(display != nullptr, "maximum layout keeps parameter display");
    test_support::check(display->getBounds() == ehl::juce_design::parameterDisplayArea(editor.getLocalBounds()),
                        "maximum layout uses shared parameter display bounds");

    for (int i = 0; i < editor.getNumChildComponents(); ++i)
    {
        auto* child = editor.getChildComponent(i);
        if (! child->getComponentID().startsWith("foldknife-") || child == display)
            continue;
        test_support::check(! child->getBounds().isEmpty(), "maximum layout keeps product control visible");
        test_support::check(editor.getLocalBounds().contains(child->getBounds()),
                            "maximum layout keeps product control inside editor");
    }
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juce;
    return test_support::run("foldknife_editor_tests", [] {
        FoldKnifeAudioProcessor processor;
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        auto* custom = dynamic_cast<FoldKnifeAudioProcessorEditor*>(editor.get());
        test_support::check(custom != nullptr, "custom editor type, not GenericAudioProcessorEditor");
        test_support::check(dynamic_cast<juce::GenericAudioProcessorEditor*>(editor.get()) == nullptr, "not GenericAudioProcessorEditor");
        test_support::check(editor->getWidth() == FoldKnifeAudioProcessorEditor::defaultWidth, "default width");
        test_support::check(editor->getHeight() == FoldKnifeAudioProcessorEditor::defaultHeight, "default height");
        test_support::check(editor->getComponentID() == "foldknife-editor", "component id");
        test_support::check(editor->getName().isNotEmpty(), "accessible name");
        test_support::check(custom->getTooltip().isNotEmpty(), "tooltip");
        test_support::check(editor->getWantsKeyboardFocus(), "keyboard focus");
        test_support::check(editor->getNumChildComponents() >= 12, "all parameter controls are visible");

        auto* display = dynamic_cast<ehl::juce_design::ParameterDisplay*>(editor->findChildWithID("foldknife-parameter-display"));
        test_support::check(display != nullptr, "parameter display exists");
        test_support::check(display->getKind() == ehl::juce_design::DisplayKind::distortion, "parameter display kind");
        test_support::check(display->getBounds() == ehl::juce_design::parameterDisplayArea(editor->getLocalBounds()), "parameter display bounds");
        test_support::check(! display->getWantsKeyboardFocus(), "parameter display is noninteractive");

        int productControls = 0;
        for (int i = 0; i < editor->getNumChildComponents(); ++i)
        {
            auto* child = editor->getChildComponent(i);
            if (! child->getComponentID().startsWith("foldknife-"))
                continue;
            if (child == display)
                continue;
            ++productControls;
            test_support::check(child->getComponentID().isNotEmpty(), "control component id");
            test_support::check(child->getName().isNotEmpty(), "control accessible name");
            auto* tooltipClient = dynamic_cast<juce::TooltipClient*>(child);
            test_support::check(tooltipClient != nullptr && tooltipClient->getTooltip().isNotEmpty(), "control tooltip");
            test_support::check(child->getWantsKeyboardFocus(), "control keyboard focus");
            const auto bounds = child->getBounds();
            test_support::check(! bounds.isEmpty(), "control is laid out immediately after editor construction");
            test_support::check(bounds.getY() >= ehl::juce_design::Metrics::controlsTop, "control starts below the parameter display");
            test_support::check(bounds.getRight() <= editor->getWidth(), "control stays inside editor width");
            test_support::check(bounds.getBottom() <= editor->getHeight(), "control stays inside editor height");
        }
        test_support::check(productControls == 12, "all product controls have ids");

        auto& drive = requireSlider(*editor, "foldknife-drive-control");
        auto& fold = requireSlider(*editor, "foldknife-fold-control");
        auto& bias = requireSlider(*editor, "foldknife-bias-control");
        auto& symmetry = requireSlider(*editor, "foldknife-symmetry-control");
        requireSlider(*editor, "foldknife-pre-gain-control");
        requireSlider(*editor, "foldknife-post-tone-control");
        requireSlider(*editor, "foldknife-mix-control");
        requireSlider(*editor, "foldknife-output-control");

        dispatchEditorTimer(*custom);
        auto values = display->getValues();
        test_support::check(nearlyEqual(values[0], parameterNormalizedValue(processor, foldknife::parameters::drive, 0.45f)), "display reads default drive");
        test_support::check(nearlyEqual(values[1], parameterNormalizedValue(processor, foldknife::parameters::fold, 0.62f)), "display reads default fold");
        test_support::check(nearlyEqual(values[2], parameterNormalizedValue(processor, foldknife::parameters::bias, 0.0f)), "display reads default bias");
        test_support::check(nearlyEqual(values[3], parameterNormalizedValue(processor, foldknife::parameters::symmetry, 0.5f)), "display reads default symmetry");

        setSliderToNormalized(processor, drive, foldknife::parameters::drive, 0.0f);
        setSliderToNormalized(processor, fold, foldknife::parameters::fold, 0.5f);
        setSliderToNormalized(processor, bias, foldknife::parameters::bias, 1.0f);
        setSliderToNormalized(processor, symmetry, foldknife::parameters::symmetry, 0.25f);
        dispatchEditorTimer(*custom);
        values = display->getValues();
        test_support::check(nearlyEqual(values[0], 0.0f) && nearlyEqual(values[1], 0.5f)
                                && nearlyEqual(values[2], 1.0f) && nearlyEqual(values[3], 0.25f),
                            "parameter-to-display changes follow slider min/default/max positions");

        auto* clipMode = dynamic_cast<juce::ComboBox*>(editor->findChildWithID("foldknife-clip-mode-control"));
        test_support::check(clipMode != nullptr, "clip mode remains combo-controlled");
        clipMode->setSelectedId(1, juce::sendNotificationSync);
        test_support::check(processor.parameters.getRawParameterValue(foldknife::parameters::clipMode)->load() < 0.5f,
                            "clip combo writes first choice");
        clipMode->setSelectedId(3, juce::sendNotificationSync);
        test_support::check(processor.parameters.getRawParameterValue(foldknife::parameters::clipMode)->load() > 1.5f,
                            "clip combo writes third choice");

        auto* alias = dynamic_cast<juce::ToggleButton*>(editor->findChildWithID("foldknife-alias-control"));
        auto* substep = dynamic_cast<juce::ToggleButton*>(editor->findChildWithID("foldknife-substep-control"));
        auto* dcGuard = dynamic_cast<juce::ToggleButton*>(editor->findChildWithID("foldknife-dc-guard-control"));
        test_support::check(alias != nullptr && substep != nullptr && dcGuard != nullptr, "edge modes remain toggle-controlled");
        alias->setToggleState(true, juce::sendNotificationSync);
        substep->setToggleState(false, juce::sendNotificationSync);
        dcGuard->setToggleState(false, juce::sendNotificationSync);
        test_support::check(processor.parameters.getRawParameterValue(foldknife::parameters::aliasMode)->load() > 0.5f, "alias toggle writes true");
        test_support::check(processor.parameters.getRawParameterValue(foldknife::parameters::substepMode)->load() < 0.5f, "substep toggle writes false");
        test_support::check(processor.parameters.getRawParameterValue(foldknife::parameters::dcGuard)->load() < 0.5f, "dc guard toggle writes false");

        checkPaintContract(*editor, FoldKnifeAudioProcessorEditor::defaultWidth, FoldKnifeAudioProcessorEditor::defaultHeight);
        checkPaintContract(*editor, FoldKnifeAudioProcessorEditor::minimumWidth, FoldKnifeAudioProcessorEditor::minimumHeight);
        checkMaximumLayout(*editor);
        checkPaintContract(*editor, ehl::juce_design::Metrics::maximumWidth,
                           ehl::juce_design::Metrics::maximumHeight);
    });
}
