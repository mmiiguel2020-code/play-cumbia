#pragma once

#include <array>
#include <juce_audio_processors/juce_audio_processors.h>

class BajxterEqAudioProcessor final : public juce::AudioProcessor
{
public:
    BajxterEqAudioProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    void setBandDb(int band, float decibels);
    float getBandDb(int band) const;

private:
    void updateFilters();

    mutable juce::CriticalSection lock;
    double activeSampleRate = 44100.0;
    std::array<float, 3> gains{};
    std::array<std::array<juce::IIRFilter, 3>, 2> filters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BajxterEqAudioProcessor)
};
