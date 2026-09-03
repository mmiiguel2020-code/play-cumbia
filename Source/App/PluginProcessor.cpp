#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <juce_audio_formats/juce_audio_formats.h>

namespace
{
class PreviewSound final : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class PreviewVoice final : public juce::SynthesiserVoice
{
public:
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<PreviewSound*>(sound) != nullptr;
    }

    void startNote(int note, float velocity, juce::SynthesiserSound*,
                   int) override
    {
        currentAngle = 0.0;
        level = velocity * 0.12;
        tailOff = 0.0;
        const auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(note);
        angleDelta = cyclesPerSecond * juce::MathConstants<double>::twoPi
            / getSampleRate();
    }

    void stopNote(float, bool allowTailOff) override
    {
        if (allowTailOff && tailOff == 0.0)
            tailOff = 1.0;
        else
            clearCurrentNote();
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& output, int startSample,
                         int numSamples) override
    {
        if (angleDelta == 0.0)
            return;

        while (--numSamples >= 0)
        {
            const auto sample = static_cast<float>(std::sin(currentAngle) * level
                * (tailOff > 0.0 ? tailOff : 1.0));
            for (int channel = 0; channel < output.getNumChannels(); ++channel)
                output.addSample(channel, startSample, sample);

            currentAngle += angleDelta;
            ++startSample;
            if (tailOff > 0.0)
            {
                tailOff *= 0.992;
                if (tailOff <= 0.005)
                {
                    clearCurrentNote();
                    angleDelta = 0.0;
                    break;
                }
            }
        }
    }

private:
    double currentAngle = 0.0;
    double angleDelta = 0.0;
    double level = 0.0;
    double tailOff = 0.0;
};

bool writeConstantWav(const juce::File& file, float value)
{
    juce::AudioBuffer<float> buffer(2, 512);
    for (int ch = 0; ch < 2; ++ch)
        juce::FloatVectorOperations::fill(buffer.getWritePointer(ch), value, 512);

    std::unique_ptr<juce::OutputStream> stream(file.createOutputStream().release());
    if (stream == nullptr)
        return false;
    juce::WavAudioFormat wav;
    const auto options = juce::AudioFormatWriterOptions{}
        .withSampleRate(44100.0)
        .withNumChannels(2)
        .withBitsPerSample(16);
    auto writer = wav.createWriterFor(stream, options);
    return writer != nullptr
        && writer->writeFromAudioSampleBuffer(buffer, 0, 512);
}

juce::String runLayerMixSmoke(juce::AudioFormatManager& formats)
{
    auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("picha-mix-smoke");
    if (!dir.createDirectory())
        return "FAIL no pude crear temp";
    const auto wavA = dir.getChildFile("a.wav");
    const auto wavB = dir.getChildFile("b.wav");
    if (!writeConstantWav(wavA, 0.40f) || !writeConstantWav(wavB, 0.40f))
        return "FAIL no pude escribir wav";

    SampleLayerBank bank;
    bank.prepare(44100.0);
    bank.setLooping(false);
    if (!bank.loadSlot(0, wavA, formats) || !bank.loadSlot(1, wavB, formats))
        return "FAIL no cargaron los wav";

    juce::Array<int> onlyFirst;
    onlyFirst.add(0);
    bank.armOnly(onlyFirst);
    bank.start();
    juce::AudioBuffer<float> one(2, 256);
    one.clear();
    bank.process(one);
    const auto peakOne = one.getMagnitude(0, 256);

    juce::Array<int> bothSlots;
    bothSlots.add(0);
    bothSlots.add(1);
    bank.armOnly(bothSlots);
    bank.start();
    juce::AudioBuffer<float> both(2, 256);
    both.clear();
    bank.process(both);
    const auto peakBoth = both.getMagnitude(0, 256);

    if (peakOne < 0.30f || peakOne > 0.50f)
        return "FAIL solo-slot peak=" + juce::String(peakOne, 3);
    if (peakBoth < 0.65f || peakBoth > 0.90f)
        return "FAIL dos-slots peak=" + juce::String(peakBoth, 3)
            + " (uno=" + juce::String(peakOne, 3) + ")";
    return "PASS uno=" + juce::String(peakOne, 3)
        + " dos=" + juce::String(peakBoth, 3);
}

juce::String runPianoTuneCheck()
{
    const auto a4 = juce::MidiMessage::getMidiNoteInHertz(69);
    const auto c4 = juce::MidiMessage::getMidiNoteInHertz(60);
    const auto c5 = juce::MidiMessage::getMidiNoteInHertz(72);
    const auto c6 = juce::MidiMessage::getMidiNoteInHertz(84);
    const auto c7 = juce::MidiMessage::getMidiNoteInHertz(96);
    if (std::abs(a4 - 440.0) > 1.0e-6)
        return "FAIL piano A4=" + juce::String(a4, 8);
    if (std::abs(c5 / c4 - 2.0) > 1.0e-9 || std::abs(c6 / c4 - 4.0) > 1.0e-9
        || std::abs(c7 / c4 - 8.0) > 1.0e-9)
        return "FAIL piano octavas C4-C7";
    if (std::abs(c4 - 261.625565) > 0.01)
        return "FAIL piano C4=" + juce::String(c4, 6);
    return "PASS piano C4=" + juce::String(c4, 2) + "Hz MIDI60 A4=440 C5="
        + juce::String(c5, 1) + " C6=" + juce::String(c6, 1)
        + " (FL suele etiquetar MIDI60 como C5)";
}
}

