#include "GrooveEngine.h"

namespace
{
std::shared_ptr<LoadedAudioSample> loadAudioFile(const juce::File& file)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(
        formats.createReaderFor(file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return {};

    auto result = std::make_shared<LoadedAudioSample>();
    const auto channels = juce::jlimit(
        1, 2, static_cast<int>(reader->numChannels));
    const auto length = static_cast<int>(juce::jmin<juce::int64>(
        reader->lengthInSamples, std::numeric_limits<int>::max()));
    result->audio.setSize(channels, length);
    if (!reader->read(&result->audio, 0, length, 0, true, true))
        return {};
    result->sampleRate = reader->sampleRate;
    result->name = file.getFileName();
    return result;
}

float interpolatedSample(const LoadedAudioSample& sample, int channel,
                         double position)
{
    const auto length = sample.audio.getNumSamples();
    const auto index = static_cast<int>(position);
    if (index < 0 || index >= length)
        return 0.0f;
    const auto sourceChannel = juce::jmin(
        channel, sample.audio.getNumChannels() - 1);
    if (index + 1 >= length)
        return sample.audio.getSample(sourceChannel, index);
    const auto fraction = static_cast<float>(position - index);
    return sample.audio.getSample(sourceChannel, index) * (1.0f - fraction)
        + sample.audio.getSample(sourceChannel, index + 1) * fraction;
}

bool writeWav(const juce::File& destination,
              juce::AudioBuffer<float>& audio, double sampleRate)
{
    const auto peak = audio.getMagnitude(0, audio.getNumSamples());
    if (peak > 0.98f)
        audio.applyGain(0.98f / peak);

    destination.deleteFile();
    std::unique_ptr<juce::OutputStream> stream =
        destination.createOutputStream();
    if (stream == nullptr)
        return false;
    juce::WavAudioFormat wav;
    const auto options = juce::AudioFormatWriterOptions{}
        .withSampleRate(sampleRate)
        .withNumChannels(audio.getNumChannels())
        .withBitsPerSample(24);
    auto writer = wav.createWriterFor(stream, options);
    return writer != nullptr
        && writer->writeFromAudioSampleBuffer(
            audio, 0, audio.getNumSamples());
}
}

void GrooveEngine::prepare(double sampleRate)
{
    const juce::ScopedLock scoped(lock);
    hostSampleRate = sampleRate;
    samplesUntilNextStep = 0.0;
    currentStep.store(-1);
    renderScratch.setSize(2, 512);
    for (auto& voice : voices)
        voice.position = -1.0;
    for (int channel = 0; channel < channelCount; ++channel)
        updateTrackEqLocked(channel);
    lastSnapshot.samples = samples;
    lastSnapshot.pattern = pattern;
    lastSnapshot.gains = gains;
    lastSnapshot.valid = true;
}

void GrooveEngine::renderVoiceBlock(
    int source, int frameOffset, int numSamples, double rateRatio,
    juce::AudioBuffer<float>& output)
{
    auto& voice = voices[static_cast<size_t>(source)];
    const auto& sample = lastSnapshot.samples[static_cast<size_t>(source)];
    if (voice.position < 0.0 || sample == nullptr)
        return;

    const auto gain = lastSnapshot.gains[static_cast<size_t>(source)];
    const auto length = sample->audio.getNumSamples();
    const auto ch0 = 0;
    const auto ch1 = juce::jmin(1, sample->audio.getNumChannels() - 1);
    auto* scratchL = renderScratch.getWritePointer(0);
    auto* scratchR = renderScratch.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        if (voice.position < 0.0 || voice.position >= length)
        {
            scratchL[i] = 0.0f;
            scratchR[i] = 0.0f;
            continue;
        }

        const auto index = static_cast<int>(voice.position);
        const auto fraction = static_cast<float>(voice.position - index);
        float valueL = 0.0f;
        float valueR = 0.0f;

        if (index + 1 < length)
        {
            valueL = sample->audio.getSample(ch0, index) * (1.0f - fraction)
                + sample->audio.getSample(ch0, index + 1) * fraction;
            valueR = sample->audio.getSample(ch1, index) * (1.0f - fraction)
                + sample->audio.getSample(ch1, index + 1) * fraction;
        }
        else
        {
            valueL = sample->audio.getSample(ch0, index);
            valueR = sample->audio.getSample(ch1, index);
        }

        scratchL[i] = valueL * gain;
        scratchR[i] = valueR * gain;
        voice.position += rateRatio;
        if (voice.position >= length)
            voice.position = -1.0;
    }

