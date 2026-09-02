#include "FxRack.h"

#include <cmath>

namespace
{
float peakOf(const juce::AudioBuffer<float>& buffer, int samples)
{
    auto peak = 0.0f;
    const auto channels = juce::jmin(2, buffer.getNumChannels());
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < samples; ++i)
            peak = juce::jmax(peak, std::abs(data[i]));
    }
    return peak;
}

float slotPeak(const juce::AudioBuffer<float>& buffer, int samples, int channel)
{
    if (channel >= buffer.getNumChannels())
        return 0.0f;
    auto peak = 0.0f;
    auto* data = buffer.getReadPointer(channel);
    for (int i = 0; i < samples; ++i)
        peak = juce::jmax(peak, std::abs(data[i]));
    return peak;
}

const char* slotName(int index)
{
    static constexpr const char* names[] = {
        "filterHp", "filterLp", "compressor", "exciter", "doubler",
        "distortion", "delay", "efecto",
        "volume", "velocity"
    };
    return names[index];
}
}

FxRack::FxRack()
{
    amounts[static_cast<size_t>(FxSlot::volume)].store(1.0f);
    amounts[static_cast<size_t>(FxSlot::velocity)].store(0.49f);
    amounts[static_cast<size_t>(FxSlot::efecto)].store(0.0f);
}

void FxRack::prepare(double sampleRate, int samplesPerBlock)
{
    activeSampleRate = juce::jmax(8000.0, sampleRate);
    dryBuffer.setSize(2, juce::jmax(samplesPerBlock, 32), false, false, true);
    for (auto& filter : hp)
        filter.reset();
    for (auto& filter : lp)
        filter.reset();
    lastHp = -1.0f;
    lastLp = -1.0f;
    compEnv[0] = compEnv[1] = 0.0f;
    for (auto& line : delayLine)
        line.fill(0.0f);
    delayWrite = 0;
    for (auto& line : doubleDelay)
        line.fill(0.0f);
    doubleWrite = 0;
    doublePhase = 0.0;
    reverb.reset();
    juce::Reverb::Parameters params;
    params.roomSize = 0.35f;
    params.damping = 0.45f;
    params.wetLevel = 0.0f;
    params.dryLevel = 1.0f;
    params.width = 1.0f;
    reverb.setParameters(params);
    reverb.setSampleRate(activeSampleRate);
}

void FxRack::setAmount(FxSlot slot, float amount01)
{
    amounts[static_cast<size_t>(slot)].store(juce::jlimit(0.0f, 1.0f, amount01));
}

float FxRack::getAmount(FxSlot slot) const
{
    return amounts[static_cast<size_t>(slot)].load();
}

void FxRack::setMuted(FxSlot slot, bool muted)
{
    mutes[static_cast<size_t>(slot)].store(muted);
}

bool FxRack::isMuted(FxSlot slot) const
{
    return mutes[static_cast<size_t>(slot)].load();
}

float FxRack::getLed(FxSlot slot) const
{
    return leds[static_cast<size_t>(slot)].load();
}

float FxRack::getInputLed(int channel) const
{
    return inputLeds[static_cast<size_t>(juce::jlimit(0, 1, channel))].load();
}

float FxRack::getOutputLed(int channel) const
{
    return outputLeds[static_cast<size_t>(juce::jlimit(0, 1, channel))].load();
}

void FxRack::setMasterMute(bool shouldMute)
{
    masterMute.store(shouldMute);
}

float FxRack::velocityGain() const
{
    return 0.05f + getAmount(FxSlot::velocity) * 1.95f;
}

float FxRack::volumeGain() const
{
    return getAmount(FxSlot::volume) * 1.4f;
}

bool FxRack::active(FxSlot slot) const
{
    return !isMuted(slot) && getAmount(slot) > 0.001f;
}

