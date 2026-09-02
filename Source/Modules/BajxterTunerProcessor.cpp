#include "BajxterTunerProcessor.h"
#include "BajxterTunerEditor.h"

#include <cmath>
#include <vector>

namespace
{
const char* noteName(int pitchClass)
{
    static constexpr const char* names[] = {
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };
    return names[(pitchClass % 12 + 12) % 12];
}
}

BajxterTunerAudioProcessor::BajxterTunerAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("In", juce::AudioChannelSet::stereo(), true)
          .withOutput("Out", juce::AudioChannelSet::stereo(), true))
{
}

void BajxterTunerAudioProcessor::prepareToPlay(double sampleRate, int)
{
    activeSampleRate = juce::jmax(8000.0, sampleRate);
    const juce::ScopedLock sl(lock);
    capture.setSize(1, 8192, false, true, true);
    writePos = 0;
    samplesUntilAnalyse = 0;
}

bool BajxterTunerAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return (output == juce::AudioChannelSet::mono()
            || output == juce::AudioChannelSet::stereo())
        && output == layouts.getMainInputChannelSet();
}

void BajxterTunerAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto samples = buffer.getNumSamples();
    if (samples <= 0)
        return;

    {
        const juce::ScopedLock sl(lock);
        auto* dest = capture.getWritePointer(0);
        const auto* src = buffer.getReadPointer(0);
        const auto cap = capture.getNumSamples();
        for (int i = 0; i < samples; ++i)
        {
            dest[writePos] = src[i];
            writePos = (writePos + 1) % cap;
        }
        samplesUntilAnalyse += samples;
        if (samplesUntilAnalyse >= static_cast<int>(activeSampleRate * 0.12))
        {
            samplesUntilAnalyse = 0;
            analyseLocked();
        }
    }
}

void BajxterTunerAudioProcessor::analyseLocked()
{
    const auto total = capture.getNumSamples();
    const auto windowSize = juce::jmin(4096, total);
    std::vector<double> signal(static_cast<size_t>(windowSize));
    auto mean = 0.0;
    auto energy = 0.0;
    auto start = writePos - windowSize;
    while (start < 0)
        start += total;
    for (int i = 0; i < windowSize; ++i)
    {
        const auto value = capture.getSample(0, (start + i) % total);
        signal[static_cast<size_t>(i)] = value;
        mean += value;
        energy += value * value;
    }
    if (energy / windowSize < 1.0e-6)
    {
        hasPitch.store(false);
        return;
    }
    mean /= windowSize;
    for (auto& sample : signal)
        sample -= mean;

    const auto minLag = juce::jmax(2, static_cast<int>(activeSampleRate / 2000.0));
    const auto maxLag = juce::jmin(windowSize / 2,
                                   static_cast<int>(activeSampleRate / 40.0));
    if (minLag >= maxLag)
    {
        hasPitch.store(false);
        return;
    }

    std::vector<double> difference(static_cast<size_t>(maxLag + 1), 0.0);
    std::vector<double> normalized(static_cast<size_t>(maxLag + 1), 1.0);
    const auto comparisonLength = windowSize - maxLag;
    for (int lag = 1; lag <= maxLag; ++lag)
    {
        auto sum = 0.0;
        for (int sample = 0; sample < comparisonLength; ++sample)
        {
            const auto delta = signal[static_cast<size_t>(sample)]
                - signal[static_cast<size_t>(sample + lag)];
            sum += delta * delta;
        }
        difference[static_cast<size_t>(lag)] = sum;
    }
    auto runningSum = 0.0;
    for (int lag = 1; lag <= maxLag; ++lag)
    {
        runningSum += difference[static_cast<size_t>(lag)];
        normalized[static_cast<size_t>(lag)] = runningSum > 0.0
            ? difference[static_cast<size_t>(lag)] * lag / runningSum
            : 1.0;
    }
    auto candidate = minLag;
    for (int lag = minLag + 1; lag <= maxLag; ++lag)
        if (normalized[static_cast<size_t>(lag)]
            < normalized[static_cast<size_t>(candidate)])
            candidate = lag;
    if (1.0 - normalized[static_cast<size_t>(candidate)] < 0.55)
    {
        hasPitch.store(false);
        return;
    }
    auto refinedLag = static_cast<double>(candidate);
    const auto freq = activeSampleRate / juce::jmax(1.0, refinedLag);
    const auto midi = 69.0 + 12.0 * std::log2(freq / 440.0);
    const auto nearest = static_cast<int>(std::round(midi));
    frequency.store(freq);
    cents.store((midi - nearest) * 100.0);
    note = juce::String(noteName((nearest % 12 + 12) % 12))
        + juce::String(nearest / 12 - 1);
    hasPitch.store(true);
}

juce::String BajxterTunerAudioProcessor::getNote() const
{
    const juce::ScopedLock sl(lock);
    return note;
}

juce::AudioProcessorEditor* BajxterTunerAudioProcessor::createEditor()
{
    return new BajxterTunerEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BajxterTunerAudioProcessor();
}