    auto& filtersL = trackEqFilters[static_cast<size_t>(source)][0];
    auto& filtersR = trackEqFilters[static_cast<size_t>(source)][1];
    for (auto& filter : filtersL)
        filter.processSamples(scratchL, numSamples);
    for (auto& filter : filtersR)
        filter.processSamples(scratchR, numSamples);

    const auto outChannels = output.getNumChannels();
    if (outChannels > 0)
        output.addFrom(0, frameOffset, scratchL, 0, numSamples);
    if (outChannels > 1)
        output.addFrom(1, frameOffset, scratchR, 0, numSamples);
    else if (outChannels > 0)
        output.addFrom(0, frameOffset, scratchR, 0, numSamples);
}

void GrooveEngine::process(juce::AudioBuffer<float>& output)
{
    if (!playing.load())
        return;

    {
        const juce::ScopedTryLock tryLock(lock);
        if (tryLock.isLocked())
        {
            lastSnapshot.samples = samples;
            lastSnapshot.pattern = pattern;
            lastSnapshot.gains = gains;
            lastSnapshot.valid = true;
        }
        else if (!lastSnapshot.valid)
        {
            return;
        }
    }

    const auto numSamples = output.getNumSamples();
    if (numSamples <= 0 || !lastSnapshot.valid)
        return;

    if (renderScratch.getNumSamples() < numSamples)
        renderScratch.setSize(2, numSamples, false, false, true);

    const auto stepLength = hostSampleRate * 60.0
        / (juce::jmax(30.0, bpm.load()) * stepsPerBeat.load());
    const auto loopSteps = juce::jlimit(1, stepCount, loopLength.load());

    int frameOffset = 0;
    int samplesRemaining = numSamples;

    while (samplesRemaining > 0)
    {
        if (samplesUntilNextStep <= 0.0)
        {
            const auto step = (currentStep.load() + 1) % loopSteps;
            currentStep.store(step);
            for (int channel = 0; channel < channelCount; ++channel)
            {
                if (lastSnapshot.pattern[static_cast<size_t>(channel)]
                               [static_cast<size_t>(step)]
                    && lastSnapshot.samples[static_cast<size_t>(channel)]
                        != nullptr)
                {
                    voices[static_cast<size_t>(channel)].position = 0.0;
                    for (auto& outputChannel
                         : trackEqFilters[static_cast<size_t>(channel)])
                        for (auto& filter : outputChannel)
                            filter.reset();
                }
            }
            samplesUntilNextStep += stepLength;
        }

        const auto chunkSize = juce::jmin(
            samplesRemaining,
            juce::jmax(1, static_cast<int>(std::ceil(samplesUntilNextStep))));

        for (int source = 0; source < channelCount; ++source)
        {
            const auto& samplePtr =
                lastSnapshot.samples[static_cast<size_t>(source)];
            if (samplePtr == nullptr)
                continue;
            const auto rateRatio = samplePtr->sampleRate / hostSampleRate;
            renderVoiceBlock(source, frameOffset, chunkSize, rateRatio, output);
        }

        frameOffset += chunkSize;
        samplesRemaining -= chunkSize;
        samplesUntilNextStep -= static_cast<double>(chunkSize);
    }
}