void FxRack::applyDelay(juce::AudioBuffer<float>& buffer)
{
    const auto channels = juce::jmin(2, buffer.getNumChannels());
    const auto samples = buffer.getNumSamples();
    const auto amount = getAmount(FxSlot::delay);
    const auto delayMs = 8.0f + amount * 28.0f;
    const auto maxIndex = static_cast<int>(delayLine[0].size());
    const auto delaySamples = juce::jlimit(
        1, maxIndex - 2,
        static_cast<int>(activeSampleRate * delayMs * 0.001));
    const auto feedback = 0.05f + amount * 0.14f;
    const auto mix = 0.28f + amount * 0.32f;
    for (int i = 0; i < samples; ++i)
    {
        auto read = delayWrite - delaySamples;
        while (read < 0)
            read += maxIndex;
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            auto& line = delayLine[static_cast<size_t>(ch)];
            const auto delayed = line[static_cast<size_t>(read)];
            line[static_cast<size_t>(delayWrite)] =
                data[i] + delayed * feedback;
            data[i] = data[i] * (1.0f - mix) + delayed * mix;
        }
        delayWrite = (delayWrite + 1) % maxIndex;
    }
    updateLeds(FxSlot::delay, buffer, samples);
}

void FxRack::applyReverb(juce::AudioBuffer<float>& buffer)
{
    const auto channels = juce::jmin(2, buffer.getNumChannels());
    const auto samples = buffer.getNumSamples();
    const auto amount = getAmount(FxSlot::efecto);
    juce::Reverb::Parameters params;
    params.roomSize = juce::jlimit(
        0.0f, 1.0f, (0.22f + amount * 0.68f) * 1.30f);
    params.damping = 0.68f - amount * 0.10f;
    params.wetLevel = 0.07f + amount * 0.22f;
    params.dryLevel = 1.0f;
    params.width = 0.72f;
    params.freezeMode = 0.0f;
    reverb.setParameters(params);
    if (channels >= 2)
        reverb.processStereo(buffer.getWritePointer(0),
                             buffer.getWritePointer(1), samples);
    else
        reverb.processMono(buffer.getWritePointer(0), samples);
    buffer.applyGain(1.0f / (1.0f + params.wetLevel));
    updateLeds(FxSlot::efecto, buffer, samples);
}

void FxRack::updateLeds(FxSlot slot, const juce::AudioBuffer<float>& buffer,
                        int samples)
{
    const auto peak = peakOf(buffer, samples);
    auto& led = leds[static_cast<size_t>(slot)];
    led.store(juce::jmax(led.load() * 0.72f, juce::jmin(1.0f, peak * 3.2f)));
}

void FxRack::decayLeds()
{
    for (auto& led : leds)
        led.store(led.load() * 0.86f);
    for (auto& led : inputLeds)
        led.store(led.load() * 0.86f);
    for (auto& led : outputLeds)
        led.store(led.load() * 0.86f);
}

