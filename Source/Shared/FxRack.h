#pragma once

#include <array>
#include <atomic>
#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>

enum class FxSlot
{
    filterHp = 0,
    filterLp,
    compressor,
    exciter,
    doubler,
    distortion,
    delay,
    efecto,
    volume,
    velocity,
    count
};

class FxRack
{
public:
    static constexpr int slotCount = static_cast<int>(FxSlot::count);

    FxRack();
    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer);

    void setAmount(FxSlot slot, float amount01);
    float getAmount(FxSlot slot) const;
    void setMuted(FxSlot slot, bool muted);
    bool isMuted(FxSlot slot) const;
    float getLed(FxSlot slot) const;
    float getInputLed(int channel) const;
    float getOutputLed(int channel) const;

    void setMasterMute(bool shouldMute);
    bool isMasterMute() const { return masterMute.load(); }

    float velocityGain() const;
    float volumeGain() const;

    juce::ValueTree toTree() const;
    void fromTree(const juce::ValueTree& tree);
    void processOnly(FxSlot slot, juce::AudioBuffer<float>& buffer);

private:
    void updateLeds(FxSlot slot, const juce::AudioBuffer<float>& buffer,
                    int samples);
    void decayLeds();
    bool active(FxSlot slot) const;

    std::array<std::atomic<float>, slotCount> amounts{};
    std::array<std::atomic<bool>, slotCount> mutes{};
    std::array<std::atomic<float>, slotCount> leds{};
    std::array<std::atomic<float>, 2> inputLeds{};
    std::array<std::atomic<float>, 2> outputLeds{};
    std::atomic<bool> masterMute{ false };

    juce::AudioBuffer<float> dryBuffer;
    double activeSampleRate = 44100.0;
    juce::IIRFilter hp[2];
    juce::IIRFilter lp[2];
    float lastHp = -1.0f;
    float lastLp = -1.0f;
    float compEnv[2]{};
    std::array<std::array<float, 48000>, 2> delayLine{};
    int delayWrite = 0;
    std::array<std::array<float, 4096>, 2> doubleDelay{};
    int doubleWrite = 0;
    double doublePhase = 0.0;
};