bool GrooveEngine::loadSample(int channel, const juce::File& file)
{
    if (!juce::isPositiveAndBelow(channel, channelCount))
        return false;
    auto loaded = loadAudioFile(file);
    if (loaded == nullptr)
        return false;
    const juce::ScopedLock scoped(lock);
    samples[static_cast<size_t>(channel)] = std::move(loaded);
    samplePaths[static_cast<size_t>(channel)] = file.getFullPathName();
    return true;
}

void GrooveEngine::setStep(int channel, int step, bool enabled)
{
    if (!juce::isPositiveAndBelow(channel, channelCount)
        || !juce::isPositiveAndBelow(step, stepCount))
        return;
    const juce::ScopedLock scoped(lock);
    pattern[static_cast<size_t>(channel)][static_cast<size_t>(step)] = enabled;
}

bool GrooveEngine::getStep(int channel, int step) const
{
    if (!juce::isPositiveAndBelow(channel, channelCount)
        || !juce::isPositiveAndBelow(step, stepCount))
        return false;
    const juce::ScopedLock scoped(lock);
    return pattern[static_cast<size_t>(channel)][static_cast<size_t>(step)];
}

void GrooveEngine::clearPattern()
{
    const juce::ScopedLock scoped(lock);
    for (auto& row : pattern)
        row.fill(false);
}

void GrooveEngine::setGain(int channel, float gain)
{
    if (!juce::isPositiveAndBelow(channel, channelCount))
        return;
    const juce::ScopedLock scoped(lock);
    gains[static_cast<size_t>(channel)] = juce::jlimit(0.0f, 1.5f, gain);
}

float GrooveEngine::getGain(int channel) const
{
    if (!juce::isPositiveAndBelow(channel, channelCount))
        return 0.0f;
    const juce::ScopedLock scoped(lock);
    return gains[static_cast<size_t>(channel)];
}

juce::String GrooveEngine::getSampleName(int channel) const
{
    if (!juce::isPositiveAndBelow(channel, channelCount))
        return {};
    const juce::ScopedLock scoped(lock);
    const auto& sample = samples[static_cast<size_t>(channel)];
    return sample != nullptr ? sample->name : "Cargar sample";
}

void GrooveEngine::setBpm(double newBpm)
{
    bpm.store(juce::jlimit(30.0, 300.0, newBpm));
}

void GrooveEngine::setLoopLength(int steps)
{
    const auto valid = juce::jlimit(1, stepCount, steps);
    loopLength.store(valid);
    if (currentStep.load() >= valid)
        currentStep.store(-1);
}

void GrooveEngine::setGridResolution(int denominator)
{
    static constexpr std::array<int, 5> valid{ 4, 8, 16, 32, 64 };
    auto selected = 16;
    for (const auto value : valid)
        if (std::abs(value - denominator) < std::abs(selected - denominator))
            selected = value;
    gridResolution.store(selected);
    stepsPerBeat.store(selected / 4);
    setLoopLength(selected * 4);
}

void GrooveEngine::setTrackEqGain(int channel, int band, float decibels)
{
    if (!juce::isPositiveAndBelow(channel, channelCount)
        || !juce::isPositiveAndBelow(band, trackEqBandCount))
        return;
    const juce::ScopedLock scoped(lock);
    trackEqGains[static_cast<size_t>(channel)][static_cast<size_t>(band)] =
        juce::jlimit(-18.0f, 18.0f, decibels);
    updateTrackEqLocked(channel);
}

float GrooveEngine::getTrackEqGain(int channel, int band) const
{
    if (!juce::isPositiveAndBelow(channel, channelCount)
        || !juce::isPositiveAndBelow(band, trackEqBandCount))
        return 0.0f;
    const juce::ScopedLock scoped(lock);
    return trackEqGains[static_cast<size_t>(channel)]
                       [static_cast<size_t>(band)];
}

