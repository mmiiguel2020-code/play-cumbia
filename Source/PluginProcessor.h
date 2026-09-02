#pragma once

#include "FxRack.h"
#include "GrooveEngine.h"
#include "SectionEq.h"
#include "SessionState.h"

#include <array>
#include <juce_audio_utils/juce_audio_utils.h>

class ShapedSampleSource final : public juce::PositionableAudioSource
{
public:
    void setBuffer(juce::AudioBuffer<float> newBuffer, double nativeRate);
    void setRack(FxRack* rackToUse) { rack = rackToUse; }

    void prepareToPlay(int, double) override {}
    void releaseResources() override {}
    void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override;
    void setNextReadPosition(juce::int64) override;
    juce::int64 getNextReadPosition() const override;
    juce::int64 getTotalLength() const override;
    void setLooping(bool shouldLoop) override { looping = shouldLoop; }
    bool isLooping() const override { return looping; }
    void setPitchRatio(double ratio) { pitchRatio.store(ratio); }
    void setTrimEnd(float amount01) { trimEnd.store(juce::jlimit(0.05f, 1.0f, amount01)); }
    void setFadeIn(float amount01) { fadeIn.store(juce::jlimit(0.0f, 1.0f, amount01)); }
    void setFadeOut(float amount01) { fadeOut.store(juce::jlimit(0.0f, 1.0f, amount01)); }
    bool takeReachedEnd() { return reachedEnd.exchange(false); }

private:
    juce::AudioBuffer<float> buffer;
    FxRack* rack = nullptr;
    double position = 0.0;
    double sourceRate = 44100.0;
    std::atomic<double> pitchRatio{ 1.0 };
    std::atomic<float> trimEnd{ 1.0f };
    std::atomic<float> fadeIn{ 0.0f };
    std::atomic<float> fadeOut{ 0.0f };
    std::atomic<bool> reachedEnd{ false };
    bool looping = true;
};

class SampleLayerBank
{
public:
    static constexpr int slotCount = 4;

    void prepare(double sampleRate);
    bool loadSlot(int slot, const juce::File& file,
                  juce::AudioFormatManager& formats);
    void start();
    void stop();
    void process(juce::AudioBuffer<float>& output);
    void setVolume(int slot, float volume);
    void setMuted(int slot, bool shouldMute);
    void setPitchSemitones(int slot, double semitones);
    void setLooping(bool shouldLoop);
    bool isPlaying() const { return playing.load(); }
    bool isLooping() const { return looping.load(); }
    bool isMuted(int slot) const;
    float getLed(int slot) const;
    bool hasSample(int slot) const;
    juce::String getSlotName(int slot) const;
    juce::String getSlotPath(int slot) const;
    float getVolume(int slot) const;
    double getPitchSemitones(int slot) const;

private:
    struct Slot
    {
        juce::AudioBuffer<float> audio;
        juce::String path;
        juce::String name;
        double sourceRate = 44100.0;
        double position = 0.0;
        std::atomic<float> volume{ 1.0f };
        std::atomic<double> pitchSemitones{ 0.0 };
        std::atomic<bool> loaded{ false };
        std::atomic<bool> muted{ false };
        std::atomic<float> led{ 0.0f };
    };

    mutable juce::CriticalSection lock;
    std::array<Slot, slotCount> slots;
    std::atomic<bool> playing{ false };
    std::atomic<bool> looping{ true };
    double hostSampleRate = 44100.0;
};

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
    bool loadLayerSample(int slot, const juce::File& file);
    void playSample();
    void stopSamplePlayback();
    void toggleSamplePlayback();
    void toggleCapture();
    int getCaptureState() const { return captureState.load(); }
    bool takeCompletedCapture(juce::File& fileOut);
    void stopPreviews();
    void setSampleLooping(bool shouldLoop);
    void setSamplePitchSemitones(double semitones);
    void setSampleTrim(float amount01);
    void setSampleFadeIn(float amount01);
    void setSampleFadeOut(float amount01);
    bool isSampleLooping() const { return shapedSource.isLooping(); }
    bool isSamplePlaying() const { return sampleTransport.isPlaying(); }
    static constexpr int sampleEqBandCount = 3;
    void setSampleEqGain(int band, float decibels);
    float getSampleEqGain(int band) const;
    void startMidiPreview(const juce::MidiFile&, double bpm);
    bool isMidiPreviewPlaying() const { return midiPreviewPlaying.load(); }
    double getMidiPreviewProgress() const
    {
        return midiPreviewProgress.load();
    }
    SampleLayerBank& getLayerBank() { return layerBank; }
    GrooveEngine& getGrooveEngine() { return grooveEngine; }
    BroncoPianoEngine& getPianoEngine() { return pianoEngine; }
    SectionEqBank& getSectionEqBank() { return sectionEqBank; }
    FxRack& getFxRack() { return fxRack; }

    void setUiSessionState(const juce::ValueTree& state);
    juce::ValueTree getUiSessionState() const;
    juce::ValueTree buildFullSessionState() const;
    void restoreFullSessionState(const juce::ValueTree& session);
    void saveAutosaveSession();
    bool loadAutosaveSession();

private:
    void addPreviewMidi(juce::MidiBuffer&, int numSamples);
    void updateSampleEqLocked();
    void applySampleEq(juce::AudioBuffer<float>& buffer);

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
    ShapedSampleSource shapedSource;
    mutable juce::CriticalSection sampleEqLock;
    std::array<std::array<juce::IIRFilter, sampleEqBandCount>, 2>
        sampleEqFilters;
    std::array<float, sampleEqBandCount> sampleEqGains{};
    juce::AudioBuffer<float> samplePreviewBuffer;
    juce::AudioBuffer<float> sectionBuffer;
    juce::AudioBuffer<float> captureBuffer;
    juce::CriticalSection captureLock;
    std::atomic<int> captureState{ 0 };
    std::atomic<int> captureWritten{ 0 };
    std::atomic<bool> captureStopRequest{ false };
    std::atomic<bool> captureHeardSample{ false };
    double lastCapturePressMs = 0.0;
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
    SampleLayerBank layerBank;
    GrooveEngine grooveEngine;
    BroncoPianoEngine pianoEngine;
    SectionEqBank sectionEqBank;
    FxRack fxRack;
    juce::ValueTree uiSessionState{ "UiSession" };
    mutable juce::CriticalSection sessionLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiguelMusicAssistantAudioProcessor)
};
