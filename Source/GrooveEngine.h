#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

struct LoadedAudioSample
{
    juce::AudioBuffer<float> audio;
    double sampleRate = 44100.0;
    juce::String name;
};

class GrooveEngine
{
public:
    static constexpr int channelCount = 8;
    static constexpr int stepCount = 256;
    static constexpr int trackEqBandCount = 3;

    void prepare(double sampleRate);
    void process(juce::AudioBuffer<float>& output);
    bool loadSample(int channel, const juce::File& file);
    void setStep(int channel, int step, bool enabled);
    bool getStep(int channel, int step) const;
    void clearPattern();
    void setGain(int channel, float gain);
    float getGain(int channel) const;
    juce::String getSampleName(int channel) const;
    void setBpm(double newBpm);
    double getBpm() const { return bpm.load(); }
    void setLoopLength(int steps);
    int getLoopLength() const { return loopLength.load(); }
    void setGridResolution(int denominator);
    int getGridResolution() const { return gridResolution.load(); }
    int getStepsPerBeat() const { return stepsPerBeat.load(); }
    void setTrackEqGain(int channel, int band, float decibels);
    float getTrackEqGain(int channel, int band) const;
    void start();
    void stop();
    bool isPlaying() const { return playing.load(); }
    int getCurrentStep() const { return currentStep.load(); }
    bool exportLoop(const juce::File& destination, int bars = 4);
    juce::String getSamplePath(int channel) const;
    juce::String getPatternData() const;
    void setPatternData(const juce::String& data);

private:
    struct Voice
    {
        double position = -1.0;
    };

    struct ProcessSnapshot
    {
        std::array<std::shared_ptr<LoadedAudioSample>, channelCount> samples;
        std::array<std::array<bool, stepCount>, channelCount> pattern{};
        std::array<float, channelCount> gains{};
        bool valid = false;
    };

    void renderVoiceBlock(int source, int frameOffset, int numSamples,
                          double rateRatio, juce::AudioBuffer<float>& output);

    mutable juce::CriticalSection lock;
    std::array<std::shared_ptr<LoadedAudioSample>, channelCount> samples;
    std::array<juce::String, channelCount> samplePaths;
    std::array<std::array<bool, stepCount>, channelCount> pattern{};
    std::array<float, channelCount> gains{
        0.85f, 0.85f, 0.85f, 0.85f, 0.85f, 0.85f, 0.85f, 0.85f
    };
    std::array<Voice, channelCount> voices;
    std::array<std::array<std::array<juce::IIRFilter, trackEqBandCount>, 2>,
               channelCount> trackEqFilters;
    std::array<std::array<float, trackEqBandCount>, channelCount>
        trackEqGains{};
    std::atomic<double> bpm{ 120.0 };
    std::atomic<int> loopLength{ 32 };
    std::atomic<int> gridResolution{ 8 };
    std::atomic<int> stepsPerBeat{ 2 };
    std::atomic<bool> playing{ false };
    std::atomic<int> currentStep{ -1 };
    double hostSampleRate = 44100.0;
    double samplesUntilNextStep = 0.0;
    juce::AudioBuffer<float> renderScratch;
    ProcessSnapshot lastSnapshot;

    void updateTrackEqLocked(int channel);
};

class BroncoPianoEngine
{
public:
    struct RecordedNote
    {
        int midiNote = 60;
        float velocity = 0.8f;
        double startSeconds = 0.0;
    };

    void prepare(double sampleRate);
    void process(juce::AudioBuffer<float>& output);
    int loadLibrary(const juce::File& folder);
    void noteOn(int midiNote, float velocity);
    void startRecording();
    void stopRecording();
    bool isRecording() const { return recording.load(); }
    void playRecording();
    void stopPlayback();
    bool isPlayingRecording() const { return playback.load(); }
    int getRecordedNoteCount() const;
    bool exportRecording(const juce::File& destination);

private:
    struct Voice
    {
        std::shared_ptr<LoadedAudioSample> sample;
        double position = 0.0;
        float velocity = 1.0f;
        bool active = false;
    };

    void triggerNoteLocked(int midiNote, float velocity);

    mutable juce::CriticalSection lock;
    std::array<std::shared_ptr<LoadedAudioSample>, 128> notes;
    std::array<Voice, 20> voices;
    std::vector<RecordedNote> recordingEvents;
    std::atomic<bool> recording{ false };
    std::atomic<bool> playback{ false };
    double hostSampleRate = 44100.0;
    double recordStartTimeMs = 0.0;
    double playbackPositionSeconds = 0.0;
    size_t nextPlaybackEvent = 0;
};
