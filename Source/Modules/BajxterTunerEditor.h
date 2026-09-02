#pragma once

#include "BajxterTunerProcessor.h"
#include "MiguelLookAndFeel.h"

class BajxterTunerEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BajxterTunerEditor(BajxterTunerAudioProcessor&);
    ~BajxterTunerEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    BajxterTunerAudioProcessor& processor;
    MiguelLookAndFeel lookAndFeel;
    juce::Label title;
    juce::Label readout;
    double cents = 0.0;
    bool hasReading = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BajxterTunerEditor)
};