MiguelMusicAssistantAudioProcessor::MiguelMusicAssistantAudioProcessor()
    : AudioProcessor(BusesProperties()
#if JucePlugin_IsSynth
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#else
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
        )
{
    formatManager.registerBasicFormats();
    shapedSource.setRack(&fxRack);
    for (int voice = 0; voice < 16; ++voice)
        previewSynth.addVoice(new PreviewVoice());
    previewSynth.addSound(new PreviewSound());
    for (int voice = 0; voice < 8; ++voice)
        refPianoSynth.addVoice(new PreviewVoice());
    refPianoSynth.addSound(new PreviewSound());

    const auto smoke = runLayerMixSmoke(formatManager);
    const auto pianoTune = runPianoTuneCheck();
    auto report = juce::File::getSpecialLocation(
        juce::File::userDocumentsDirectory)
        .getChildFile("Miguel Music Assistant")
        .getChildFile("mix-smoke.txt");
    report.getParentDirectory().createDirectory();
    report.replaceWithText(smoke + "\n" + pianoTune + "\n");
}

MiguelMusicAssistantAudioProcessor::~MiguelMusicAssistantAudioProcessor()
{
}

void MiguelMusicAssistantAudioProcessor::prepareToPlay(double sampleRate,
                                                        int samplesPerBlock)
{
    activeSampleRate = sampleRate;
    previewSynth.setCurrentPlaybackSampleRate(sampleRate);
    refPianoSynth.setCurrentPlaybackSampleRate(sampleRate);
    grooveEngine.prepare(sampleRate);
    layerBank.prepare(sampleRate);
    pianoEngine.prepare(sampleRate);
    sectionEqBank.prepare(sampleRate);
    fxRack.prepare(sampleRate, samplesPerBlock);
    sampleTransport.prepareToPlay(samplesPerBlock, sampleRate);
    captureBuffer.setSize(
        juce::jmax(1, getTotalNumInputChannels()),
        juce::jmax(1, static_cast<int>(sampleRate * 30.0)),
        false, true, true);
    captureWritten.store(0);
    {
        const juce::ScopedLock scoped(sampleEqLock);
        updateSampleEqLocked();
        for (auto& channelFilters : sampleEqFilters)
            for (auto& filter : channelFilters)
                filter.reset();
    }
    samplePreviewBuffer.setSize(
        juce::jmax(1, getTotalNumOutputChannels()), samplesPerBlock);
    sectionBuffer.setSize(
        juce::jmax(1, getTotalNumOutputChannels()), samplesPerBlock);
    rmsDb.store(-100.0f);
    peakDb.store(-100.0f);
    leftRmsDb.store(-100.0f);
    rightRmsDb.store(-100.0f);
    leftPeakDb.store(-100.0f);
    rightPeakDb.store(-100.0f);
    stereoCorrelation.store(1.0f);
    clipped.store(false);
    analyzerFifo.reset();
}

void MiguelMusicAssistantAudioProcessor::releaseResources()
{
    sampleTransport.releaseResources();
    samplePreviewBuffer.setSize(0, 0);
    sectionBuffer.setSize(0, 0);
}

bool MiguelMusicAssistantAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    if (output != juce::AudioChannelSet::mono()
        && output != juce::AudioChannelSet::stereo())
        return false;
#if JucePlugin_IsSynth
    const auto input = layouts.getMainInputChannelSet();
    return input.isDisabled() || input == output;
#else
    return output == layouts.getMainInputChannelSet();
#endif
}

void MiguelMusicAssistantAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const auto channels = buffer.getNumChannels();
    const auto samples = buffer.getNumSamples();
    if (channels == 0 || samples == 0)
        return;

    buffer.clear();

    if (layerBank.isPlaying())
    {
        layerBank.process(buffer);
        applySampleEq(buffer);
    }

    sectionEqBank.process(previewSection.load(), buffer);

    if (midiPreviewPlaying.load())
    {
        addPreviewMidi(midi, samples);
        if (sectionBuffer.getNumChannels() < channels
            || sectionBuffer.getNumSamples() < samples)
            sectionBuffer.setSize(channels, samples, false, false, true);
        sectionBuffer.clear();
        previewSynth.renderNextBlock(sectionBuffer, midi, 0, samples);
        for (int channel = 0; channel < channels; ++channel)
            buffer.addFrom(channel, 0, sectionBuffer, channel, 0, samples);
    }

    auto mixPeak = 0.0f;
    for (int channel = 0; channel < channels; ++channel)
    {
        auto* data = buffer.getReadPointer(channel);
        for (int i = 0; i < samples; ++i)
            mixPeak = juce::jmax(mixPeak, std::abs(data[i]));
    }

    auto state = captureState.load();
    if (state == 2)
    {
        if (mixPeak > 0.018f)
            captureSilentSamples.store(0);
        else
            captureSilentSamples.fetch_add(samples);

        if (captureBuffer.getNumSamples() < static_cast<int>(activeSampleRate * 30.0)
            || captureBuffer.getNumChannels() < juce::jmax(1, channels))
            captureBuffer.setSize(
                juce::jmax(1, channels),
                juce::jmax(samples, static_cast<int>(activeSampleRate * 30.0)),
                false, true, true);
        const juce::ScopedLock sl(captureLock);
        const auto room = captureBuffer.getNumSamples() - captureWritten.load();
        const auto toCopy = juce::jmin(samples, juce::jmax(0, room));
        const auto destCh = juce::jmin(channels, captureBuffer.getNumChannels());
        const auto start = captureWritten.load();
        for (int channel = 0; channel < destCh; ++channel)
            captureBuffer.copyFrom(channel, start, buffer, channel, 0, toCopy);
        captureWritten.store(start + toCopy);
        const auto silenceLimit = static_cast<int>(activeSampleRate * 10.0);
        if (captureStopRequest.load() || toCopy < samples
            || start + toCopy >= captureBuffer.getNumSamples()
            || captureSilentSamples.load() >= silenceLimit)
            captureState.store(3);
    }

    fxRack.process(buffer);

    juce::MidiBuffer pianoMidi;
    refPianoState.processNextMidiBuffer(pianoMidi, 0, samples, true);
    if (!midiPreviewPlaying.load())
    {
        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();
            if (!message.isNoteOnOrOff())
                continue;
            const auto note = message.getNoteNumber();
            if (note >= 60 && note <= 96)
                pianoMidi.addEvent(message, metadata.samplePosition);
        }
    }
    if (sectionBuffer.getNumChannels() < channels
        || sectionBuffer.getNumSamples() < samples)
        sectionBuffer.setSize(channels, samples, false, false, true);
    sectionBuffer.clear();
    refPianoSynth.renderNextBlock(sectionBuffer, pianoMidi, 0, samples);
    for (int channel = 0; channel < channels; ++channel)
        buffer.addFrom(channel, 0, sectionBuffer, channel, 0, samples);
}

int MiguelMusicAssistantAudioProcessor::popAnalyzerSamples(
    float* destination, int maximumSamples)
{
    if (destination == nullptr || maximumSamples <= 0)
        return 0;
    int start1 = 0;
    int size1 = 0;
    int start2 = 0;
    int size2 = 0;
    analyzerFifo.prepareToRead(
        maximumSamples, start1, size1, start2, size2);
    std::copy_n(analyzerBuffer.data() + start1, size1, destination);
    std::copy_n(analyzerBuffer.data() + start2, size2,
                destination + size1);
    analyzerFifo.finishedRead(size1 + size2);
    return size1 + size2;
}

void ShapedSampleSource::setBuffer(juce::AudioBuffer<float> newBuffer,
                                   double nativeRate)
{
    buffer = std::move(newBuffer);
    sourceRate = nativeRate > 0.0 ? nativeRate : 44100.0;
    position = 0.0;
    reachedEnd.store(false);
}

void ShapedSampleSource::setNextReadPosition(juce::int64 newPosition)
{
    position = static_cast<double>(juce::jmax<juce::int64>(0, newPosition));
}

juce::int64 ShapedSampleSource::getNextReadPosition() const
{
    return static_cast<juce::int64>(position);
}

juce::int64 ShapedSampleSource::getTotalLength() const
{
    return buffer.getNumSamples();
}

void ShapedSampleSource::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();
    const auto total = buffer.getNumSamples();
    if (total <= 1 || info.buffer == nullptr || info.numSamples <= 0)
        return;

    const auto gain = (rack != nullptr && !rack->isMuted(FxSlot::velocity))
        ? rack->velocityGain()
        : 1.0f;
    const auto increment = juce::jlimit(0.25, 4.0, pitchRatio.load());
    const auto srcCh = buffer.getNumChannels();
    const auto dstCh = info.buffer->getNumChannels();
    const auto span = static_cast<double>(juce::jmax(1, total - 1));
    const auto startPos = span * static_cast<double>(trimStart.load());
    auto endPos = span * static_cast<double>(trimEnd.load());
    if (endPos < startPos + span * 0.05)
        endPos = juce::jmin(span, startPos + span * 0.05);
    const auto fadeInLen = (endPos - startPos) * static_cast<double>(fadeIn.load()) * 0.5;
    const auto fadeOutLen = (endPos - startPos) * static_cast<double>(fadeOut.load()) * 0.5;
    auto pos = position;
    if (pos < startPos)
        pos = startPos;
    auto stillPlaying = false;

    for (int i = 0; i < info.numSamples; ++i)
    {
        if (pos >= endPos)
        {
            if (!looping)
                break;
            pos = startPos;
        }
        stillPlaying = true;
        const auto i0 = juce::jlimit(0, total - 2, static_cast<int>(pos));
        const auto frac = static_cast<float>(pos - static_cast<double>(i0));
        auto env = 1.0f;
        if (fadeInLen > 1.0 && pos < startPos + fadeInLen)
            env *= static_cast<float>((pos - startPos) / fadeInLen);
        if (fadeOutLen > 1.0 && pos > endPos - fadeOutLen)
            env *= static_cast<float>((endPos - pos) / fadeOutLen);
        for (int ch = 0; ch < dstCh; ++ch)
        {
            const auto src = juce::jmin(ch, srcCh - 1);
            const auto a = buffer.getSample(src, i0);
            const auto b = buffer.getSample(src, i0 + 1);
            info.buffer->setSample(ch, info.startSample + i,
                                   (a + (b - a) * frac) * gain * env);
        }
        pos += increment;
    }
    position = pos;
    if (!looping && !stillPlaying)
        reachedEnd.store(true);
}

