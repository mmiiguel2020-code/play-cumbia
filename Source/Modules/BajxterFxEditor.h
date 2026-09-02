#pragma once

#include "BajxterFxProcessor.h"
#include "MiguelLookAndFeel.h"
#include "PrecisionRotarySlider.h"

class BajxterFxEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BajxterFxEditor(BajxterFxAudioProcessor&);
    ~BajxterFxEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    static juce::String slotTitle(FxSlot slot);
    static juce::Colour slotColour(FxSlot slot);

    BajxterFxAudioProcessor& processor;
    MiguelLookAndFeel lookAndFeel;
    juce::Label title;
    PrecisionRotarySlider knob;
    juce::Label valueLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BajxterFxEditor)
};
