#pragma once

#include <array>
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>

class BajxterTunerAudioProcessor final : public juce::AudioProcessor
{
public:
    BajxterTunerAudioProcessor();

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

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    bool hasReading() const { return hasPitch.load(); }
    double getFrequency() const { return frequency.load(); }
    double getCents() const { return cents.load(); }
    juce::String getNote() const;

private:
    void analyseLocked();

    mutable juce::CriticalSection lock;
    juce::AudioBuffer<float> capture;
    int writePos = 0;
    int samplesUntilAnalyse = 0;
    double activeSampleRate = 44100.0;
    std::atomic<bool> hasPitch{ false };
    std::atomic<double> frequency{ 0.0 };
    std::atomic<double> cents{ 0.0 };
    juce::String note{ "-" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BajxterTunerAudioProcessor)
};