void SampleLayerBank::prepare(double sampleRate)
{
    hostSampleRate = juce::jmax(8000.0, sampleRate);
}

bool SampleLayerBank::loadSlot(int slot, const juce::File& file,
                               juce::AudioFormatManager& formats)
{
    if (!juce::isPositiveAndBelow(slot, slotCount) || !file.existsAsFile())
        return false;

    auto* reader = formats.createReaderFor(file);
    if (reader == nullptr)
        return false;

    const auto length = static_cast<int>(reader->lengthInSamples);
    juce::AudioBuffer<float> loaded(
        juce::jmax(1, static_cast<int>(reader->numChannels)),
        juce::jmax(1, length));
    reader->read(&loaded, 0, loaded.getNumSamples(), 0, true, true);
    const auto nativeRate = reader->sampleRate;
    delete reader;

    const juce::ScopedLock sl(lock);
    auto& target = slots[static_cast<size_t>(slot)];
    target.audio = std::move(loaded);
    target.sourceRate = nativeRate > 0.0 ? nativeRate : 44100.0;
    target.path = file.getFullPathName();
    target.name = file.getFileName();
    target.position = 0.0;
    target.gapLeft = 0;
    target.lpState[0] = 0.0f;
    target.lpState[1] = 0.0f;
    target.loaded.store(true);
    return true;
}

void SampleLayerBank::clearSlot(int slot)
{
    if (!juce::isPositiveAndBelow(slot, slotCount))
        return;
    const juce::ScopedLock sl(lock);
    auto& target = slots[static_cast<size_t>(slot)];
    target.audio.setSize(0, 0);
    target.path.clear();
    target.name.clear();
    target.position = 0.0;
    target.gapLeft = 0;
    target.loaded.store(false);
    target.armed.store(false);
}

void SampleLayerBank::armOnly(const juce::Array<int>& slotsToPlay)
{
    for (int i = 0; i < slotCount; ++i)
        slots[static_cast<size_t>(i)].armed.store(false);
    for (const auto slot : slotsToPlay)
        if (juce::isPositiveAndBelow(slot, slotCount))
            slots[static_cast<size_t>(slot)].armed.store(true);
}

void SampleLayerBank::start()
{
    const juce::ScopedLock sl(lock);
    const auto startN = juce::jlimit(
        0.0, 0.90, static_cast<double>(trimStart.load()));
    auto endN = juce::jlimit(0.10, 1.0, static_cast<double>(trimEnd.load()));
    if (endN < startN + 0.05)
        endN = juce::jmin(1.0, startN + 0.05);
    const auto reverse = reversed.load();
    auto any = false;
    for (auto& slot : slots)
    {
        const auto length = slot.audio.getNumSamples();
        const auto lengthN = static_cast<double>(juce::jmax(1, length - 1));
        slot.position = (reverse ? endN : startN) * lengthN;
        slot.gapLeft = 0;
        if (slot.armed.load() && slot.loaded.load()
            && slot.audio.getNumSamples() > 1)
            any = true;
    }
    playing.store(any);
}

void SampleLayerBank::stop()
{
    playing.store(false);
    const juce::ScopedLock sl(lock);
    for (auto& slot : slots)
    {
        slot.position = 0.0;
        slot.led.store(0.0f);
    }
}

