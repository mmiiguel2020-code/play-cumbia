#include "PluginProcessor.h"
#include "PluginEditor.h"

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
}

MiguelMusicAssistantAudioProcessor::MiguelMusicAssistantAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats();
    for (int voice = 0; voice < 16; ++voice)
        previewSynth.addVoice(new PreviewVoice());
    previewSynth.addSound(new PreviewSound());
}

void MiguelMusicAssistantAudioProcessor::prepareToPlay(double sampleRate,
                                                        int samplesPerBlock)
{
    activeSampleRate = sampleRate;
    previewSynth.setCurrentPlaybackSampleRate(sampleRate);
    grooveEngine.prepare(sampleRate);
    pianoEngine.prepare(sampleRate);
    sectionEqBank.prepare(sampleRate);
    sampleTransport.prepareToPlay(samplesPerBlock, sampleRate);
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
    return (output == juce::AudioChannelSet::mono()
            || output == juce::AudioChannelSet::stereo())
        && output == layouts.getMainInputChannelSet();
}

void MiguelMusicAssistantAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const auto channels = buffer.getNumChannels();
    const auto samples = buffer.getNumSamples();
    if (channels == 0 || samples == 0)
        return;

    if (samplePreviewBuffer.getNumChannels() < channels
        || samplePreviewBuffer.getNumSamples() < samples)
        samplePreviewBuffer.setSize(channels, samples, false, false, true);
    samplePreviewBuffer.clear();
    juce::AudioSourceChannelInfo previewInfo(&samplePreviewBuffer, 0, samples);
    sampleTransport.getNextAudioBlock(previewInfo);
    sectionEqBank.process(previewSection.load(), samplePreviewBuffer);
    for (int channel = 0; channel < channels; ++channel)
        buffer.addFrom(channel, 0, samplePreviewBuffer, channel, 0, samples);

    addPreviewMidi(midi, samples);
    if (sectionBuffer.getNumChannels() < channels
        || sectionBuffer.getNumSamples() < samples)
        sectionBuffer.setSize(channels, samples, false, false, true);

    sectionBuffer.clear();
    previewSynth.renderNextBlock(sectionBuffer, midi, 0, samples);
    sectionEqBank.process(AudioSection::generator, sectionBuffer);
    for (int channel = 0; channel < channels; ++channel)
        buffer.addFrom(channel, 0, sectionBuffer, channel, 0, samples);

    sectionBuffer.clear();
    grooveEngine.process(sectionBuffer);
    sectionEqBank.process(AudioSection::rhythms, sectionBuffer);
    for (int channel = 0; channel < channels; ++channel)
        buffer.addFrom(channel, 0, sectionBuffer, channel, 0, samples);

    sectionBuffer.clear();
    pianoEngine.process(sectionBuffer);
    sectionEqBank.process(AudioSection::piano, sectionBuffer);
    for (int channel = 0; channel < channels; ++channel)
        buffer.addFrom(channel, 0, sectionBuffer, channel, 0, samples);

    double sumSquares = 0.0;
    double leftRight = 0.0;
    double leftSquares = 0.0;
    double rightSquares = 0.0;
    float peak = 0.0f;
    float leftPeak = 0.0f;
    float rightPeak = 0.0f;

    for (int sample = 0; sample < samples; ++sample)
    {
        const auto left = buffer.getSample(0, sample);
        const auto right = channels > 1 ? buffer.getSample(1, sample) : left;
        sumSquares += 0.5 * (left * left + right * right);
        leftRight += left * right;
        leftSquares += left * left;
        rightSquares += right * right;
        leftPeak = juce::jmax(leftPeak, std::abs(left));
        rightPeak = juce::jmax(rightPeak, std::abs(right));
        peak = juce::jmax(peak, std::abs(left), std::abs(right));
    }

    const auto rms = static_cast<float>(std::sqrt(sumSquares / samples));
    const auto denominator = std::sqrt(leftSquares * rightSquares);
    const auto correlation = denominator > 0.0
        ? static_cast<float>(leftRight / denominator) : 1.0f;

    rmsDb.store(juce::Decibels::gainToDecibels(rms, -100.0f));
    peakDb.store(juce::Decibels::gainToDecibels(peak, -100.0f));
    leftRmsDb.store(juce::Decibels::gainToDecibels(
        static_cast<float>(std::sqrt(leftSquares / samples)), -100.0f));
    rightRmsDb.store(juce::Decibels::gainToDecibels(
        static_cast<float>(std::sqrt(rightSquares / samples)), -100.0f));
    leftPeakDb.store(juce::Decibels::gainToDecibels(leftPeak, -100.0f));
    rightPeakDb.store(juce::Decibels::gainToDecibels(rightPeak, -100.0f));
    stereoCorrelation.store(juce::jlimit(-1.0f, 1.0f, correlation));
    if (peak >= 1.0f)
        clipped.store(true);

    int start1 = 0;
    int size1 = 0;
    int start2 = 0;
    int size2 = 0;
    analyzerFifo.prepareToWrite(
        samples, start1, size1, start2, size2);
    const auto copyMono = [&](int destinationStart, int count,
                              int sourceStart)
    {
        for (int index = 0; index < count; ++index)
        {
            const auto sourceIndex = sourceStart + index;
            const auto left = buffer.getSample(0, sourceIndex);
            const auto right = channels > 1
                ? buffer.getSample(1, sourceIndex) : left;
            analyzerBuffer[static_cast<size_t>(destinationStart + index)] =
                (left + right) * 0.5f;
        }
    };
    copyMono(start1, size1, 0);
    copyMono(start2, size2, size1);
    analyzerFifo.finishedWrite(size1 + size2);
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

bool MiguelMusicAssistantAudioProcessor::loadSample(
    const juce::File& file, AudioSection section)
{
    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr)
        return false;

    sampleTransport.stop();
    sampleTransport.setSource(nullptr);
    sampleReader.reset();
    sampleReader = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
    sampleTransport.setSource(sampleReader.get(), 0, nullptr,
                              reader->sampleRate);
    previewSection.store(section);
    return true;
}

void MiguelMusicAssistantAudioProcessor::playSample()
{
    if (sampleReader == nullptr)
        return;
    sampleTransport.setPosition(0.0);
    sampleTransport.start();
}

void MiguelMusicAssistantAudioProcessor::stopPreviews()
{
    sampleTransport.stop();
    sampleTransport.setPosition(0.0);
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
    return SessionState::buildFromEngines(
        grooveEngine, sectionEqBank, uiCopy);
}

void MiguelMusicAssistantAudioProcessor::restoreFullSessionState(
    const juce::ValueTree& session)
{
    SessionState::applyToEngines(session, grooveEngine, sectionEqBank);
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
