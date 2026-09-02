#pragma once

#include "BajxterEqProcessor.h"
#include "MiguelLookAndFeel.h"
#include "PrecisionRotarySlider.h"

#include <array>

class BajxterEqEditor final : public juce::AudioProcessorEditor
{
public:
    explicit BajxterEqEditor(BajxterEqAudioProcessor&);
    ~BajxterEqEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    BajxterEqAudioProcessor& processor;
    MiguelLookAndFeel lookAndFeel;
    juce::Label title;
    std::array<PrecisionRotarySlider, 3> knobs;
    std::array<juce::Label, 3> labels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BajxterEqEditor)
};