void SampleLayerBank::process(juce::AudioBuffer<float>& output)
{
    if (!playing.load())
        return;

    const juce::ScopedLock sl(lock);
    const auto host = hostSampleRate > 0.0 ? hostSampleRate : 44100.0;
    const auto numSamples = output.getNumSamples();
    const auto outCh = output.getNumChannels();
    const auto loop = looping.load();
    const auto reverse = reversed.load();
    const auto startN = juce::jlimit(
        0.0, 0.90, static_cast<double>(trimStart.load()));
    auto endN = juce::jlimit(0.10, 1.0, static_cast<double>(trimEnd.load()));
    if (endN < startN + 0.05)
        endN = juce::jmin(1.0, startN + 0.05);
    const auto fadeInAmt = static_cast<double>(fadeIn.load()) * 0.35;
    const auto fadeOutAmt = static_cast<double>(fadeOut.load()) * 0.35;
    const auto gapSamples = juce::jmax(1, static_cast<int>(host * 1.0));
    const auto lpCoeff = 1.0f - std::exp(-2.0f * 3.14159265f * 650.0f
        / static_cast<float>(host));
    auto anyVoice = false;

    for (auto& slot : slots)
    {
        if (!slot.armed.load() || !slot.loaded.load()
            || slot.audio.getNumSamples() < 2)
            continue;

        const auto length = slot.audio.getNumSamples();
        const auto srcCh = slot.audio.getNumChannels();
        const auto ratio = std::pow(2.0, slot.pitchSemitones.load() / 12.0);
        const auto increment = juce::jlimit(
            0.05, 8.0, (slot.sourceRate / host) * ratio);
        const auto gain = slot.muted.load()
            ? 0.0f
            : juce::jlimit(0.0f, 1.5f, slot.volume.load());
        const auto tilt = juce::jlimit(0.0f, 1.0f, slot.eqTilt.load()) * 2.0f - 1.0f;
        const auto lengthN = static_cast<double>(juce::jmax(1, length - 1));
        const auto startPos = startN * lengthN;
        const auto endPos = endN * lengthN;
        const auto span = juce::jmax(1.0, endPos - startPos);
        const auto fadeInLen = span * fadeInAmt;
        const auto fadeOutLen = span * fadeOutAmt;
        auto pos = slot.position;
        if (reverse)
        {
            if (pos > endPos)
                pos = endPos;
        }
        else if (pos < startPos)
        {
            pos = startPos;
        }
        auto gapLeft = slot.gapLeft;
        auto alive = true;
        auto peak = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            if (gapLeft > 0)
            {
                --gapLeft;
                if (gapLeft == 0)
                    pos = reverse ? endPos : startPos;
                continue;
            }

            const auto finished = reverse ? (pos <= startPos) : (pos >= endPos);
            if (finished)
            {
                if (loop)
                {
                    pos = reverse ? endPos : startPos;
                    gapLeft = gapSamples;
                    --gapLeft;
                    continue;
                }
                alive = false;
                break;
            }

            const auto i0 = juce::jlimit(0, length - 2, static_cast<int>(pos));
            const auto frac = static_cast<float>(pos - static_cast<double>(i0));
            auto env = 1.0f;
            if (fadeInLen > 1.0 && pos < startPos + fadeInLen)
                env *= static_cast<float>((pos - startPos) / fadeInLen);
            if (fadeOutLen > 1.0 && pos > endPos - fadeOutLen)
                env *= static_cast<float>((endPos - pos) / fadeOutLen);

            for (int ch = 0; ch < outCh; ++ch)
            {
                const auto src = juce::jmin(ch, srcCh - 1);
                const auto a = slot.audio.getSample(src, i0);
                const auto b = slot.audio.getSample(src, i0 + 1);
                auto value = (a + (b - a) * frac) * gain * env;
                auto& lp = slot.lpState[static_cast<size_t>(juce::jmin(ch, 1))];
                lp += lpCoeff * (value - lp);
                value = value + tilt * (value - lp) * 1.25f;
                peak = juce::jmax(peak, std::abs(value));
                if (gain > 0.0f)
                    output.addSample(ch, i, value);
            }
            pos += reverse ? -increment : increment;
        }

        slot.position = pos;
        slot.gapLeft = gapLeft;
        slot.led.store(juce::jmax(slot.led.load() * 0.72f, juce::jmin(1.0f, peak * 1.8f)));
        if (alive || loop)
            anyVoice = true;
    }

    if (!anyVoice)
        playing.store(false);
}

void SampleLayerBank::setVolume(int slot, float volume)
{
    if (!juce::isPositiveAndBelow(slot, slotCount))
        return;
    slots[static_cast<size_t>(slot)].volume.store(
        juce::jlimit(0.0f, 1.5f, volume));
}

void SampleLayerBank::setMuted(int slot, bool shouldMute)
{
    if (!juce::isPositiveAndBelow(slot, slotCount))
        return;
    slots[static_cast<size_t>(slot)].muted.store(shouldMute);
}

void SampleLayerBank::setEqTilt(int slot, float amount01)
{
    if (!juce::isPositiveAndBelow(slot, slotCount))
        return;
    slots[static_cast<size_t>(slot)].eqTilt.store(
        juce::jlimit(0.0f, 1.0f, amount01));
}

void SampleLayerBank::setTrimStart(float amount01)
{
    trimStart.store(juce::jlimit(0.0f, 0.90f, amount01));
}

void SampleLayerBank::setTrimEnd(float amount01)
{
    trimEnd.store(juce::jlimit(0.10f, 1.0f, amount01));
}

void SampleLayerBank::setFadeIn(float amount01)
{
    fadeIn.store(juce::jlimit(0.0f, 1.0f, amount01));
}

void SampleLayerBank::setFadeOut(float amount01)
{
    fadeOut.store(juce::jlimit(0.0f, 1.0f, amount01));
}

bool SampleLayerBank::isMuted(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, slotCount))
        return false;
    return slots[static_cast<size_t>(slot)].muted.load();
}

bool SampleLayerBank::isArmed(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, slotCount))
        return false;
    return slots[static_cast<size_t>(slot)].armed.load();
}

