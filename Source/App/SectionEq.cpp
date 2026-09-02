#include "SectionEq.h"

#include <cmath>

void SectionEq::prepare(double sampleRate)
{
    const juce::ScopedLock scoped(lock);
    activeSampleRate = sampleRate;
    for (auto& delay : broncoDelay)
        delay.fill(0.0f);
    broncoEnvelope.fill(0.0f);
    broncoWritePosition = 0;
    broncoPhase = 0.0;
    for (int band = 0; band < bandCount; ++band)
    {
        updateBandLocked(false, band);
        updateBandLocked(true, band);
        for (int channel = 0; channel < 2; ++channel)
        {
            inputFilters[static_cast<size_t>(channel)]
                        [static_cast<size_t>(band)].reset();
            outputFilters[static_cast<size_t>(channel)]
                         [static_cast<size_t>(band)].reset();
        }
    }
}

void SectionEq::updateBandLocked(bool outputStage, int band)
{
    if (!juce::isPositiveAndBelow(band, bandCount))
        return;
    const auto gainDb = outputStage
        ? outputGains[static_cast<size_t>(band)]
        : inputGains[static_cast<size_t>(band)];
    const auto coefficients = juce::IIRCoefficients::makePeakFilter(
        activeSampleRate, frequencies[static_cast<size_t>(band)], 1.0,
        juce::Decibels::decibelsToGain(gainDb));
    auto& filters = outputStage ? outputFilters : inputFilters;
    for (auto& channel : filters)
        channel[static_cast<size_t>(band)].setCoefficients(coefficients);
}

void SectionEq::process(juce::AudioBuffer<float>& buffer)
{
    if (isBypassed())
        return;
    const juce::ScopedTryLock scoped(lock);
    if (!scoped.isLocked())
        return;
    const auto channels = juce::jmin(2, buffer.getNumChannels());
    const auto samples = buffer.getNumSamples();
    for (int channel = 0; channel < channels; ++channel)
    {
        auto* data = buffer.getWritePointer(channel);
        for (auto& filter : inputFilters[static_cast<size_t>(channel)])
            filter.processSamples(data, samples);
        for (auto& filter : outputFilters[static_cast<size_t>(channel)])
            filter.processSamples(data, samples);
    }

    const auto broncoMix = broncoMax.load();
    if (broncoMix > 0.0f)
    {
        const auto baseDelay = static_cast<int>(activeSampleRate * 0.012);
        for (int sample = 0; sample < samples; ++sample)
        {
            const auto modulation = std::sin(broncoPhase) * 2.0;
            broncoPhase += juce::MathConstants<double>::twoPi
                * 0.8 / activeSampleRate;
            if (broncoPhase >= juce::MathConstants<double>::twoPi)
                broncoPhase -= juce::MathConstants<double>::twoPi;

            for (int channel = 0; channel < channels; ++channel)
            {
                auto* data = buffer.getWritePointer(channel);
                const auto dry = data[sample];
                auto& envelope = broncoEnvelope[static_cast<size_t>(channel)];
                const auto previousEnvelope = envelope;
                envelope = juce::jmax(
                    std::abs(dry), envelope * 0.995f);
                const auto transient = std::abs(dry)
                    > previousEnvelope * 1.25f
                    ? 2.1f : 1.15f;

                auto& delay = broncoDelay[static_cast<size_t>(channel)];
                delay[static_cast<size_t>(broncoWritePosition)] = dry;
                auto readPosition = broncoWritePosition - baseDelay
                    - static_cast<int>(std::round(modulation
                        * (channel == 0 ? 1.0 : -1.0)));
                while (readPosition < 0)
                    readPosition += static_cast<int>(delay.size());
                const auto doubled =
                    delay[static_cast<size_t>(readPosition)];
                const auto wet = std::tanh(
                    (dry * transient + doubled * 1.15f) * (1.35f + broncoMix));
                data[sample] = dry * (1.0f - broncoMix)
                    + wet * broncoMix;
            }
            broncoWritePosition =
                (broncoWritePosition + 1)
                % static_cast<int>(broncoDelay[0].size());
        }
    }
    buffer.applyGain(volume.load());
}

void SectionEq::setBandGain(bool outputStage, int band, float decibels)
{
    if (!juce::isPositiveAndBelow(band, bandCount))
        return;
    const juce::ScopedLock scoped(lock);
    auto& gains = outputStage ? outputGains : inputGains;
    gains[static_cast<size_t>(band)] =
        juce::jlimit(-18.0f, 18.0f, decibels);
    updateBandLocked(outputStage, band);
}

float SectionEq::getBandGain(bool outputStage, int band) const
{
    if (!juce::isPositiveAndBelow(band, bandCount))
        return 0.0f;
    const juce::ScopedLock scoped(lock);
    return (outputStage ? outputGains : inputGains)
        [static_cast<size_t>(band)];
}

void SectionEq::setVolume(float gain)
{
    volume.store(juce::jlimit(0.0f, 2.0f, gain));
}

void SectionEq::setBroncoMax(float amount)
{
    broncoMax.store(juce::jlimit(0.0f, 1.0f, amount));
}

bool SectionEq::isBypassed() const
{
    if (std::abs(volume.load() - 1.0f) > 0.001f)
        return false;
    if (broncoMax.load() > 0.001f)
        return false;
    const juce::ScopedLock scoped(lock);
    for (int band = 0; band < bandCount; ++band)
    {
        if (std::abs(inputGains[static_cast<size_t>(band)]) > 0.01f
            || std::abs(outputGains[static_cast<size_t>(band)]) > 0.01f)
            return false;
    }
    return true;
}

void SectionEqBank::prepare(double sampleRate)
{
    for (auto& section : sections)
        section.prepare(sampleRate);
}

void SectionEqBank::process(AudioSection section,
                            juce::AudioBuffer<float>& buffer)
{
    get(section).process(buffer);
}

SectionEq& SectionEqBank::get(AudioSection section)
{
    return sections[static_cast<size_t>(section)];
}

const SectionEq& SectionEqBank::get(AudioSection section) const
{
    return sections[static_cast<size_t>(section)];
}