void FxRack::process(juce::AudioBuffer<float>& buffer)
{
    const auto channels = juce::jmin(2, buffer.getNumChannels());
    const auto samples = buffer.getNumSamples();
    if (channels <= 0 || samples <= 0)
        return;

    decayLeds();
    for (int ch = 0; ch < channels; ++ch)
        inputLeds[static_cast<size_t>(ch)].store(juce::jmax(
            inputLeds[static_cast<size_t>(ch)].load() * 0.7f,
            juce::jmin(1.0f, slotPeak(buffer, samples, ch) * 3.0f)));

    if (masterMute.load())
    {
        buffer.clear();
        return;
    }

    if (active(FxSlot::filterHp) || active(FxSlot::filterLp))
    {
        const auto hpAmt = getAmount(FxSlot::filterHp);
        const auto lpAmt = getAmount(FxSlot::filterLp);
        const auto hpHz = 40.0f + hpAmt * 360.0f;
        const auto lpHz = 1800.0f + lpAmt * 14000.0f;
        if (std::abs(hpHz - lastHp) > 0.5f)
        {
            lastHp = hpHz;
            const auto coef = juce::IIRCoefficients::makeHighPass(
                activeSampleRate, hpHz);
            hp[0].setCoefficients(coef);
            hp[1].setCoefficients(coef);
        }
        if (std::abs(lpHz - lastLp) > 0.5f)
        {
            lastLp = lpHz;
            const auto coef = juce::IIRCoefficients::makeLowPass(
                activeSampleRate, lpHz);
            lp[0].setCoefficients(coef);
            lp[1].setCoefficients(coef);
        }
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            if (active(FxSlot::filterHp))
                hp[ch].processSamples(data, samples);
            if (active(FxSlot::filterLp))
                lp[ch].processSamples(data, samples);
        }
        updateLeds(FxSlot::filterHp, buffer, samples);
        updateLeds(FxSlot::filterLp, buffer, samples);
    }

    if (active(FxSlot::distortion))
    {
        const auto drive = 1.0f + getAmount(FxSlot::distortion) * 9.0f;
        const auto norm = std::tanh(drive);
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < samples; ++i)
            {
                const auto x = data[i];
                const auto odd = x + 0.12f * x * std::abs(x);
                data[i] = std::tanh(odd * drive) / norm;
            }
        }
        updateLeds(FxSlot::distortion, buffer, samples);
    }

    if (active(FxSlot::doubler))
    {
        const auto mix = getAmount(FxSlot::doubler);
        const auto baseDelay = static_cast<int>(activeSampleRate * 0.0024);
        const auto lineSize = static_cast<int>(doubleDelay[0].size());
        for (int i = 0; i < samples; ++i)
        {
            const auto modulation = std::sin(doublePhase) * 2.4;
            doublePhase += juce::MathConstants<double>::twoPi
                * 0.7 / activeSampleRate;
            if (doublePhase >= juce::MathConstants<double>::twoPi)
                doublePhase -= juce::MathConstants<double>::twoPi;
            for (int ch = 0; ch < channels; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                const auto dry = data[i];
                auto& line = doubleDelay[static_cast<size_t>(ch)];
                line[static_cast<size_t>(doubleWrite)] = dry;
                auto read = doubleWrite - baseDelay
                    - static_cast<int>(std::round(
                        modulation * (ch == 0 ? 1.0 : -1.0)));
                while (read < 0)
                    read += lineSize;
                const auto doubled = line[static_cast<size_t>(read)];
                data[i] = dry * (1.0f - mix)
                    + std::tanh(dry + doubled * 0.72f) * mix;
            }
            doubleWrite = (doubleWrite + 1) % lineSize;
        }
        updateLeds(FxSlot::doubler, buffer, samples);
    }

    if (active(FxSlot::delay))
        applyDelay(buffer);

    if (active(FxSlot::exciter))
    {
        const auto amount = getAmount(FxSlot::exciter) * 0.35f;
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            float prev = 0.0f;
            for (int i = 0; i < samples; ++i)
            {
                const auto high = data[i] - prev;
                prev = data[i];
                data[i] += amount * std::tanh(high * 3.2f);
            }
        }
        updateLeds(FxSlot::exciter, buffer, samples);
    }

    if (active(FxSlot::compressor))
    {
        const auto amount = getAmount(FxSlot::compressor);
        const auto thresh = 0.28f - amount * 0.18f;
        const auto ratio = 1.4f + amount * 4.0f;
        const auto atk = std::exp(-1.0 / (activeSampleRate * 0.008));
        const auto rel = std::exp(-1.0 / (activeSampleRate * 0.12));
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            auto& env = compEnv[ch];
            for (int i = 0; i < samples; ++i)
            {
                const auto absx = std::abs(data[i]);
                env = absx > env
                    ? static_cast<float>(atk) * env
                        + static_cast<float>(1.0 - atk) * absx
                    : static_cast<float>(rel) * env
                        + static_cast<float>(1.0 - rel) * absx;
                auto gain = 1.0f;
                if (env > thresh)
                    gain = (thresh + (env - thresh) / ratio) / (env + 1.0e-5f);
                data[i] *= gain * (1.0f + amount * 0.18f);
            }
        }
        updateLeds(FxSlot::compressor, buffer, samples);
    }

    if (active(FxSlot::efecto))
        applyReverb(buffer);

    if (!isMuted(FxSlot::volume))
        buffer.applyGain(volumeGain());
    else
        buffer.clear();
    updateLeds(FxSlot::volume, buffer, samples);
    updateLeds(FxSlot::velocity, buffer, samples);

    for (int ch = 0; ch < channels; ++ch)
        outputLeds[static_cast<size_t>(ch)].store(juce::jmax(
            outputLeds[static_cast<size_t>(ch)].load() * 0.7f,
            juce::jmin(1.0f, slotPeak(buffer, samples, ch) * 3.0f)));
}