float SampleLayerBank::getLed(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, slotCount))
        return 0.0f;
    const auto& target = slots[static_cast<size_t>(slot)];
    return target.muted.load() ? 1.0f : target.led.load();
}

void SampleLayerBank::setPitchSemitones(int slot, double semitones)
{
    if (!juce::isPositiveAndBelow(slot, slotCount))
        return;
    slots[static_cast<size_t>(slot)].pitchSemitones.store(
        juce::jlimit(-12.0, 12.0, semitones));
}

void SampleLayerBank::setLooping(bool shouldLoop)
{
    looping.store(shouldLoop);
}

void SampleLayerBank::setReversed(bool shouldReverse)
{
    reversed.store(shouldReverse);
}

bool SampleLayerBank::hasSample(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, slotCount))
        return false;
    return slots[static_cast<size_t>(slot)].loaded.load();
}

juce::String SampleLayerBank::getSlotName(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, slotCount))
        return {};
    const juce::ScopedLock sl(lock);
    const auto& name = slots[static_cast<size_t>(slot)].name;
    return name.isNotEmpty() ? name : "Cargar sample";
}

juce::String SampleLayerBank::getSlotPath(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, slotCount))
        return {};
    const juce::ScopedLock sl(lock);
    return slots[static_cast<size_t>(slot)].path;
}

float SampleLayerBank::getVolume(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, slotCount))
        return 0.0f;
    return slots[static_cast<size_t>(slot)].volume.load();
}

float SampleLayerBank::getEqTilt(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, slotCount))
        return 0.5f;
    return slots[static_cast<size_t>(slot)].eqTilt.load();
}

double SampleLayerBank::getPitchSemitones(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, slotCount))
        return 0.0;
    return slots[static_cast<size_t>(slot)].pitchSemitones.load();
}

bool MiguelMusicAssistantAudioProcessor::loadSample(
    const juce::File& file, AudioSection section)
{
    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr)
        return false;

    const auto length = static_cast<int>(reader->lengthInSamples);
    juce::AudioBuffer<float> loaded(
        juce::jmax(1, static_cast<int>(reader->numChannels)),
        juce::jmax(1, length));
    reader->read(&loaded, 0, loaded.getNumSamples(), 0, true, true);
    const auto nativeRate = reader->sampleRate;
    delete reader;

    sampleTransport.stop();
    sampleTransport.setSource(nullptr);
    sampleReader.reset();
    shapedSource.setBuffer(std::move(loaded), nativeRate);
    sampleTransport.setSource(&shapedSource, 0, nullptr, nativeRate);
    sampleTransport.setLooping(shapedSource.isLooping());
    previewSection.store(section);
    return true;
}

bool MiguelMusicAssistantAudioProcessor::loadLayerSample(
    int slot, const juce::File& file)
{
    return layerBank.loadSlot(slot, file, formatManager);
}

void MiguelMusicAssistantAudioProcessor::playSlots(const juce::Array<int>& slots)
{
    juce::Array<int> armed;
    for (int i = 0; i < slots.size(); ++i)
        if (juce::isPositiveAndBelow(slots[i], SampleLayerBank::slotCount)
            && layerBank.hasSample(slots[i]))
            armed.add(slots[i]);
    layerBank.armOnly(armed);
    layerBank.start();
}

void MiguelMusicAssistantAudioProcessor::playSample()
{
    layerBank.start();
}

void MiguelMusicAssistantAudioProcessor::stopSamplePlayback()
{
    sampleTransport.stop();
    sampleTransport.setPosition(0.0);
    layerBank.stop();
}

void MiguelMusicAssistantAudioProcessor::toggleSamplePlayback()
{
    if (layerBank.isPlaying())
        stopSamplePlayback();
    else
        playSample();
}

void MiguelMusicAssistantAudioProcessor::toggleCapture()
{
    const auto state = captureState.load();
    if (state == 2)
    {
        captureStopRequest.store(true);
        return;
    }

    captureWritten.store(0);
    captureStopRequest.store(false);
    captureSilentSamples.store(0);
    captureState.store(2);
}

bool MiguelMusicAssistantAudioProcessor::takeCompletedCapture(juce::File& fileOut)
{
    if (captureState.load() != 3)
        return false;

    juce::AudioBuffer<float> copy;
    int frames = 0;
    {
        const juce::ScopedLock sl(captureLock);
        frames = captureWritten.load();
        if (frames < 256)
        {
            captureState.store(0);
            return false;
        }
        copy.setSize(captureBuffer.getNumChannels(), frames);
        for (int ch = 0; ch < copy.getNumChannels(); ++ch)
            copy.copyFrom(ch, 0, captureBuffer, ch, 0, frames);
        captureState.store(0);
        captureWritten.store(0);
    }

    auto folder = juce::File::getSpecialLocation(
        juce::File::userDocumentsDirectory)
        .getChildFile("Miguel Music Assistant")
        .getChildFile("Grabaciones");
    folder.createDirectory();
    const auto destination = folder.getNonexistentChildFile(
        "Grabacion_" + juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S"),
        ".wav", false);
    std::unique_ptr<juce::OutputStream> stream =
        destination.createOutputStream();
    if (stream == nullptr)
        return false;

    juce::WavAudioFormat wav;
    const auto options = juce::AudioFormatWriterOptions{}
        .withSampleRate(activeSampleRate)
        .withNumChannels(copy.getNumChannels())
        .withBitsPerSample(24);
    auto writer = wav.createWriterFor(stream, options);
    if (writer == nullptr
        || !writer->writeFromAudioSampleBuffer(copy, 0, frames))
        return false;

    fileOut = destination;
    return true;
}

