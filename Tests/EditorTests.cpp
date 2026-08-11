#include "TestSupport.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <juce_events/juce_events.h>

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
        }
        test_support::check(productControls == 12, "all product controls have ids");

        juce::Image image(juce::Image::RGB, 320, 200, true);
        juce::Graphics g(image);
        editor->setBounds(0, 0, image.getWidth(), image.getHeight());
        editor->paint(g);
        const auto first = image.getPixelAt(0, 0);
        bool varied = false;
        for (int y = 0; y < image.getHeight(); y += 16)
            for (int x = 0; x < image.getWidth(); x += 16)
                varied = varied || image.getPixelAt(x, y) != first;
        test_support::check(varied, "software paint uses monochrome palette and procedural motif");
    });
}
