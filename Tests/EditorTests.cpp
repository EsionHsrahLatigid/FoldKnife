#include "TestSupport.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <juce_events/juce_events.h>
#include <string>

namespace
{
void checkPaintContract(juce::AudioProcessorEditor& editor, int width, int height)
{
    juce::Image image(juce::Image::RGB, width, height, true);
    juce::Graphics g(image);
    editor.setBounds(0, 0, width, height);
    editor.paint(g);

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

        int productControls = 0;
        for (int i = 0; i < editor->getNumChildComponents(); ++i)
        {
            auto* child = editor->getChildComponent(i);
            if (! child->getComponentID().startsWith("foldknife-"))
                continue;
            ++productControls;
            test_support::check(child->getComponentID().isNotEmpty(), "control component id");
            test_support::check(child->getName().isNotEmpty(), "control accessible name");
            auto* tooltipClient = dynamic_cast<juce::TooltipClient*>(child);
            test_support::check(tooltipClient != nullptr && tooltipClient->getTooltip().isNotEmpty(), "control tooltip");
            test_support::check(child->getWantsKeyboardFocus(), "control keyboard focus");
            const auto bounds = child->getBounds();
            test_support::check(! bounds.isEmpty(), "control is laid out immediately after editor construction");
            test_support::check(bounds.getY() >= ehl::juce_design::Metrics::headerHeight, "control starts below the shared header");
            test_support::check(bounds.getRight() <= editor->getWidth(), "control stays inside editor width");
            test_support::check(bounds.getBottom() <= editor->getHeight(), "control stays inside editor height");
        }
        test_support::check(productControls == 12, "all product controls have ids");

        checkPaintContract(*editor, FoldKnifeAudioProcessorEditor::defaultWidth, FoldKnifeAudioProcessorEditor::defaultHeight);
        checkPaintContract(*editor, FoldKnifeAudioProcessorEditor::minimumWidth, FoldKnifeAudioProcessorEditor::minimumHeight);
    });
}