void MiguelMusicAssistantAudioProcessor::setSampleLooping(bool shouldLoop)
{
    shapedSource.setLooping(shouldLoop);
    sampleTransport.setLooping(shouldLoop);
    layerBank.setLooping(shouldLoop);
}

void MiguelMusicAssistantAudioProcessor::setSampleReversed(bool shouldReverse)
{
    layerBank.setReversed(shouldReverse);
}

void MiguelMusicAssistantAudioProcessor::setSampleTrimStart(float amount01)
{
    shapedSource.setTrimStart(amount01);
    layerBank.setTrimStart(amount01);
}

void MiguelMusicAssistantAudioProcessor::setSampleTrimEnd(float amount01)
{
    shapedSource.setTrimEnd(amount01);
    layerBank.setTrimEnd(amount01);
}

void MiguelMusicAssistantAudioProcessor::setSampleFadeIn(float amount01)
{
    shapedSource.setFadeIn(amount01);
    layerBank.setFadeIn(amount01);
}

void MiguelMusicAssistantAudioProcessor::setSampleFadeOut(float amount01)
{
    shapedSource.setFadeOut(amount01);
    layerBank.setFadeOut(amount01);
}

void MiguelMusicAssistantAudioProcessor::setSamplePitchSemitones(
    double semitones, int slot)
{
    shapedSource.setPitchRatio(std::pow(2.0, semitones / 12.0));
    if (juce::isPositiveAndBelow(slot, SampleLayerBank::slotCount))
        layerBank.setPitchSemitones(slot, semitones);
}

void MiguelMusicAssistantAudioProcessor::updateSampleEqLocked()
{
    const auto rate = juce::jmax(8000.0, activeSampleRate);
    for (int band = 0; band < sampleEqBandCount; ++band)
    {
        const auto freq = sampleEqFrequencies[static_cast<size_t>(band)];
        const auto gain = juce::Decibels::decibelsToGain(
            sampleEqGains[static_cast<size_t>(band)]);
        juce::IIRCoefficients coefficients;
        if (band == 0)
            coefficients = juce::IIRCoefficients::makeLowShelf(
                rate, freq, 0.707, gain);
        else if (band == sampleEqBandCount - 1)
            coefficients = juce::IIRCoefficients::makeHighShelf(
                rate, freq, 0.707, gain);
        else
            coefficients = juce::IIRCoefficients::makePeakFilter(
                rate, freq, 0.9, gain);
        for (auto& channelFilters : sampleEqFilters)
            channelFilters[static_cast<size_t>(band)].setCoefficients(
                coefficients);
    }
}

void MiguelMusicAssistantAudioProcessor::setSampleEqGain(int band,
                                                          float decibels)
{
    if (!juce::isPositiveAndBelow(band, sampleEqBandCount))
        return;
    const juce::ScopedLock scoped(sampleEqLock);
    sampleEqGains[static_cast<size_t>(band)] =
        juce::jlimit(-18.0f, 18.0f, decibels);
    updateSampleEqLocked();
}

float MiguelMusicAssistantAudioProcessor::getSampleEqGain(int band) const
{
    if (!juce::isPositiveAndBelow(band, sampleEqBandCount))
        return 0.0f;
    const juce::ScopedLock scoped(sampleEqLock);
    return sampleEqGains[static_cast<size_t>(band)];
}

void MiguelMusicAssistantAudioProcessor::applySampleEq(
    juce::AudioBuffer<float>& buffer)
{
    const juce::ScopedTryLock scoped(sampleEqLock);
    if (!scoped.isLocked())
        return;
    bool idle = true;
    for (const auto gain : sampleEqGains)
        if (std::abs(gain) >= 0.01f)
            idle = false;
    if (idle)
        return;
    const auto channels = juce::jmin(2, buffer.getNumChannels());
    const auto numSamples = buffer.getNumSamples();
    for (int channel = 0; channel < channels; ++channel)
    {
        auto* data = buffer.getWritePointer(channel);
        for (auto& filter : sampleEqFilters[static_cast<size_t>(channel)])
            filter.processSamples(data, numSamples);
    }
}

void MiguelMusicAssistantAudioProcessor::stopPreviews()
{
    sampleTransport.stop();
    sampleTransport.setPosition(0.0);
    layerBank.stop();
    const juce::ScopedLock lock(previewLock);
    midiPreviewPlaying.store(false);
    midiPreviewProgress.store(0.0);
    previewSynth.allNotesOff(0, false);
}

