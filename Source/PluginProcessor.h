#pragma once

#include "GrooveEngine.h"
#include "SectionEq.h"
#include "SessionState.h"

#include <juce_audio_utils/juce_audio_utils.h>

class MiguelMusicAssistantAudioProcessor final : public juce::AudioProcessor
{
public:
    MiguelMusicAssistantAudioProcessor();
    ~MiguelMusicAssistantAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
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

    float getRmsDb() const { return rmsDb.load(); }
    float getPeakDb() const { return peakDb.load(); }
    float getLeftRmsDb() const { return leftRmsDb.load(); }
    float getRightRmsDb() const { return rightRmsDb.load(); }
    float getLeftPeakDb() const { return leftPeakDb.load(); }
    float getRightPeakDb() const { return rightPeakDb.load(); }
    float getStereoCorrelation() const { return stereoCorrelation.load(); }
    int popAnalyzerSamples(float* destination, int maximumSamples);
    bool hasClipped() { return clipped.exchange(false); }
    bool loadSample(const juce::File&,
                    AudioSection section = AudioSection::samples);
    void playSample();
    void stopPreviews();
    bool isSamplePlaying() const { return sampleTransport.isPlaying(); }
    void startMidiPreview(const juce::MidiFile&, double bpm);
    bool isMidiPreviewPlaying() const { return midiPreviewPlaying.load(); }
    double getMidiPreviewProgress() const
    {
        return midiPreviewProgress.load();
    }
    GrooveEngine& getGrooveEngine() { return grooveEngine; }
    BroncoPianoEngine& getPianoEngine() { return pianoEngine; }
    SectionEqBank& getSectionEqBank() { return sectionEqBank; }

    void setUiSessionState(const juce::ValueTree& state);
    juce::ValueTree getUiSessionState() const;
    juce::ValueTree buildFullSessionState() const;
    void restoreFullSessionState(const juce::ValueTree& session);
    void saveAutosaveSession();
    bool loadAutosaveSession();

private:
    void addPreviewMidi(juce::MidiBuffer&, int numSamples);

    std::atomic<float> rmsDb{ -100.0f };
    std::atomic<float> peakDb{ -100.0f };
    std::atomic<float> leftRmsDb{ -100.0f };
    std::atomic<float> rightRmsDb{ -100.0f };
    std::atomic<float> leftPeakDb{ -100.0f };
    std::atomic<float> rightPeakDb{ -100.0f };
    std::atomic<float> stereoCorrelation{ 1.0f };
    std::atomic<bool> clipped{ false };
    static constexpr int analyzerBufferSize = 16384;
    std::array<float, analyzerBufferSize> analyzerBuffer{};
    juce::AbstractFifo analyzerFifo{ analyzerBufferSize };

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> sampleReader;
    juce::AudioTransportSource sampleTransport;
    juce::AudioBuffer<float> samplePreviewBuffer;
    juce::AudioBuffer<float> sectionBuffer;
    std::atomic<AudioSection> previewSection{ AudioSection::samples };

    juce::Synthesiser previewSynth;
    juce::MidiMessageSequence previewSequence;
    juce::CriticalSection previewLock;
    std::atomic<bool> midiPreviewPlaying{ false };
    std::atomic<double> midiPreviewProgress{ 0.0 };
    int nextPreviewEvent = 0;
    juce::int64 previewSamplePosition = 0;
    juce::int64 previewTotalSamples = 1;
    double activeSampleRate = 44100.0;
    double previewTempo = 120.0;
    GrooveEngine grooveEngine;
    BroncoPianoEngine pianoEngine;
    SectionEqBank sectionEqBank;
    juce::ValueTree uiSessionState{ "UiSession" };
    mutable juce::CriticalSection sessionLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiguelMusicAssistantAudioProcessor)
};