void GrooveEngine::updateTrackEqLocked(int channel)
{
    const auto& bandGains = trackEqGains[static_cast<size_t>(channel)];
    const std::array<juce::IIRCoefficients, trackEqBandCount> coefficients{
        juce::IIRCoefficients::makeLowShelf(
            hostSampleRate, 140.0, 0.707,
            juce::Decibels::decibelsToGain(bandGains[0])),
        juce::IIRCoefficients::makePeakFilter(
            hostSampleRate, 1000.0, 0.9,
            juce::Decibels::decibelsToGain(bandGains[1])),
        juce::IIRCoefficients::makeHighShelf(
            hostSampleRate, 7000.0, 0.707,
            juce::Decibels::decibelsToGain(bandGains[2]))
    };
    for (auto& outputChannel
         : trackEqFilters[static_cast<size_t>(channel)])
        for (int band = 0; band < trackEqBandCount; ++band)
            outputChannel[static_cast<size_t>(band)].setCoefficients(
                coefficients[static_cast<size_t>(band)]);
}

void GrooveEngine::start()
{
    const juce::ScopedLock scoped(lock);
    currentStep.store(-1);
    samplesUntilNextStep = 0.0;
    for (auto& voice : voices)
        voice.position = -1.0;
    playing.store(true);
}

void GrooveEngine::stop()
{
    playing.store(false);
    currentStep.store(-1);
}

bool GrooveEngine::exportLoop(const juce::File& destination, int bars)
{
    constexpr double exportRate = 44100.0;
    std::array<std::shared_ptr<LoadedAudioSample>, channelCount> localSamples;
    std::array<std::array<bool, stepCount>, channelCount> localPattern{};
    std::array<float, channelCount> localGains{};
    std::array<std::array<float, trackEqBandCount>, channelCount>
        localTrackEqGains{};
    {
        const juce::ScopedLock scoped(lock);
        localSamples = samples;
        localPattern = pattern;
        localGains = gains;
        localTrackEqGains = trackEqGains;
    }

    const auto stepLength = exportRate * 60.0
        / (juce::jmax(30.0, bpm.load()) * stepsPerBeat.load());
    const auto totalSteps = juce::jmax(1, bars) * 4 * stepsPerBeat.load();
    auto maxTail = 0;
    for (const auto& sample : localSamples)
        if (sample != nullptr)
            maxTail = juce::jmax(
                maxTail, static_cast<int>(sample->audio.getNumSamples()
                    * exportRate / sample->sampleRate));
    const auto totalLength = static_cast<int>(
        std::ceil(totalSteps * stepLength)) + maxTail;
    juce::AudioBuffer<float> result(2, totalLength);
    result.clear();

    for (int step = 0; step < totalSteps; ++step)
    {
        const auto patternStep = step
            % juce::jlimit(1, stepCount, loopLength.load());
        const auto start = static_cast<int>(std::round(step * stepLength));
        for (int channel = 0; channel < channelCount; ++channel)
        {
            const auto& sample = localSamples[static_cast<size_t>(channel)];
            if (sample == nullptr
                || !localPattern[static_cast<size_t>(channel)]
                                [static_cast<size_t>(patternStep)])
                continue;
            const auto increment = sample->sampleRate / exportRate;
            auto position = 0.0;
            std::array<std::array<juce::IIRFilter, trackEqBandCount>, 2>
                exportFilters;
            const auto& eqGains =
                localTrackEqGains[static_cast<size_t>(channel)];
            const std::array<juce::IIRCoefficients, trackEqBandCount>
                coefficients{
                    juce::IIRCoefficients::makeLowShelf(
                        exportRate, 140.0, 0.707,
                        juce::Decibels::decibelsToGain(eqGains[0])),
                    juce::IIRCoefficients::makePeakFilter(
                        exportRate, 1000.0, 0.9,
                        juce::Decibels::decibelsToGain(eqGains[1])),
                    juce::IIRCoefficients::makeHighShelf(
                        exportRate, 7000.0, 0.707,
                        juce::Decibels::decibelsToGain(eqGains[2]))
                };
            for (auto& outputChannel : exportFilters)
                for (int band = 0; band < trackEqBandCount; ++band)
                    outputChannel[static_cast<size_t>(band)].setCoefficients(
                        coefficients[static_cast<size_t>(band)]);
            for (int frame = start; frame < totalLength
                 && position < sample->audio.getNumSamples(); ++frame)
            {
                for (int out = 0; out < 2; ++out)
                {
                    auto value = interpolatedSample(*sample, out, position);
                    for (auto& filter
                         : exportFilters[static_cast<size_t>(out)])
                        value = filter.processSingleSampleRaw(value);
                    result.addSample(
                        out, frame, value
                            * localGains[static_cast<size_t>(channel)]);
                }
                position += increment;
            }
        }
    }
    return writeWav(destination, result, exportRate);
}

