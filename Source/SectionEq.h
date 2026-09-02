#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

enum class AudioSection
{
    generator = 0,
    samples,
    chords,
    rhythms,
    piano,
    count
};

class SectionEq
{
public:
    static constexpr int bandCount = 7;
    static constexpr std::array<double, bandCount> frequencies{
        60.0, 120.0, 250.0, 500.0, 1000.0, 4000.0, 10000.0
    };

    void prepare(double sampleRate);
    void process(juce::AudioBuffer<float>& buffer);
    bool isBypassed() const;
    void setBandGain(bool outputStage, int band, float decibels);
    float getBandGain(bool outputStage, int band) const;
    void setVolume(float gain);
    float getVolume() const { return volume.load(); }
    void setBroncoMax(float amount);
    float getBroncoMax() const { return broncoMax.load(); }

private:
    void updateBandLocked(bool outputStage, int band);

    mutable juce::CriticalSection lock;
    std::array<float, bandCount> inputGains{};
    std::array<float, bandCount> outputGains{};
    std::array<std::array<juce::IIRFilter, bandCount>, 2> inputFilters;
    std::array<std::array<juce::IIRFilter, bandCount>, 2> outputFilters;
    std::atomic<float> volume{ 1.0f };
    std::atomic<float> broncoMax{ 0.0f };
    std::array<std::array<float, 4096>, 2> broncoDelay{};
    std::array<float, 2> broncoEnvelope{};
    int broncoWritePosition = 0;
    double broncoPhase = 0.0;
    double activeSampleRate = 44100.0;
};

class SectionEqBank
{
public:
    void prepare(double sampleRate);
    void process(AudioSection section, juce::AudioBuffer<float>& buffer);
    SectionEq& get(AudioSection section);
    const SectionEq& get(AudioSection section) const;

private:
    std::array<SectionEq, static_cast<size_t>(AudioSection::count)> sections;
};