void FxRack::processOnly(FxSlot slot, juce::AudioBuffer<float>& buffer)
{
    const auto channels = juce::jmin(2, buffer.getNumChannels());
    const auto samples = buffer.getNumSamples();
    if (channels <= 0 || samples <= 0)
        return;

    decayLeds();
    for (int ch = 0; ch < channels; ++ch)
        inputLeds[static_cast<size_t>(ch)].store(juce::jmax(
            inputLeds[static_cast<size_t>(ch)].load() * 0.7f,
            juce::jmin(1.0f, slotPeak(buffer, samples, ch) * 3.0f)));

    if (masterMute.load())
    {
        buffer.clear();
        return;
    }

    switch (slot)
    {
        case FxSlot::filterHp:
        case FxSlot::filterLp:
        {
            const auto hpAmt = getAmount(FxSlot::filterHp);
            const auto lpAmt = getAmount(FxSlot::filterLp);
            const auto hpHz = 40.0f + hpAmt * 360.0f;
            const auto lpHz = 1800.0f + lpAmt * 14000.0f;
            if (slot == FxSlot::filterHp && std::abs(hpHz - lastHp) > 0.5f)
            {
                lastHp = hpHz;
                const auto coef = juce::IIRCoefficients::makeHighPass(
                    activeSampleRate, hpHz);
                hp[0].setCoefficients(coef);
                hp[1].setCoefficients(coef);
            }
            if (slot == FxSlot::filterLp && std::abs(lpHz - lastLp) > 0.5f)
            {
                lastLp = lpHz;
                const auto coef = juce::IIRCoefficients::makeLowPass(
                    activeSampleRate, lpHz);
                lp[0].setCoefficients(coef);
                lp[1].setCoefficients(coef);
            }
            for (int ch = 0; ch < channels; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                if (slot == FxSlot::filterHp)
                    hp[ch].processSamples(data, samples);
                else
                    lp[ch].processSamples(data, samples);
            }
            updateLeds(slot, buffer, samples);
            break;
        }
        case FxSlot::distortion:
        {
            const auto drive = 1.0f + getAmount(FxSlot::distortion) * 9.0f;
            const auto norm = std::tanh(drive);
            for (int ch = 0; ch < channels; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                for (int i = 0; i < samples; ++i)
                {
                    const auto x = data[i];
                    const auto odd = x + 0.12f * x * std::abs(x);
                    data[i] = std::tanh(odd * drive) / norm;
                }
            }
            updateLeds(FxSlot::distortion, buffer, samples);
            break;
        }
        case FxSlot::doubler:
        {
            const auto mix = getAmount(FxSlot::doubler);
            const auto baseDelay = static_cast<int>(activeSampleRate * 0.0024);
            const auto lineSize = static_cast<int>(doubleDelay[0].size());
            for (int i = 0; i < samples; ++i)
            {
                const auto modulation = std::sin(doublePhase) * 2.4;
                doublePhase += juce::MathConstants<double>::twoPi
                    * 0.7 / activeSampleRate;
                if (doublePhase >= juce::MathConstants<double>::twoPi)
                    doublePhase -= juce::MathConstants<double>::twoPi;
                for (int ch = 0; ch < channels; ++ch)
                {
                    auto* data = buffer.getWritePointer(ch);
                    const auto dry = data[i];
                    auto& line = doubleDelay[static_cast<size_t>(ch)];
                    line[static_cast<size_t>(doubleWrite)] = dry;
                    auto read = doubleWrite - baseDelay
                        - static_cast<int>(std::round(
                            modulation * (ch == 0 ? 1.0 : -1.0)));
                    while (read < 0)
                        read += lineSize;
                    const auto doubled = line[static_cast<size_t>(read)];
                    data[i] = dry * (1.0f - mix)
                        + std::tanh(dry + doubled * 0.72f) * mix;
                }
                doubleWrite = (doubleWrite + 1) % lineSize;
            }
            updateLeds(FxSlot::doubler, buffer, samples);
            break;
        }
        case FxSlot::delay:
            applyDelay(buffer);
            break;
        case FxSlot::exciter:
        {
            const auto amount = getAmount(FxSlot::exciter) * 0.35f;
            for (int ch = 0; ch < channels; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                float prev = 0.0f;
                for (int i = 0; i < samples; ++i)
                {
                    const auto high = data[i] - prev;
                    prev = data[i];
                    data[i] += amount * std::tanh(high * 3.2f);
                }
            }
            updateLeds(FxSlot::exciter, buffer, samples);
            break;
        }
        case FxSlot::compressor:
        {
            const auto amount = getAmount(FxSlot::compressor);
            const auto thresh = 0.28f - amount * 0.18f;
            const auto ratio = 1.4f + amount * 4.0f;
            const auto atk = std::exp(-1.0 / (activeSampleRate * 0.008));
            const auto rel = std::exp(-1.0 / (activeSampleRate * 0.12));
            for (int ch = 0; ch < channels; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                auto& env = compEnv[ch];
                for (int i = 0; i < samples; ++i)
                {
                    const auto absx = std::abs(data[i]);
                    env = absx > env
                        ? static_cast<float>(atk) * env
                            + static_cast<float>(1.0 - atk) * absx
                        : static_cast<float>(rel) * env
                            + static_cast<float>(1.0 - rel) * absx;
                    auto gain = 1.0f;
                    if (env > thresh)
                        gain = (thresh + (env - thresh) / ratio) / (env + 1.0e-5f);
                    data[i] *= gain * (1.0f + amount * 0.18f);
                }
            }
            updateLeds(FxSlot::compressor, buffer, samples);
            break;
        }
        case FxSlot::efecto:
            applyReverb(buffer);
            break;
        case FxSlot::volume:
            buffer.applyGain(volumeGain());
            updateLeds(FxSlot::volume, buffer, samples);
            break;
        case FxSlot::velocity:
            buffer.applyGain(velocityGain());
            updateLeds(FxSlot::velocity, buffer, samples);
            break;
        default:
            break;
    }

    for (int ch = 0; ch < channels; ++ch)
        outputLeds[static_cast<size_t>(ch)].store(juce::jmax(
            outputLeds[static_cast<size_t>(ch)].load() * 0.7f,
            juce::jmin(1.0f, slotPeak(buffer, samples, ch) * 3.0f)));
}

juce::ValueTree FxRack::toTree() const
{
    juce::ValueTree tree("FxRack");
    tree.setProperty("masterMute", isMasterMute(), nullptr);
    for (int i = 0; i < slotCount; ++i)
    {
        tree.setProperty(slotName(i),
                         amounts[static_cast<size_t>(i)].load(), nullptr);
        tree.setProperty(juce::String(slotName(i)) + "Mute",
                         mutes[static_cast<size_t>(i)].load(), nullptr);
    }
    return tree;
}

void FxRack::fromTree(const juce::ValueTree& tree)
{
    if (!tree.hasType("FxRack"))
        return;
    setMasterMute(static_cast<bool>(tree.getProperty("masterMute", false)));
    for (int i = 0; i < slotCount; ++i)
    {
        amounts[static_cast<size_t>(i)].store(static_cast<float>(
            tree.getProperty(slotName(i),
                             amounts[static_cast<size_t>(i)].load())));
        mutes[static_cast<size_t>(i)].store(static_cast<bool>(
            tree.getProperty(juce::String(slotName(i)) + "Mute", false)));
    }
}