juce::String GrooveEngine::getSamplePath(int channel) const
{
    if (!juce::isPositiveAndBelow(channel, channelCount))
        return {};
    const juce::ScopedLock scoped(lock);
    return samplePaths[static_cast<size_t>(channel)];
}

juce::String GrooveEngine::getPatternData() const
{
    const juce::ScopedLock scoped(lock);
    juce::String result;
    result.preallocateBytes(channelCount * stepCount);
    for (int channel = 0; channel < channelCount; ++channel)
        for (int step = 0; step < stepCount; ++step)
            result += pattern[static_cast<size_t>(channel)]
                              [static_cast<size_t>(step)] ? '1' : '0';
    return result;
}

void GrooveEngine::setPatternData(const juce::String& data)
{
    const juce::ScopedLock scoped(lock);
    for (auto& row : pattern)
        row.fill(false);
    const auto length = juce::jmin(channelCount * stepCount, data.length());
    for (int index = 0; index < length; ++index)
    {
        const auto channel = index / stepCount;
        const auto step = index % stepCount;
        pattern[static_cast<size_t>(channel)][static_cast<size_t>(step)] =
            data[index] == '1';
    }
}

void BroncoPianoEngine::prepare(double sampleRate)
{
    const juce::ScopedLock scoped(lock);
    hostSampleRate = sampleRate;
}

void BroncoPianoEngine::process(juce::AudioBuffer<float>& output)
{
    const juce::ScopedTryLock scoped(lock);
    if (!scoped.isLocked())
        return;

    const auto numSamples = output.getNumSamples();
    for (int frame = 0; frame < numSamples; ++frame)
    {
        if (playback.load())
        {
            while (nextPlaybackEvent < recordingEvents.size()
                && recordingEvents[nextPlaybackEvent].startSeconds
                    <= playbackPositionSeconds)
            {
                const auto& event = recordingEvents[nextPlaybackEvent++];
                triggerNoteLocked(event.midiNote, event.velocity);
            }
            if (nextPlaybackEvent >= recordingEvents.size()
                && std::none_of(voices.begin(), voices.end(),
                                [](const Voice& voice) { return voice.active; }))
                playback.store(false);
        }

        for (auto& voice : voices)
        {
            if (!voice.active || voice.sample == nullptr)
                continue;
            for (int channel = 0; channel < output.getNumChannels(); ++channel)
                output.addSample(
                    channel, frame,
                    interpolatedSample(*voice.sample, channel, voice.position)
                        * voice.velocity);
            voice.position += voice.sample->sampleRate / hostSampleRate;
            if (voice.position >= voice.sample->audio.getNumSamples())
                voice.active = false;
        }
        playbackPositionSeconds += 1.0 / hostSampleRate;
    }
}

