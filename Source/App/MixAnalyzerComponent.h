#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>

class MixAnalyzerComponent final : public juce::Component
{
public:
    MixAnalyzerComponent();

    void setLevels(float leftRmsDb, float rightRmsDb,
                   float leftPeakDb, float rightPeakDb,
                   float correlation);
    void pushAudio(const float* samples, int count);
    void paint(juce::Graphics&) override;

private:
    static constexpr int fftOrder = 10;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int waveformSize = 512;

    void calculateSpectrum();
    static float dbToProportion(float db);

    juce::dsp::FFT fft{ fftOrder };
    juce::dsp::WindowingFunction<float> window{
        fftSize, juce::dsp::WindowingFunction<float>::hann, true
    };
    std::array<float, fftSize * 2> fftData{};
    std::array<float, fftSize / 2> spectrum{};
    std::array<float, waveformSize> waveform{};
    int fftWritePosition = 0;
    int waveformWritePosition = 0;
    float leftRms = -100.0f;
    float rightRms = -100.0f;
    float leftPeak = -100.0f;
    float rightPeak = -100.0f;
    float stereoCorrelation = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixAnalyzerComponent)
};