void MiguelMusicAssistantAudioProcessor::startMidiPreview(
    const juce::MidiFile& file, double bpm)
{
    const juce::ScopedLock lock(previewLock);
    previewSequence.clear();
    for (int track = 0; track < file.getNumTracks(); ++track)
        if (const auto* sequence = file.getTrack(track))
            previewSequence.addSequence(*sequence, 0.0);

    previewSequence.sort();
    previewTempo = juce::jmax(40.0, bpm);
    previewSamplePosition = 0;
    constexpr double ticksPerQuarter = 960.0;
    const auto samplesPerTick = activeSampleRate * 60.0
        / (previewTempo * ticksPerQuarter);
    const auto lastTimestamp = previewSequence.getEndTime();
    previewTotalSamples = juce::jmax<juce::int64>(
        1, static_cast<juce::int64>(
            std::ceil(lastTimestamp * samplesPerTick)));
    midiPreviewProgress.store(0.0);
    nextPreviewEvent = 0;
    previewSynth.allNotesOff(0, false);
    midiPreviewPlaying.store(previewSequence.getNumEvents() > 0);
}

void MiguelMusicAssistantAudioProcessor::addPreviewMidi(
    juce::MidiBuffer& midi, int numSamples)
{
    const juce::ScopedLock lock(previewLock);
    if (!midiPreviewPlaying.load())
        return;

    constexpr double ticksPerQuarter = 960.0;
    const auto samplesPerTick = activeSampleRate * 60.0
        / (previewTempo * ticksPerQuarter);
    const auto blockEnd = previewSamplePosition + numSamples;

    while (nextPreviewEvent < previewSequence.getNumEvents())
    {
        const auto* event = previewSequence.getEventPointer(nextPreviewEvent);
        const auto eventSample = static_cast<juce::int64>(
            std::round(event->message.getTimeStamp() * samplesPerTick));
        if (eventSample >= blockEnd)
            break;
        if (!event->message.isMetaEvent())
            midi.addEvent(event->message,
                          static_cast<int>(juce::jmax<juce::int64>(
                              0, eventSample - previewSamplePosition)));
        ++nextPreviewEvent;
    }

    previewSamplePosition = blockEnd;
    midiPreviewProgress.store(juce::jlimit(
        0.0, 1.0, static_cast<double>(previewSamplePosition)
            / static_cast<double>(previewTotalSamples)));
    if (nextPreviewEvent >= previewSequence.getNumEvents())
    {
        midiPreviewPlaying.store(false);
        midiPreviewProgress.store(1.0);
        midi.addEvent(juce::MidiMessage::allNotesOff(1), numSamples - 1);
        midi.addEvent(juce::MidiMessage::allNotesOff(2), numSamples - 1);
    }
}

juce::AudioProcessorEditor*
MiguelMusicAssistantAudioProcessor::createEditor()
{
    return new MiguelMusicAssistantAudioProcessorEditor(*this);
}

void MiguelMusicAssistantAudioProcessor::getStateInformation(
    juce::MemoryBlock& destination)
{
    if (auto* editor = dynamic_cast<MiguelMusicAssistantAudioProcessorEditor*>(
            getActiveEditor()))
        editor->syncSessionStateToProcessor();

    const auto session = buildFullSessionState();
    if (const auto xml = session.createXml())
    {
        juce::MemoryOutputStream stream(destination, false);
        xml->writeTo(stream);
    }
}

void MiguelMusicAssistantAudioProcessor::setStateInformation(
    const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    const juce::String xml(
        juce::String::createStringFromData(data, sizeInBytes));
    if (const auto parsed = juce::parseXML(xml))
        restoreFullSessionState(juce::ValueTree::fromXml(*parsed));
}

void MiguelMusicAssistantAudioProcessor::setUiSessionState(
    const juce::ValueTree& state)
{
    const juce::ScopedLock lock(sessionLock);
    uiSessionState = state.isValid() ? state.createCopy()
                                     : juce::ValueTree("UiSession");
}

juce::ValueTree MiguelMusicAssistantAudioProcessor::getUiSessionState() const
{
    const juce::ScopedLock lock(sessionLock);
    return uiSessionState.createCopy();
}

juce::ValueTree MiguelMusicAssistantAudioProcessor::buildFullSessionState() const
{
    juce::ValueTree uiCopy;
    {
        const juce::ScopedLock lock(sessionLock);
        uiCopy = uiSessionState.createCopy();
    }
    auto session = SessionState::buildFromEngines(
        grooveEngine, sectionEqBank, uiCopy, fxRack.toTree());
    return session;
}

void MiguelMusicAssistantAudioProcessor::restoreFullSessionState(
    const juce::ValueTree& session)
{
    SessionState::applyToEngines(session, grooveEngine, sectionEqBank);
    fxRack.fromTree(session.getChildWithName("FxRack"));
    const auto ui = session.getChildWithName("UiSession");
    if (ui.isValid())
        setUiSessionState(ui);
}

void MiguelMusicAssistantAudioProcessor::saveAutosaveSession()
{
    SessionState::writeAutosave(buildFullSessionState());
}

bool MiguelMusicAssistantAudioProcessor::loadAutosaveSession()
{
    const auto session = SessionState::readAutosave();
    if (!session.isValid())
        return false;
    restoreFullSessionState(session);
    return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MiguelMusicAssistantAudioProcessor();
}