int BroncoPianoEngine::loadLibrary(const juce::File& folder)
{
    auto files = folder.findChildFiles(
        juce::File::findFiles, false, "note_*.wav");
    auto loadedCount = 0;
    const juce::ScopedLock scoped(lock);
    notes.fill(nullptr);
    for (const auto& file : files)
    {
        const auto number = file.getFileNameWithoutExtension()
            .fromFirstOccurrenceOf("note_", false, true).getIntValue();
        if (!juce::isPositiveAndBelow(number, 128))
            continue;
        auto sample = loadAudioFile(file);
        if (sample != nullptr)
        {
            notes[static_cast<size_t>(number)] = std::move(sample);
            ++loadedCount;
        }
    }
    return loadedCount;
}

void BroncoPianoEngine::triggerNoteLocked(int midiNote, float velocity)
{
    if (!juce::isPositiveAndBelow(midiNote, 128))
        return;
    const auto& sample = notes[static_cast<size_t>(midiNote)];
    if (sample == nullptr)
        return;
    auto* voice = &voices[0];
    for (auto& candidate : voices)
        if (!candidate.active)
        {
            voice = &candidate;
            break;
        }
    voice->sample = sample;
    voice->position = 0.0;
    voice->velocity = juce::jlimit(0.0f, 1.0f, velocity);
    voice->active = true;
}

void BroncoPianoEngine::noteOn(int midiNote, float velocity)
{
    const juce::ScopedLock scoped(lock);
    triggerNoteLocked(midiNote, velocity);
    if (recording.load())
        recordingEvents.push_back({
            midiNote, velocity,
            (juce::Time::getMillisecondCounterHiRes() - recordStartTimeMs)
                / 1000.0
        });
}

void BroncoPianoEngine::startRecording()
{
    const juce::ScopedLock scoped(lock);
    recordingEvents.clear();
    recordStartTimeMs = juce::Time::getMillisecondCounterHiRes();
    recording.store(true);
    playback.store(false);
}

void BroncoPianoEngine::stopRecording()
{
    recording.store(false);
}

void BroncoPianoEngine::playRecording()
{
    const juce::ScopedLock scoped(lock);
    recording.store(false);
    playbackPositionSeconds = 0.0;
    nextPlaybackEvent = 0;
    playback.store(!recordingEvents.empty());
}

void BroncoPianoEngine::stopPlayback()
{
    const juce::ScopedLock scoped(lock);
    playback.store(false);
    for (auto& voice : voices)
        voice.active = false;
}

int BroncoPianoEngine::getRecordedNoteCount() const
{
    const juce::ScopedLock scoped(lock);
    return static_cast<int>(recordingEvents.size());
}

bool BroncoPianoEngine::exportRecording(const juce::File& destination)
{
    constexpr double exportRate = 44100.0;
    std::vector<RecordedNote> events;
    std::array<std::shared_ptr<LoadedAudioSample>, 128> localNotes;
    {
        const juce::ScopedLock scoped(lock);
        events = recordingEvents;
        localNotes = notes;
    }
    if (events.empty())
        return false;

    auto duration = events.back().startSeconds;
    for (const auto& event : events)
        if (const auto& sample = localNotes[static_cast<size_t>(event.midiNote)])
            duration = juce::jmax(
                duration, event.startSeconds
                    + sample->audio.getNumSamples() / sample->sampleRate);
    juce::AudioBuffer<float> result(
        2, static_cast<int>(std::ceil(duration * exportRate)) + 1);
    result.clear();

    for (const auto& event : events)
    {
        const auto& sample = localNotes[static_cast<size_t>(event.midiNote)];
        if (sample == nullptr)
            continue;
        const auto start = static_cast<int>(
            std::round(event.startSeconds * exportRate));
        auto position = 0.0;
        const auto increment = sample->sampleRate / exportRate;
        for (int frame = start; frame < result.getNumSamples()
             && position < sample->audio.getNumSamples(); ++frame)
        {
            for (int channel = 0; channel < 2; ++channel)
                result.addSample(
                    channel, frame,
                    interpolatedSample(*sample, channel, position)
                        * event.velocity);
            position += increment;
        }
    }
    return writeWav(destination, result, exportRate);
}
