#include "PluginEditor.h"

SampleWaveform::SampleWaveform()
{
    formats.registerBasicFormats();
    thumbnail.addChangeListener(this);
}

SampleWaveform::~SampleWaveform()
{
    thumbnail.removeChangeListener(this);
}

void SampleWaveform::setFile(const juce::File& file)
{
    fileName = file.getFileName();
    thumbnail.clear();
    if (file.existsAsFile())
        thumbnail.setSource(new juce::FileInputSource(file));
    repaint();
}

void SampleWaveform::paint(juce::Graphics& graphics)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    juce::ColourGradient background(
        MiguelColours::panelRaised(), bounds.getCentreX(), bounds.getY(),
        MiguelColours::panel(), bounds.getCentreX(), bounds.getBottom(), false);
    graphics.setGradientFill(background);
    graphics.fillRoundedRectangle(bounds, 7.0f);
    graphics.setColour(MiguelColours::cyan().withAlpha(0.8f));
    graphics.drawRoundedRectangle(bounds, 7.0f, 1.0f);

    if (thumbnail.getTotalLength() > 0.0)
    {
        graphics.setColour(MiguelColours::cyan());
        thumbnail.drawChannels(graphics, getLocalBounds().reduced(8),
                               0.0, thumbnail.getTotalLength(), 1.0f);
    }
    else
    {
        graphics.setColour(MiguelColours::textMuted());
        graphics.drawFittedText(
            fileName.isEmpty() ? "Selecciona un sample" : "Cargando " + fileName,
            getLocalBounds(), juce::Justification::centred, 1);
    }
}

void TunerNeedle::setReading(double centsToUse,
                             const juce::String& noteToUse,
                             double frequencyToUse)
{
    cents = juce::jlimit(-50.0, 50.0, centsToUse);
    note = noteToUse;
    frequency = frequencyToUse;
    hasReading = true;
    repaint();
}

void TunerNeedle::clear()
{
    hasReading = false;
    note = "-";
    frequency = 0.0;
    cents = 0.0;
    repaint();
}

void TunerNeedle::paint(juce::Graphics& graphics)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    juce::ColourGradient background(
        MiguelColours::panelRaised(), bounds.getCentreX(), bounds.getY(),
        MiguelColours::panel(), bounds.getCentreX(), bounds.getBottom(), false);
    graphics.setGradientFill(background);
    graphics.fillRoundedRectangle(bounds, 7.0f);
    graphics.setColour(MiguelColours::cyan().withAlpha(0.8f));
    graphics.drawRoundedRectangle(bounds, 7.0f, 1.0f);

    const auto centre = juce::Point<float>(
        getWidth() * 0.5f, getHeight() - 18.0f);
    const auto radius = juce::jmin(getWidth() * 0.42f,
                                   getHeight() * 0.72f);

    for (int tick = -50; tick <= 50; tick += 10)
    {
        const auto proportion = (tick + 50.0f) / 100.0f;
        const auto angle = juce::MathConstants<float>::pi
            * (-0.75f + proportion * 0.5f);
        const auto outer = centre + juce::Point<float>(
            std::cos(angle) * radius, std::sin(angle) * radius);
        const auto inner = centre + juce::Point<float>(
            std::cos(angle) * (radius - (tick == 0 ? 13.0f : 8.0f)),
            std::sin(angle) * (radius - (tick == 0 ? 13.0f : 8.0f)));
        graphics.setColour(tick == 0 ? MiguelColours::green()
                                     : MiguelColours::textMuted());
        graphics.drawLine({ inner, outer }, tick == 0 ? 2.5f : 1.2f);
    }

    const auto proportion = static_cast<float>((cents + 50.0) / 100.0);
    const auto needleAngle = juce::MathConstants<float>::pi
        * (-0.75f + proportion * 0.5f);
    const auto tip = centre + juce::Point<float>(
        std::cos(needleAngle) * (radius - 17.0f),
        std::sin(needleAngle) * (radius - 17.0f));
    graphics.setColour(hasReading ? MiguelColours::yellow()
                                  : MiguelColours::border());
    graphics.drawLine({ centre, tip }, 3.0f);
    graphics.fillEllipse(centre.x - 5.0f, centre.y - 5.0f, 10.0f, 10.0f);

    graphics.setColour(MiguelColours::text());
    graphics.setFont(juce::FontOptions(21.0f, juce::Font::bold));
    graphics.drawText(note, 8, 7, getWidth() - 16, 25,
                      juce::Justification::centred);
    graphics.setFont(juce::FontOptions(12.0f));
    const auto detail = hasReading
        ? juce::String(frequency, 2) + " Hz  /  "
            + juce::String(cents, 1) + " cents"
        : "Sin lectura";
    graphics.drawText(detail, 8, 31, getWidth() - 16, 20,
                      juce::Justification::centred);
}

namespace
{
struct PitchResult
{
    bool detected = false;
    double frequency = 0.0;
    double cents = 0.0;
    double midiExact = 0.0;
    juce::String note;
};

PitchResult detectPitch(const juce::File& file)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(
        formats.createReaderFor(file));
    if (reader == nullptr || reader->sampleRate <= 0.0
        || reader->lengthInSamples < 2048)
        return {};

    const auto samplesToRead = static_cast<int>(juce::jmin<juce::int64>(
        reader->lengthInSamples,
        static_cast<juce::int64>(reader->sampleRate * 3.0)));
    juce::AudioBuffer<float> source(1, samplesToRead);
    if (!reader->read(&source, 0, samplesToRead, 0, true, false))
        return {};

    const auto windowSize = juce::jmin(8192, samplesToRead);
    auto bestStart = 0;
    auto bestEnergy = 0.0;
    for (int start = 0; start + windowSize <= samplesToRead;
         start += juce::jmax(512, windowSize / 4))
    {
        auto energy = 0.0;
        for (int sample = 0; sample < windowSize; ++sample)
        {
            const auto value = source.getSample(0, start + sample);
            energy += value * value;
        }
        if (energy > bestEnergy)
        {
            bestEnergy = energy;
            bestStart = start;
        }
    }

    if (bestEnergy / windowSize < 1.0e-6)
        return {};

    std::vector<double> signal(static_cast<size_t>(windowSize));
    auto mean = 0.0;
    for (int sample = 0; sample < windowSize; ++sample)
        mean += source.getSample(0, bestStart + sample);
    mean /= windowSize;
    for (int sample = 0; sample < windowSize; ++sample)
        signal[static_cast<size_t>(sample)]
            = source.getSample(0, bestStart + sample) - mean;

    const auto minLag = juce::jmax(
        2, static_cast<int>(reader->sampleRate / 2000.0));
    const auto maxLag = juce::jmin(
        windowSize / 2, static_cast<int>(reader->sampleRate / 40.0));
    if (minLag >= maxLag)
        return {};

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

    auto candidate = -1;
    for (int lag = minLag; lag < maxLag; ++lag)
    {
        if (normalized[static_cast<size_t>(lag)] < 0.18
            && normalized[static_cast<size_t>(lag)]
                <= normalized[static_cast<size_t>(lag + 1)])
        {
            candidate = lag;
            break;
        }
    }
    if (candidate < 0)
    {
        candidate = minLag;
        for (int lag = minLag + 1; lag <= maxLag; ++lag)
            if (normalized[static_cast<size_t>(lag)]
                < normalized[static_cast<size_t>(candidate)])
                candidate = lag;
    }

    const auto confidence =
        1.0 - normalized[static_cast<size_t>(candidate)];
    if (confidence < 0.55)
        return {};

    auto refinedLag = static_cast<double>(candidate);
    if (candidate > minLag && candidate < maxLag)
    {
        const auto before = normalized[static_cast<size_t>(candidate - 1)];
        const auto centre = normalized[static_cast<size_t>(candidate)];
        const auto after = normalized[static_cast<size_t>(candidate + 1)];
        const auto denominator = before - 2.0 * centre + after;
        if (std::abs(denominator) > 1.0e-12)
            refinedLag += 0.5 * (before - after) / denominator;
    }

    const auto frequency = reader->sampleRate / refinedLag;
    const auto midiNote = 69.0 + 12.0 * std::log2(frequency / 440.0);
    const auto nearestNote = static_cast<int>(std::round(midiNote));
    const auto pitchClass = (nearestNote % 12 + 12) % 12;

    PitchResult result;
    result.detected = true;
    result.frequency = frequency;
    result.midiExact = midiNote;
    result.cents = (midiNote - nearestNote) * 100.0;
    result.note = MusicGenerator::noteName(pitchClass)
        + juce::String(nearestNote / 12 - 1);
    return result;
}

bool renderPitchShiftedFile(const juce::File& sourceFile,
                            const juce::File& destination,
                            double semitones,
                            double broncoMaxPercent)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(
        formats.createReaderFor(sourceFile));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return false;

    const auto channels = juce::jlimit(
        1, 2, static_cast<int>(reader->numChannels));
    const auto inputLength = static_cast<int>(juce::jmin<juce::int64>(
        reader->lengthInSamples, std::numeric_limits<int>::max() - 16));
    juce::AudioBuffer<float> input(channels, inputLength + 16);
    input.clear();
    if (!reader->read(&input, 0, inputLength, 0, true, true))
        return false;

    const auto ratio = std::pow(2.0, semitones / 12.0);
    const auto outputLength = juce::jmax(
        1, static_cast<int>(std::round(inputLength / ratio)));
    juce::AudioBuffer<float> output(channels, outputLength);
    for (int channel = 0; channel < channels; ++channel)
    {
        juce::LagrangeInterpolator interpolator;
        interpolator.process(ratio, input.getReadPointer(channel),
                             output.getWritePointer(channel), outputLength);
    }

    const auto broncoMix = juce::jlimit(
        0.0, 1.0, broncoMaxPercent / 100.0);
    if (broncoMix > 0.0)
    {
        juce::AudioBuffer<float> wet(channels, outputLength);
        wet.clear();
        const auto attackLength = juce::jmin(
            outputLength, static_cast<int>(reader->sampleRate * 0.020));
        const auto delay = static_cast<int>(reader->sampleRate * 0.0014);

        for (int channel = 0; channel < channels; ++channel)
        {
            const auto* dry = output.getReadPointer(channel);
            auto* bronco = wet.getWritePointer(channel);
            const auto detuneCents = channel % 2 == 0 ? 3.5 : -3.5;
            const auto detuneRatio = std::pow(2.0, detuneCents / 1200.0);

            for (int sample = 0; sample < outputLength; ++sample)
            {
                const auto attackGain = sample < attackLength
                    ? 1.0 + 0.55
                        * (1.0 - static_cast<double>(sample)
                            / juce::jmax(1, attackLength))
                    : 1.0;
                bronco[sample] += static_cast<float>(
                    dry[sample] * attackGain);

                const auto destinationSample = sample + delay;
                const auto sourcePosition = sample * detuneRatio;
                const auto sourceIndex = static_cast<int>(sourcePosition);
                if (destinationSample < outputLength
                    && sourceIndex + 1 < outputLength)
                {
                    const auto fraction = static_cast<float>(
                        sourcePosition - sourceIndex);
                    const auto detuned = dry[sourceIndex] * (1.0f - fraction)
                        + dry[sourceIndex + 1] * fraction;
                    bronco[destinationSample] += detuned * 0.82f;
                }
            }
        }

        const auto wetPeak = wet.getMagnitude(0, outputLength);
        if (wetPeak > 0.0f)
            wet.applyGain(0.9f / wetPeak);

        const auto muteStart = juce::jmin(
            outputLength, static_cast<int>(reader->sampleRate * 0.405));
        const auto muteLength = juce::jmin(
            outputLength - muteStart,
            static_cast<int>(reader->sampleRate * 0.018));
        for (int channel = 0; channel < channels; ++channel)
        {
            auto* wetData = wet.getWritePointer(channel);
            for (int sample = 0; sample < muteLength; ++sample)
                wetData[muteStart + sample] *= static_cast<float>(
                    1.0 - static_cast<double>(sample)
                        / juce::jmax(1, muteLength));
            juce::FloatVectorOperations::clear(
                wetData + muteStart + muteLength,
                outputLength - muteStart - muteLength);

            auto* result = output.getWritePointer(channel);
            for (int sample = 0; sample < outputLength; ++sample)
                result[sample] = static_cast<float>(
                    result[sample] * (1.0 - broncoMix)
                    + wetData[sample] * broncoMix);
        }
    }

    destination.deleteFile();
    std::unique_ptr<juce::OutputStream> stream =
        destination.createOutputStream();
    if (stream == nullptr)
        return false;

    juce::WavAudioFormat wav;
    const auto options = juce::AudioFormatWriterOptions{}
        .withSampleRate(reader->sampleRate)
        .withNumChannels(channels)
        .withBitsPerSample(24);
    auto writer = wav.createWriterFor(stream, options);
    return writer != nullptr
        && writer->writeFromAudioSampleBuffer(output, 0, outputLength);
}

void configureSlider(juce::Slider& slider, double minimum, double maximum,
                     double interval, double value, const juce::String& suffix)
{
    slider.setRange(minimum, maximum, interval);
    slider.setValue(value);
    slider.setTextValueSuffix(suffix);
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 24);
}

class TrackEqPopup final : public juce::Component
{
public:
    TrackEqPopup(GrooveEngine& engineToUse, int channelToUse)
        : engine(engineToUse), channel(channelToUse)
    {
        setSize(300, 150);
        for (int band = 0; band < GrooveEngine::trackEqBandCount; ++band)
        {
            auto& slider = sliders[static_cast<size_t>(band)];
            addAndMakeVisible(slider);
            slider.setRange(-18.0, 18.0, 0.1);
            slider.setValue(engine.getTrackEqGain(channel, band),
                            juce::dontSendNotification);
            slider.setTextValueSuffix(" dB");
            slider.setDoubleClickReturnValue(true, 0.0);
            slider.setColour(juce::Slider::rotarySliderFillColourId,
                             band == 0 ? MiguelColours::green()
                                       : band == 1 ? MiguelColours::cyan()
                                                   : MiguelColours::orange());
            slider.onValueChange = [this, band]
            {
                engine.setTrackEqGain(
                    channel, band,
                    static_cast<float>(
                        sliders[static_cast<size_t>(band)].getValue()));
            };
        }
    }

    void paint(juce::Graphics& graphics) override
    {
        graphics.fillAll(MiguelColours::panelRaised());
        graphics.setColour(MiguelColours::text());
        graphics.setFont(juce::FontOptions(15.0f, juce::Font::bold));
        graphics.drawText("EQ / Pista " + juce::String(channel + 1),
                          12, 7, getWidth() - 24, 22,
                          juce::Justification::centredLeft);
        static const juce::StringArray names{ "LOW", "MID", "HIGH" };
        graphics.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        for (int band = 0; band < GrooveEngine::trackEqBandCount; ++band)
            graphics.drawText(names[band], band * 96 + 7, 31, 90, 18,
                              juce::Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds().withTrimmedTop(48).reduced(6);
        const auto width = area.getWidth() / GrooveEngine::trackEqBandCount;
        for (int band = 0; band < GrooveEngine::trackEqBandCount; ++band)
            sliders[static_cast<size_t>(band)].setBounds(
                area.removeFromLeft(width).reduced(4));
    }

private:
    GrooveEngine& engine;
    int channel;
    std::array<PrecisionRotarySlider, GrooveEngine::trackEqBandCount> sliders;
};

void configureLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, MiguelColours::text());
}

juce::String foldButtonText(bool expanded, const juce::String& label)
{
    return (expanded ? "[v] " : "[>] ") + label;
}
}

MiguelMusicAssistantAudioProcessorEditor::
MiguelMusicAssistantAudioProcessorEditor(
    MiguelMusicAssistantAudioProcessor& processorToUse)
    : AudioProcessorEditor(&processorToUse), processor(processorToUse)
{
    setLookAndFeel(&miguelLookAndFeel);
    setSize(1280, 800);
    setResizable(true, true);
    setResizeLimits(960, 640, 1920, 1200);

    addAndMakeVisible(tabs);
    tabs.setTabBarDepth(38);
    tabs.setColour(juce::TabbedComponent::backgroundColourId,
                   MiguelColours::background());
    tabs.addTab("Generador", MiguelColours::cyan(), &generatorPage, false);
    tabs.addTab("Mezcla", MiguelColours::green(), &mixPage, false);
    tabs.addTab("Samples", MiguelColours::orange(), &libraryPage, false);
    tabs.addTab("Acordes Bajoquinto", MiguelColours::purple(),
                &bajoquintoPage, false);
    tabs.addTab("Ritmos y Piano", MiguelColours::pink(),
                &studioPage, false);
    tabs.addTab("EQ 7 Bandas", MiguelColours::yellow(),
                &eqPage, false);

    for (auto* child : { static_cast<juce::Component*>(&generatorTitle),
                         static_cast<juce::Component*>(&keyBox),
                         static_cast<juce::Component*>(&modeBox),
                         static_cast<juce::Component*>(&barsBox),
                         static_cast<juce::Component*>(&bpmSlider),
                         static_cast<juce::Component*>(&humanizeSlider),
                         static_cast<juce::Component*>(&keyLabel),
                         static_cast<juce::Component*>(&modeLabel),
                         static_cast<juce::Component*>(&barsLabel),
                         static_cast<juce::Component*>(&bpmLabel),
                         static_cast<juce::Component*>(&humanizeLabel),
                         static_cast<juce::Component*>(&exportButton),
                         static_cast<juce::Component*>(&dragButton),
                         static_cast<juce::Component*>(&previewMidiButton),
                         static_cast<juce::Component*>(&stopPreviewButton),
                         static_cast<juce::Component*>(&generatorStatus),
                         static_cast<juce::Component*>(&generatorPianoRoll) })
        generatorPage.addAndMakeVisible(child);

    keyBox.addItemList({ "C", "C#", "D", "D#", "E", "F",
                         "F#", "G", "G#", "A", "A#", "B" }, 1);
    keyBox.setSelectedId(1);
    modeBox.addItemList({ "Mayor", "Menor" }, 1);
    modeBox.setSelectedId(1);
    barsBox.addItemList({ "4", "8", "16" }, 1);
    barsBox.setSelectedId(1);
    configureSlider(bpmSlider, 40.0, 240.0, 1.0, 120.0, " BPM");
    configureSlider(humanizeSlider, 0.0, 100.0, 1.0, 8.0, " %");
    configureLabel(generatorTitle, "Generador MIDI");
    generatorTitle.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    configureLabel(keyLabel, "Tonalidad");
    configureLabel(modeLabel, "Escala");
    configureLabel(barsLabel, "Compases");
    configureLabel(bpmLabel, "Tempo");
    configureLabel(humanizeLabel, "Humanización");
    configureLabel(generatorStatus,
                   "Genera acordes I-V-vi-IV y una melodía compatible.");
    generatorPianoRoll.setMidiFile(
        MusicGenerator::createSong(currentSettings()),
        static_cast<int>(barsBox.getText().getIntValue()));

    exportButton.onClick = [this] { exportMidi(); };
    dragButton.onClick = [this]
    {
        auto folder = juce::File::getSpecialLocation(
            juce::File::userDocumentsDirectory)
            .getChildFile("Miguel Music Assistant Exports");
        folder.createDirectory();
        folder.revealToUser();
    };
    previewMidiButton.onClick = [this]
    {
        const auto settings = currentSettings();
        const auto song = MusicGenerator::createSong(settings);
        generatorPianoRoll.setMidiFile(song, settings.bars);
        processor.startMidiPreview(song, settings.bpm);
        generatorStatus.setText("Reproduciendo acordes y melodía...",
                                juce::dontSendNotification);
    };
    stopPreviewButton.onClick = [this] { processor.stopPreviews(); };

    mixPage.addAndMakeVisible(levelTitle);
    mixPage.addAndMakeVisible(levelReadout);
    mixPage.addAndMakeVisible(mixSuggestion);
    mixPage.addAndMakeVisible(mixAnalyzer);
    configureLabel(levelTitle, "Análisis en tiempo real");
    levelTitle.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    configureLabel(levelReadout, "Esperando audio...");
    levelReadout.setFont(juce::FontOptions(19.0f));
    configureLabel(mixSuggestion,
                   "Reproduce audio a través del plugin para recibir sugerencias.");
    mixSuggestion.setJustificationType(juce::Justification::topLeft);

    libraryPage.addAndMakeVisible(libraryTitle);
    libraryPage.addAndMakeVisible(folderButton);
    libraryPage.addAndMakeVisible(removeSampleButton);
    libraryPage.addAndMakeVisible(folderLabel);
    libraryPage.addAndMakeVisible(sampleWaveform);
    libraryPage.addAndMakeVisible(samplePlayButton);
    libraryPage.addAndMakeVisible(sampleStopButton);
    libraryPage.addAndMakeVisible(sampleDragButton);
    libraryPage.addAndMakeVisible(sampleInfo);
    libraryPage.addAndMakeVisible(tunerTitle);
    libraryPage.addAndMakeVisible(tunerReadout);
    libraryPage.addAndMakeVisible(tunerNeedle);
    libraryPage.addAndMakeVisible(pitchShiftKnob);
    libraryPage.addAndMakeVisible(pitchShiftLabel);
    libraryPage.addAndMakeVisible(broncoMaxKnob);
    libraryPage.addAndMakeVisible(broncoMaxLabel);
    libraryPage.addAndMakeVisible(analysePitchButton);
    libraryPage.addAndMakeVisible(auditionTunedButton);
    libraryPage.addAndMakeVisible(exportTunedButton);
    libraryPage.addAndMakeVisible(importedSampleList);
    configureLabel(libraryTitle, "Biblioteca de Samples");
    libraryTitle.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    configureLabel(folderLabel, "0 samples importados");
    configureLabel(sampleInfo, "Selecciona un archivo; doble clic para escucharlo.");
    configureLabel(tunerTitle, "Afinador de samples");
    tunerTitle.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    configureLabel(tunerReadout, "Tono: esperando sample");
    configureLabel(pitchShiftLabel, "Cambio de tono");
    pitchShiftLabel.setJustificationType(juce::Justification::centred);
    pitchShiftKnob.setRange(-12.0, 12.0, 0.01);
    pitchShiftKnob.setValue(0.0);
    pitchShiftKnob.setTextValueSuffix(" st");
    pitchShiftKnob.setDoubleClickReturnValue(true, 0.0);
    pitchShiftKnob.setTextBoxStyle(
        juce::Slider::TextBoxBelow, false, 82, 22);
    pitchShiftKnob.setColour(juce::Slider::rotarySliderFillColourId,
                             MiguelColours::orange());
    pitchShiftKnob.onValueChange = [this] { updateTunerDisplay(); };
    configureLabel(broncoMaxLabel, "Bronco Max");
    broncoMaxLabel.setJustificationType(juce::Justification::centred);
    broncoMaxLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    broncoMaxKnob.setRange(0.0, 100.0, 1.0);
    broncoMaxKnob.setValue(0.0);
    broncoMaxKnob.setTextValueSuffix(" %");
    broncoMaxKnob.setDoubleClickReturnValue(true, 0.0);
    broncoMaxKnob.setTextBoxStyle(
        juce::Slider::TextBoxBelow, false, 82, 22);
    broncoMaxKnob.setColour(juce::Slider::rotarySliderFillColourId,
                           MiguelColours::orange());
    broncoMaxKnob.onValueChange = [this] { updateTunerDisplay(); };
    folderButton.onClick = [this] { chooseSampleFiles(); };
    removeSampleButton.onClick = [this]
    {
        importedSampleList.removeSelected();
    };
    importedSampleList.onSelectionChanged = [this](const juce::File& file)
    {
        if (file.existsAsFile())
            updateSelectedSample(file);
        else
        {
            selectedSample = {};
            sampleWaveform.setFile({});
            sampleInfo.setText("Agrega o arrastra un archivo de audio.",
                               juce::dontSendNotification);
        }
    };
    importedSampleList.onFileDoubleClicked = [this](const juce::File& file)
    {
        updateSelectedSample(file);
        if (selectedSample.existsAsFile())
            processor.playSample();
    };
    importedSampleList.onSampleCountChanged = [this](int count)
    {
        folderLabel.setText(juce::String(count)
                                + (count == 1 ? " sample importado"
                                              : " samples importados"),
                            juce::dontSendNotification);
    };
    samplePlayButton.onClick = [this]
    {
        if (selectedSample.existsAsFile())
            processor.playSample();
    };
    sampleStopButton.onClick = [this] { processor.stopPreviews(); };
    sampleDragButton.onClick = [this]
    {
        if (selectedSample.existsAsFile())
            selectedSample.revealToUser();
    };
    analysePitchButton.onClick = [this] { analyseSelectedSamplePitch(); };
    auditionTunedButton.onClick = [this] { processTunedSample(false); };
    exportTunedButton.onClick = [this] { processTunedSample(true); };

    for (auto* child : {
             static_cast<juce::Component*>(&bajoquintoTitle),
             static_cast<juce::Component*>(&bajoquintoDescription),
             static_cast<juce::Component*>(&bajoquintoStyleBox),
             static_cast<juce::Component*>(&chordRootBox),
             static_cast<juce::Component*>(&chordQualityBox),
             static_cast<juce::Component*>(&chordVoicingBox),
             static_cast<juce::Component*>(&bajoquintoStyleLabel),
             static_cast<juce::Component*>(&chordRootLabel),
             static_cast<juce::Component*>(&chordQualityLabel),
             static_cast<juce::Component*>(&chordVoicingLabel),
             static_cast<juce::Component*>(&generateChordsButton),
             static_cast<juce::Component*>(&previewChordButton),
             static_cast<juce::Component*>(&stopChordButton),
             static_cast<juce::Component*>(&openToneInputButton),
             static_cast<juce::Component*>(&openChordsButton),
             static_cast<juce::Component*>(&toneInputLabel),
             static_cast<juce::Component*>(&bajoquintoStatus) })
        bajoquintoPage.addAndMakeVisible(child);

    configureLabel(bajoquintoTitle, "Creador de Acordes - Bajoquinto");
    bajoquintoTitle.setFont(juce::FontOptions(25.0f, juce::Font::bold));
    configureLabel(
        bajoquintoDescription,
        "Síntesis desde notas monofónicas / 24 acordes cromáticos / "
        "voicings normal y bronco");
    configureLabel(bajoquintoStyleLabel, "Estilo");
    configureLabel(chordRootLabel, "Tónica");
    configureLabel(chordQualityLabel, "Tipo");
    configureLabel(chordVoicingLabel, "Voicing");
    configureLabel(
        toneInputLabel,
        "Entrada WAV: C:\\Users\\MIGUEL\\OneDrive2\\Desktop\\tonos");
    configureLabel(bajoquintoStatus, "Comprobando acordes existentes...");
    bajoquintoStatus.setJustificationType(juce::Justification::topLeft);

    bajoquintoStyleBox.addItemList({ "Normal", "Bronco" }, 1);
    bajoquintoStyleBox.setSelectedId(1);
    chordRootBox.addItemList(
        { "Do", "Do#", "Re", "Re#", "Mi", "Fa",
          "Fa#", "Sol", "Sol#", "La", "La#", "Si" }, 1);
    chordRootBox.setSelectedId(1);
    chordQualityBox.addItemList({ "Mayor", "Menor" }, 1);
    chordQualityBox.setSelectedId(1);
    chordVoicingBox.addItemList(
        { "Principal", "Cerrada media", "Cerrada aguda",
          "Tercera alta", "Tónica alta" }, 1);
    chordVoicingBox.setSelectedId(1);

    bajoquintoStyleBox.onChange = [this] { updateBajoquintoStatus(); };
    chordRootBox.onChange = [this] { updateBajoquintoStatus(); };
    chordQualityBox.onChange = [this] { updateBajoquintoStatus(); };
    chordVoicingBox.onChange = [this] { updateBajoquintoStatus(); };
    generateChordsButton.onClick = [this] { generateBajoquintoChords(); };
    previewChordButton.onClick = [this] { previewBajoquintoChord(); };
    stopChordButton.onClick = [this] { processor.stopPreviews(); };
    openToneInputButton.onClick = []
    {
        const auto folder = juce::File(
            "C:\\Users\\MIGUEL\\OneDrive2\\Desktop\\tonos");
        folder.createDirectory();
        folder.revealToUser();
    };
    openChordsButton.onClick = [this]
    {
        const auto folder = bajoquintoOutputFolder();
        folder.createDirectory();
        folder.revealToUser();
    };
    updateBajoquintoStatus();

    studioPage.addAndMakeVisible(studioTitle);
    studioPage.addAndMakeVisible(rhythmTitle);
    studioPage.addAndMakeVisible(rhythmFoldButton);
    studioPage.addAndMakeVisible(rhythmBpmSlider);
    studioPage.addAndMakeVisible(rhythmBpmLabel);
    studioPage.addAndMakeVisible(loopLengthBox);
    studioPage.addAndMakeVisible(loopLengthLabel);
    studioPage.addAndMakeVisible(exportBarsBox);
    studioPage.addAndMakeVisible(exportBarsLabel);
    studioPage.addAndMakeVisible(rhythmPlayButton);
    studioPage.addAndMakeVisible(rhythmStopButton);
    studioPage.addAndMakeVisible(rhythmClearButton);
    studioPage.addAndMakeVisible(rhythmExportButton);
    studioPage.addAndMakeVisible(drumLibraryBox);
    studioPage.addAndMakeVisible(drumLibraryLabel);
    studioPage.addAndMakeVisible(openDrumLibraryButton);
    studioPage.addAndMakeVisible(rhythmGridViewport);
    rhythmGridViewport.setViewedComponent(&rhythmGridContent, false);
    rhythmGridViewport.setScrollBarsShown(false, true);
    rhythmGridViewport.setScrollBarThickness(10);
    studioPage.addAndMakeVisible(pianoTitle);
    studioPage.addAndMakeVisible(pianoFoldButton);
    studioPage.addAndMakeVisible(pianoKeyboard);
    studioPage.addAndMakeVisible(preparePianoButton);
    studioPage.addAndMakeVisible(pianoRecordButton);
    studioPage.addAndMakeVisible(pianoStopButton);
    studioPage.addAndMakeVisible(pianoPlaybackButton);
    studioPage.addAndMakeVisible(pianoExportButton);
    studioPage.addAndMakeVisible(pianoStatus);

    configureLabel(studioTitle, "Estudio de Ritmos y Piano Bajo Sexto Bronco");
    studioTitle.setFont(juce::FontOptions(23.0f, juce::Font::bold));
    configureLabel(rhythmTitle,
                   "Step sequencer / 8 pistas / resolución hasta 1/64");
    rhythmTitle.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    configureLabel(rhythmBpmLabel, "Tempo");
    configureSlider(rhythmBpmSlider, 40.0, 240.0, 1.0, 120.0, " BPM");
    configureLabel(loopLengthLabel, "Loop");
    loopLengthBox.addItemList(
        { "1/4 - 16 pasos", "1/8 - 32 pasos", "1/16 - 64 pasos",
          "1/32 - 128 pasos", "1/64 - 256 pasos" }, 1);
    loopLengthBox.setSelectedId(2);
    configureLabel(exportBarsLabel, "Exportar");
    exportBarsBox.addItemList({ "1 compás", "2 compases",
                                "4 compases", "8 compases" }, 1);
    exportBarsBox.setSelectedId(3);
    configureLabel(drumLibraryLabel, "Biblioteca");
    drumLibraryBox.addItemList(
        { "Mis golpes, tambora y redobles",
          "WAV tambora y tarolas",
          "FL Cloud Sounds",
          "Carpeta Descargas" }, 1);
    drumLibraryBox.setSelectedId(1);
    openDrumLibraryButton.onClick = [this]
    {
        const auto folder = selectedDrumLibrary();
        if (folder.isDirectory())
            folder.revealToUser();
    };

    rhythmBpmSlider.onValueChange = [this]
    {
        processor.getGrooveEngine().setBpm(rhythmBpmSlider.getValue());
    };
    loopLengthBox.onChange = [this]
    {
        static constexpr std::array<int, 5> resolutions{ 4, 8, 16, 32, 64 };
        processor.getGrooveEngine().setGridResolution(
            resolutions[static_cast<size_t>(
                juce::jmax(0, loopLengthBox.getSelectedItemIndex()))]);
        resized();
    };
    rhythmPlayButton.onClick = [this]
    {
        processor.getGrooveEngine().start();
    };
    rhythmStopButton.onClick = [this]
    {
        processor.getGrooveEngine().stop();
    };
    rhythmClearButton.onClick = [this]
    {
        processor.getGrooveEngine().clearPattern();
        for (auto& row : rhythmSteps)
            for (auto& button : row)
                button.setToggleState(false, juce::dontSendNotification);
    };
    rhythmExportButton.onClick = [this] { exportRhythmLoop(); };

    for (int channel = 0; channel < GrooveEngine::channelCount; ++channel)
    {
        auto& loadButton = rhythmLoadButtons[static_cast<size_t>(channel)];
        auto& eqButton = rhythmEqButtons[static_cast<size_t>(channel)];
        auto& channelLabel = rhythmChannelLabels[static_cast<size_t>(channel)];
        auto& gain = rhythmGainSliders[static_cast<size_t>(channel)];
        studioPage.addAndMakeVisible(loadButton);
        studioPage.addAndMakeVisible(eqButton);
        studioPage.addAndMakeVisible(channelLabel);
        studioPage.addAndMakeVisible(gain);
        loadButton.setButtonText("Cargar sample");
        loadButton.onClick = [this, channel] { chooseRhythmSample(channel); };
        eqButton.setButtonText("EQ");
        eqButton.onClick = [this, channel]
        {
            auto popup = std::make_unique<TrackEqPopup>(
                processor.getGrooveEngine(), channel);
            const auto target = getLocalArea(
                &rhythmEqButtons[static_cast<size_t>(channel)],
                rhythmEqButtons[static_cast<size_t>(channel)]
                    .getLocalBounds());
            juce::CallOutBox::launchAsynchronously(
                std::move(popup), target, this);
        };
        configureLabel(channelLabel, juce::String(channel + 1));
        channelLabel.setJustificationType(juce::Justification::centred);
        configureSlider(gain, 0.0, 1.5, 0.01, 0.85, "");
        gain.setTextBoxStyle(
            juce::Slider::TextBoxRight, false, 42, 20);
        gain.onValueChange = [this, channel]
        {
            processor.getGrooveEngine().setGain(
                channel, static_cast<float>(
                    rhythmGainSliders[static_cast<size_t>(channel)]
                        .getValue()));
        };

        for (int step = 0; step < GrooveEngine::stepCount; ++step)
        {
            auto& button = rhythmSteps[static_cast<size_t>(channel)]
                                       [static_cast<size_t>(step)];
            rhythmGridContent.addAndMakeVisible(button);
            button.setClickingTogglesState(true);
            button.setColour(
                juce::TextButton::buttonColourId,
                step % 8 < 4 ? juce::Colour(0xff171044)
                             : juce::Colour(0xff25206b));
            button.setColour(
                juce::TextButton::buttonOnColourId,
                juce::Colour(0xff7c3aed));
            button.onClick = [this, channel, step]
            {
                auto& cell = rhythmSteps[static_cast<size_t>(channel)]
                                         [static_cast<size_t>(step)];
                processor.getGrooveEngine().setStep(
                    channel, step, cell.getToggleState());
            };
        }
    }

    configureLabel(pianoTitle,
                   "Piano Bajo Sexto Bronco / notas individuales");
    pianoTitle.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    configureLabel(pianoStatus, "Preparando biblioteca del piano...");
    pianoKeyboard.setAvailableRange(40, 76);
    pianoKeyboard.setLowestVisibleKey(40);
    pianoKeyboard.setKeyWidth(24.0f);
    pianoKeyboardState.addListener(this);

    const auto pianoFolder = juce::File(
        "C:\\Users\\MIGUEL\\Documents\\Miguel Music Assistant\\Piano Bronco");
    const auto loadedNotes =
        processor.getPianoEngine().loadLibrary(pianoFolder);
    pianoStatus.setText(
        "Notas Bronco disponibles: " + juce::String(loadedNotes) + "/37",
        juce::dontSendNotification);
    preparePianoButton.onClick = [this] { prepareBroncoPiano(); };
    pianoRecordButton.onClick = [this]
    {
        processor.getPianoEngine().startRecording();
        pianoStatus.setText("Grabando interpretación...",
                            juce::dontSendNotification);
    };
    pianoStopButton.onClick = [this]
    {
        auto& piano = processor.getPianoEngine();
        piano.stopRecording();
        piano.stopPlayback();
        pianoStatus.setText(
            "Grabación detenida / "
                + juce::String(piano.getRecordedNoteCount()) + " notas",
            juce::dontSendNotification);
    };
    pianoPlaybackButton.onClick = [this]
    {
        processor.getPianoEngine().playRecording();
        pianoStatus.setText("Reproduciendo grabación...",
                            juce::dontSendNotification);
    };
    pianoExportButton.onClick = [this] { exportPianoRecording(); };
    rhythmFoldButton.onClick = [this]
    {
        rhythmExpanded = !rhythmExpanded;
        rhythmFoldButton.setButtonText(
            foldButtonText(rhythmExpanded, "Ritmos / Piano Roll"));
        updateFoldVisibility();
        resized();
    };
    pianoFoldButton.onClick = [this]
    {
        pianoExpanded = !pianoExpanded;
        pianoFoldButton.setButtonText(
            foldButtonText(pianoExpanded, "Piano Bajo Sexto Bronco"));
        updateFoldVisibility();
        resized();
    };

    eqPage.addAndMakeVisible(eqTitle);
    eqPage.addAndMakeVisible(eqSectionBox);
    eqPage.addAndMakeVisible(eqSectionLabel);
    eqPage.addAndMakeVisible(inputEqLabel);
    eqPage.addAndMakeVisible(outputEqLabel);
    eqPage.addAndMakeVisible(graphicEqDisplay);
    eqPage.addAndMakeVisible(inputEqFoldButton);
    eqPage.addAndMakeVisible(outputEqFoldButton);
    eqPage.addAndMakeVisible(sectionVolumeKnob);
    eqPage.addAndMakeVisible(sectionVolumeLabel);
    eqPage.addAndMakeVisible(sectionBroncoKnob);
    eqPage.addAndMakeVisible(sectionBroncoLabel);
    eqPage.addAndMakeVisible(resetEqButton);
    configureLabel(eqTitle, "Ecualización por Sección");
    eqTitle.setFont(juce::FontOptions(25.0f, juce::Font::bold));
    configureLabel(eqSectionLabel, "Sección de audio");
    eqSectionBox.addItemList(
        { "Generador", "Samples", "Acordes", "Ritmos", "Piano" }, 1);
    eqSectionBox.setSelectedId(1);
    configureLabel(inputEqLabel, "EQ DE ENTRADA / 7 BANDAS");
    inputEqLabel.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    configureLabel(outputEqLabel, "EQ DE SALIDA / 7 BANDAS");
    outputEqLabel.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    configureLabel(sectionVolumeLabel, "Volumen de la sección");
    sectionVolumeLabel.setJustificationType(juce::Justification::centred);
    sectionVolumeKnob.setRange(0.0, 2.0, 0.01);
    sectionVolumeKnob.setValue(1.0);
    sectionVolumeKnob.setTextValueSuffix(" x");
    sectionVolumeKnob.setDoubleClickReturnValue(true, 1.0);
    sectionVolumeKnob.setTextBoxStyle(
        juce::Slider::TextBoxBelow, false, 80, 24);
    sectionVolumeKnob.setColour(juce::Slider::rotarySliderFillColourId,
                               MiguelColours::yellow());
    configureLabel(sectionBroncoLabel, "Bronco Max");
    sectionBroncoLabel.setJustificationType(juce::Justification::centred);
    sectionBroncoLabel.setFont(
        juce::FontOptions(16.0f, juce::Font::bold));
    sectionBroncoKnob.setRange(0.0, 100.0, 1.0);
    sectionBroncoKnob.setValue(0.0);
    sectionBroncoKnob.setTextValueSuffix(" %");
    sectionBroncoKnob.setDoubleClickReturnValue(true, 0.0);
    sectionBroncoKnob.setTextBoxStyle(
        juce::Slider::TextBoxBelow, false, 80, 24);
    sectionBroncoKnob.setColour(juce::Slider::rotarySliderFillColourId,
                               MiguelColours::yellow());

    static const juce::StringArray frequencyNames{
        "60 Hz", "120 Hz", "250 Hz", "500 Hz",
        "1 kHz", "4 kHz", "10 kHz"
    };
    for (int band = 0; band < SectionEq::bandCount; ++band)
    {
        auto& input = inputEqSliders[static_cast<size_t>(band)];
        auto& output = outputEqSliders[static_cast<size_t>(band)];
        auto& inputLabel = inputEqBandLabels[static_cast<size_t>(band)];
        auto& outputLabel = outputEqBandLabels[static_cast<size_t>(band)];
        for (auto* component : {
                 static_cast<juce::Component*>(&input),
                 static_cast<juce::Component*>(&output),
                 static_cast<juce::Component*>(&inputLabel),
                 static_cast<juce::Component*>(&outputLabel) })
            eqPage.addAndMakeVisible(component);

        configureSlider(input, -18.0, 18.0, 0.1, 0.0, " dB");
        configureSlider(output, -18.0, 18.0, 0.1, 0.0, " dB");
        input.setSliderStyle(juce::Slider::LinearVertical);
        output.setSliderStyle(juce::Slider::LinearVertical);
        input.setTextBoxStyle(
            juce::Slider::TextBoxBelow, false, 64, 22);
        output.setTextBoxStyle(
            juce::Slider::TextBoxBelow, false, 64, 22);
        configureLabel(inputLabel, frequencyNames[band]);
        configureLabel(outputLabel, frequencyNames[band]);
        inputLabel.setJustificationType(juce::Justification::centred);
        outputLabel.setJustificationType(juce::Justification::centred);
        input.onValueChange = [this, band]
        {
            auto& eq = processor.getSectionEqBank().get(selectedEqSection());
            eq.setBandGain(false, band, static_cast<float>(
                inputEqSliders[static_cast<size_t>(band)].getValue()));
            updateEqControls();
        };
        output.onValueChange = [this, band]
        {
            auto& eq = processor.getSectionEqBank().get(selectedEqSection());
            eq.setBandGain(true, band, static_cast<float>(
                outputEqSliders[static_cast<size_t>(band)].getValue()));
            updateEqControls();
        };
    }
    graphicEqDisplay.setGainChangedCallback(
        [this](bool outputStage, int band, float gain)
        {
            processor.getSectionEqBank().get(selectedEqSection())
                .setBandGain(outputStage, band, gain);
            auto& slider = outputStage
                ? outputEqSliders[static_cast<size_t>(band)]
                : inputEqSliders[static_cast<size_t>(band)];
            slider.setValue(gain, juce::dontSendNotification);
        });
    eqSectionBox.onChange = [this] { updateEqControls(); };
    sectionVolumeKnob.onValueChange = [this]
    {
        processor.getSectionEqBank().get(selectedEqSection())
            .setVolume(static_cast<float>(sectionVolumeKnob.getValue()));
    };
    sectionBroncoKnob.onValueChange = [this]
    {
        processor.getSectionEqBank().get(selectedEqSection())
            .setBroncoMax(static_cast<float>(
                sectionBroncoKnob.getValue() / 100.0));
    };
    resetEqButton.onClick = [this]
    {
        auto& eq = processor.getSectionEqBank().get(selectedEqSection());
        for (int band = 0; band < SectionEq::bandCount; ++band)
        {
            eq.setBandGain(false, band, 0.0f);
            eq.setBandGain(true, band, 0.0f);
        }
        eq.setVolume(1.0f);
        eq.setBroncoMax(0.0f);
        updateEqControls();
    };
    inputEqFoldButton.onClick = [this]
    {
        inputEqExpanded = !inputEqExpanded;
        graphicEqDisplay.setActiveStage(false);
        inputEqFoldButton.setButtonText(
            foldButtonText(inputEqExpanded, "EQ de entrada"));
        updateFoldVisibility();
        resized();
    };
    outputEqFoldButton.onClick = [this]
    {
        outputEqExpanded = !outputEqExpanded;
        graphicEqDisplay.setActiveStage(true);
        outputEqFoldButton.setButtonText(
            foldButtonText(outputEqExpanded, "EQ de salida"));
        updateFoldVisibility();
        resized();
    };
    updateEqControls();
    updateFoldVisibility();

    if (const auto ui = processor.getUiSessionState(); ui.isValid()
        && ui.getNumChildren() > 0)
        restoreUiSessionState(ui);
    else if (processor.loadAutosaveSession())
        restoreUiSessionState(processor.getUiSessionState());

    startTimerHz(10);
}

MiguelMusicAssistantAudioProcessorEditor::
~MiguelMusicAssistantAudioProcessorEditor()
{
    processor.setUiSessionState(captureUiSessionState());
    processor.saveAutosaveSession();
    setLookAndFeel(nullptr);
    stopTimer();
    ++pitchRequest;
    pitchPool.removeAllJobs(true, 3000);
    ++chordRequest;
    chordProcess.kill();
    chordPool.removeAllJobs(true, 3000);
    pianoGenerationProcess.kill();
    studioPool.removeAllJobs(true, 3000);
    pianoKeyboardState.removeListener(this);
    processor.stopPreviews();
}

void MiguelMusicAssistantAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient background(
        MiguelColours::panel(), bounds.getCentreX(), bounds.getY(),
        MiguelColours::background(), bounds.getCentreX(), bounds.getBottom(),
        false);
    graphics.setGradientFill(background);
    graphics.fillAll();
}

void MiguelMusicAssistantAudioProcessorEditor::resized()
{
    tabs.setBounds(getLocalBounds().reduced(8));

    auto generator = generatorPage.getLocalBounds().reduced(24);
    generatorTitle.setBounds(generator.removeFromTop(44));
    generator.removeFromTop(8);
    auto generatorRow = generator.removeFromTop(40);
    juce::FlexBox firstGeneratorRow;
    firstGeneratorRow.flexDirection = juce::FlexBox::Direction::row;
    firstGeneratorRow.alignItems = juce::FlexBox::AlignItems::center;
    firstGeneratorRow.items = {
        juce::FlexItem(keyLabel).withWidth(105.0f),
        juce::FlexItem(keyBox).withFlex(1.0f).withMargin(3.0f),
        juce::FlexItem(modeLabel).withWidth(105.0f),
        juce::FlexItem(modeBox).withFlex(1.0f).withMargin(3.0f)
    };
    firstGeneratorRow.performLayout(generatorRow.toFloat());
    generator.removeFromTop(6);

    generatorRow = generator.removeFromTop(40);
    juce::FlexBox secondGeneratorRow;
    secondGeneratorRow.flexDirection = juce::FlexBox::Direction::row;
    secondGeneratorRow.alignItems = juce::FlexBox::AlignItems::center;
    secondGeneratorRow.items = {
        juce::FlexItem(barsLabel).withWidth(105.0f),
        juce::FlexItem(barsBox).withFlex(0.7f).withMargin(3.0f),
        juce::FlexItem(bpmLabel).withWidth(105.0f),
        juce::FlexItem(bpmSlider).withFlex(1.6f).withMargin(3.0f)
    };
    secondGeneratorRow.performLayout(generatorRow.toFloat());
    generator.removeFromTop(6);

    generatorRow = generator.removeFromTop(40);
    juce::FlexBox thirdGeneratorRow;
    thirdGeneratorRow.flexDirection = juce::FlexBox::Direction::row;
    thirdGeneratorRow.alignItems = juce::FlexBox::AlignItems::center;
    thirdGeneratorRow.items = {
        juce::FlexItem(humanizeLabel).withWidth(105.0f),
        juce::FlexItem(humanizeSlider).withFlex(1.0f).withMargin(3.0f)
    };
    thirdGeneratorRow.performLayout(generatorRow.toFloat());
    generator.removeFromTop(12);

    generatorRow = generator.removeFromTop(42);
    juce::FlexBox generatorActions;
    generatorActions.flexDirection = juce::FlexBox::Direction::row;
    generatorActions.items = {
        juce::FlexItem(previewMidiButton).withFlex(1.0f).withMargin(3.0f),
        juce::FlexItem(stopPreviewButton).withFlex(1.0f).withMargin(3.0f),
        juce::FlexItem(exportButton).withFlex(1.2f).withMargin(3.0f),
        juce::FlexItem(dragButton).withFlex(1.35f).withMargin(3.0f)
    };
    generatorActions.performLayout(generatorRow.toFloat());
    generatorStatus.setBounds(generator.removeFromTop(42));
    generator.removeFromTop(8);
    generatorPianoRoll.setBounds(generator);

    auto mix = mixPage.getLocalBounds().reduced(28);
    levelTitle.setBounds(mix.removeFromTop(44));
    levelReadout.setBounds(mix.removeFromTop(38));
    mix.removeFromTop(8);
    auto suggestionArea = mix.removeFromBottom(
        juce::jlimit(72, 120, mix.getHeight() / 4));
    mix.removeFromBottom(8);
    mixAnalyzer.setBounds(mix);
    mixSuggestion.setBounds(suggestionArea);

    auto library = libraryPage.getLocalBounds().reduced(16);
    libraryTitle.setBounds(library.removeFromTop(40));
    library.removeFromTop(6);
    auto libraryTop = library.removeFromTop(42);
    folderButton.setBounds(libraryTop.removeFromLeft(170).reduced(3));
    removeSampleButton.setBounds(libraryTop.removeFromLeft(170).reduced(3));
    folderLabel.setBounds(libraryTop.reduced(8, 0));
    library.removeFromTop(8);
    sampleWaveform.setBounds(library.removeFromTop(105));
    library.removeFromTop(6);
    auto tunerArea = library.removeFromTop(165);
    tunerTitle.setBounds(tunerArea.removeFromTop(27));
    auto tunerContent = tunerArea;
    tunerNeedle.setBounds(tunerContent.removeFromLeft(300).reduced(3));
    auto knobArea = tunerContent.removeFromLeft(125);
    pitchShiftLabel.setBounds(knobArea.removeFromTop(24));
    pitchShiftKnob.setBounds(knobArea.reduced(6, 0));
    auto broncoArea = tunerContent.removeFromLeft(125);
    broncoMaxLabel.setBounds(broncoArea.removeFromTop(24));
    broncoMaxKnob.setBounds(broncoArea.reduced(6, 0));
    tunerReadout.setBounds(tunerContent.removeFromTop(38));
    analysePitchButton.setBounds(
        tunerContent.removeFromTop(32).removeFromLeft(130).reduced(3));
    auditionTunedButton.setBounds(
        tunerContent.removeFromTop(34).removeFromLeft(155).reduced(3));
    exportTunedButton.setBounds(
        tunerContent.removeFromTop(34).removeFromLeft(145).reduced(3));
    library.removeFromTop(6);
    auto sampleControls = library.removeFromTop(38);
    samplePlayButton.setBounds(sampleControls.removeFromLeft(120).reduced(3));
    sampleStopButton.setBounds(sampleControls.removeFromLeft(110).reduced(3));
    sampleDragButton.setBounds(sampleControls.removeFromLeft(180).reduced(3));
    sampleInfo.setBounds(sampleControls.reduced(8, 0));
    library.removeFromTop(6);
    importedSampleList.setBounds(library);

    auto bajoquinto = bajoquintoPage.getLocalBounds().reduced(28);
    bajoquintoTitle.setBounds(bajoquinto.removeFromTop(42));
    bajoquintoDescription.setBounds(bajoquinto.removeFromTop(30));
    bajoquinto.removeFromTop(12);

    const auto useWideBajoquinto = bajoquinto.getWidth() > 920;
    auto controlsArea = useWideBajoquinto
        ? bajoquinto.removeFromLeft(bajoquinto.getWidth() / 2 - 10)
        : bajoquinto;
    auto statusArea = bajoquinto;

    auto row = controlsArea.removeFromTop(42);
    bajoquintoStyleLabel.setBounds(row.removeFromLeft(75));
    bajoquintoStyleBox.setBounds(row.removeFromLeft(160).reduced(3));
    chordRootLabel.setBounds(row.removeFromLeft(75));
    chordRootBox.setBounds(row.removeFromLeft(140).reduced(3));
    chordQualityLabel.setBounds(row.removeFromLeft(60));
    chordQualityBox.setBounds(row.removeFromLeft(140).reduced(3));
    controlsArea.removeFromTop(8);

    row = controlsArea.removeFromTop(42);
    chordVoicingLabel.setBounds(row.removeFromLeft(75));
    chordVoicingBox.setBounds(row.removeFromLeft(250).reduced(3));
    controlsArea.removeFromTop(12);

    row = controlsArea.removeFromTop(48);
    juce::FlexBox bajoquintoActions;
    bajoquintoActions.flexDirection = juce::FlexBox::Direction::row;
    bajoquintoActions.items = {
        juce::FlexItem(generateChordsButton).withFlex(1.0f).withMargin(3.0f),
        juce::FlexItem(previewChordButton).withFlex(1.0f).withMargin(3.0f),
        juce::FlexItem(stopChordButton).withFlex(0.7f).withMargin(3.0f),
        juce::FlexItem(openToneInputButton).withFlex(1.1f).withMargin(3.0f),
        juce::FlexItem(openChordsButton).withFlex(0.9f).withMargin(3.0f)
    };
    bajoquintoActions.performLayout(row.toFloat());
    controlsArea.removeFromTop(8);
    toneInputLabel.setBounds(controlsArea.removeFromTop(30));

    if (!useWideBajoquinto)
        statusArea = bajoquinto;
    bajoquintoStatus.setBounds(statusArea.reduced(2));

    auto studio = studioPage.getLocalBounds().reduced(16);
    studioTitle.setBounds(studio.removeFromTop(34));
    rhythmFoldButton.setBounds(studio.removeFromTop(30));
    if (rhythmExpanded)
    {
        auto studioControls = studio.removeFromTop(38);
        rhythmBpmLabel.setBounds(studioControls.removeFromLeft(50));
        rhythmBpmSlider.setBounds(
            studioControls.removeFromLeft(190).reduced(2));
        loopLengthLabel.setBounds(studioControls.removeFromLeft(42));
        loopLengthBox.setBounds(
            studioControls.removeFromLeft(110).reduced(2));
        exportBarsLabel.setBounds(studioControls.removeFromLeft(65));
        exportBarsBox.setBounds(
            studioControls.removeFromLeft(125).reduced(2));

        auto libraryControls = studio.removeFromTop(40);
        drumLibraryLabel.setBounds(libraryControls.removeFromLeft(75));
        drumLibraryBox.setBounds(
            libraryControls.removeFromLeft(245).reduced(2));
        openDrumLibraryButton.setBounds(
            libraryControls.removeFromLeft(145).reduced(2));
        libraryControls.removeFromLeft(12);
        rhythmPlayButton.setBounds(
            libraryControls.removeFromLeft(100).reduced(2));
        rhythmStopButton.setBounds(
            libraryControls.removeFromLeft(80).reduced(2));
        rhythmClearButton.setBounds(
            libraryControls.removeFromLeft(85).reduced(2));
        rhythmExportButton.setBounds(
            libraryControls.removeFromLeft(130).reduced(2));
        studio.removeFromTop(5);

        const auto rowHeight = 31;
        const auto gridHeight = rowHeight * GrooveEngine::channelCount;
        auto grid = studio.removeFromTop(gridHeight);
        const auto gridOrigin = grid;
        constexpr int channelLabelWidth = 24;
        constexpr int loadButtonWidth = 126;
        constexpr int gainWidth = 88;
        constexpr int eqButtonWidth = 38;
        constexpr int cellWidth = 19;
        const auto controlsWidth = channelLabelWidth + loadButtonWidth
            + gainWidth + eqButtonWidth;
        const auto activeSteps =
            processor.getGrooveEngine().getLoopLength();
        rhythmGridViewport.setBounds(
            gridOrigin.withTrimmedLeft(controlsWidth));
        rhythmGridContent.setSize(activeSteps * cellWidth, gridHeight);
        for (int channel = 0; channel < GrooveEngine::channelCount; ++channel)
        {
            auto channelRow = grid.removeFromTop(rowHeight);
            rhythmChannelLabels[static_cast<size_t>(channel)].setBounds(
                channelRow.removeFromLeft(channelLabelWidth));
            rhythmLoadButtons[static_cast<size_t>(channel)].setBounds(
                channelRow.removeFromLeft(loadButtonWidth).reduced(2));
            rhythmGainSliders[static_cast<size_t>(channel)].setBounds(
                channelRow.removeFromLeft(gainWidth).reduced(1));
            rhythmEqButtons[static_cast<size_t>(channel)].setBounds(
                channelRow.removeFromLeft(eqButtonWidth).reduced(2));
            for (int step = 0; step < GrooveEngine::stepCount; ++step)
            {
                auto& cell = rhythmSteps[static_cast<size_t>(channel)]
                                        [static_cast<size_t>(step)];
                cell.setVisible(step < activeSteps && rhythmExpanded);
                cell.setBounds(step * cellWidth, channel * rowHeight,
                               cellWidth, rowHeight);
            }
        }
    }

    studio.removeFromTop(8);
    pianoFoldButton.setBounds(studio.removeFromTop(30));
    if (pianoExpanded)
    {
        auto pianoControls = studio.removeFromTop(38);
        preparePianoButton.setBounds(
            pianoControls.removeFromLeft(185).reduced(2));
        pianoRecordButton.setBounds(
            pianoControls.removeFromLeft(90).reduced(2));
        pianoStopButton.setBounds(
            pianoControls.removeFromLeft(90).reduced(2));
        pianoPlaybackButton.setBounds(
            pianoControls.removeFromLeft(180).reduced(2));
        pianoExportButton.setBounds(
            pianoControls.removeFromLeft(165).reduced(2));
        pianoStatus.setBounds(pianoControls.reduced(8, 0));
        studio.removeFromTop(4);
        pianoKeyboard.setBounds(studio.removeFromTop(
            juce::jmin(115, studio.getHeight())));
    }

    auto eqBounds = eqPage.getLocalBounds().reduced(28);
    eqTitle.setBounds(eqBounds.removeFromTop(44));
    auto eqSelector = eqBounds.removeFromTop(42);
    eqSectionLabel.setBounds(eqSelector.removeFromLeft(130));
    eqSectionBox.setBounds(eqSelector.removeFromLeft(240).reduced(3));
    resetEqButton.setBounds(eqSelector.removeFromRight(150).reduced(3));
    eqBounds.removeFromTop(10);

    auto volumeArea = eqBounds.removeFromRight(210);
    sectionVolumeLabel.setBounds(volumeArea.removeFromTop(30));
    sectionVolumeKnob.setBounds(volumeArea.removeFromTop(145).reduced(12));
    volumeArea.removeFromTop(12);
    sectionBroncoLabel.setBounds(volumeArea.removeFromTop(30));
    sectionBroncoKnob.setBounds(
        volumeArea.removeFromTop(145).reduced(12));

    const auto graphHeight = juce::jlimit(
        180, 260, static_cast<int>(eqBounds.getHeight() * 0.43f));
    graphicEqDisplay.setBounds(eqBounds.removeFromTop(graphHeight).reduced(2));
    eqBounds.removeFromTop(8);

    inputEqFoldButton.setBounds(eqBounds.removeFromTop(30));
    if (inputEqExpanded)
    {
        const auto reservedForOutput = 42 + (outputEqExpanded ? 92 : 0);
        auto inputBands = eqBounds.removeFromTop(juce::jmin(
            130, juce::jmax(76, eqBounds.getHeight() - reservedForOutput)));
        const auto inputBandWidth =
            inputBands.getWidth() / SectionEq::bandCount;
        for (int band = 0; band < SectionEq::bandCount; ++band)
        {
            auto bandArea = band == SectionEq::bandCount - 1
                ? inputBands
                : inputBands.removeFromLeft(inputBandWidth);
            inputEqBandLabels[static_cast<size_t>(band)].setBounds(
                bandArea.removeFromTop(25));
            inputEqSliders[static_cast<size_t>(band)].setBounds(
                bandArea.reduced(8, 0));
        }
    }

    eqBounds.removeFromTop(12);
    outputEqFoldButton.setBounds(eqBounds.removeFromTop(30));
    if (outputEqExpanded)
    {
        auto outputBands = eqBounds.removeFromTop(
            juce::jmin(130, eqBounds.getHeight()));
        const auto outputBandWidth =
            outputBands.getWidth() / SectionEq::bandCount;
        for (int band = 0; band < SectionEq::bandCount; ++band)
        {
            auto bandArea = band == SectionEq::bandCount - 1
                ? outputBands
                : outputBands.removeFromLeft(outputBandWidth);
            outputEqBandLabels[static_cast<size_t>(band)].setBounds(
                bandArea.removeFromTop(25));
            outputEqSliders[static_cast<size_t>(band)].setBounds(
                bandArea.reduced(8, 0));
        }
    }
}

void MiguelMusicAssistantAudioProcessorEditor::updateSelectedSample(
    const juce::File& file)
{
    if (!file.existsAsFile())
        return;

    selectedSample = file;
    hasDetectedPitch = false;
    detectedFrequency = 0.0;
    detectedMidiExact = 0.0;
    pitchShiftKnob.setValue(0.0, juce::dontSendNotification);
    tunerNeedle.clear();
    sampleWaveform.setFile(file);
    const auto sizeMb = static_cast<double>(file.getSize()) / (1024.0 * 1024.0);
    sampleInfo.setText(file.getFileName() + "  /  "
        + juce::String(sizeMb, 1) + " MB",
        juce::dontSendNotification);
    if (!processor.loadSample(file))
        sampleInfo.setText("No se pudo abrir " + file.getFileName(),
                           juce::dontSendNotification);
    analyseSelectedSamplePitch();
}

void MiguelMusicAssistantAudioProcessorEditor::analyseSelectedSamplePitch()
{
    if (!selectedSample.existsAsFile())
        return;

    const auto request = ++pitchRequest;
    const auto file = selectedSample;
    hasDetectedPitch = false;
    tunerNeedle.clear();
    tunerReadout.setText("Tono: analizando...",
                         juce::dontSendNotification);
    juce::Component::SafePointer<MiguelMusicAssistantAudioProcessorEditor>
        safeThis(this);

    pitchPool.addJob([safeThis, file, request]
    {
        const auto result = detectPitch(file);
        juce::MessageManager::callAsync([safeThis, result, request]
        {
            if (safeThis == nullptr
                || request != safeThis->pitchRequest.load())
                return;

            if (result.detected)
            {
                safeThis->hasDetectedPitch = true;
                safeThis->detectedFrequency = result.frequency;
                safeThis->detectedMidiExact = result.midiExact;
                safeThis->updateTunerDisplay();
            }
            else
            {
                safeThis->hasDetectedPitch = false;
                safeThis->tunerReadout.setText(
                    "Tono no estable: probablemente es percusivo o contiene varias notas.",
                    juce::dontSendNotification);
                safeThis->tunerNeedle.clear();
            }
        });
    });
}

void MiguelMusicAssistantAudioProcessorEditor::updateTunerDisplay()
{
    if (!hasDetectedPitch)
    {
        tunerNeedle.clear();
        return;
    }

    const auto shiftedMidi = detectedMidiExact + pitchShiftKnob.getValue();
    const auto nearest = static_cast<int>(std::round(shiftedMidi));
    const auto cents = (shiftedMidi - nearest) * 100.0;
    const auto frequency = detectedFrequency
        * std::pow(2.0, pitchShiftKnob.getValue() / 12.0);
    const auto pitchClass = (nearest % 12 + 12) % 12;
    const auto note = MusicGenerator::noteName(pitchClass)
        + juce::String(nearest / 12 - 1);
    const auto direction = cents > 1.0 ? "agudo"
        : cents < -1.0 ? "grave" : "afinado";

    tunerNeedle.setReading(cents, note, frequency);
    tunerReadout.setText(
        note + " / " + juce::String(frequency, 2) + " Hz / "
            + juce::String(cents, 1) + " cents (" + direction + ")\n"
            + "Bronco Max: "
            + juce::String(static_cast<int>(broncoMaxKnob.getValue())) + "%",
        juce::dontSendNotification);
}

void MiguelMusicAssistantAudioProcessorEditor::processTunedSample(
    bool exportFile)
{
    if (!selectedSample.existsAsFile())
    {
        tunerReadout.setText("Selecciona primero un sample.",
                             juce::dontSendNotification);
        return;
    }

    const auto source = selectedSample;
    const auto semitones = pitchShiftKnob.getValue();
    const auto broncoAmount = broncoMaxKnob.getValue();
    juce::File destination;
    if (exportFile)
    {
        auto folder = juce::File::getSpecialLocation(
            juce::File::userDocumentsDirectory)
            .getChildFile("Miguel Music Assistant Exports")
            .getChildFile("Samples Afinados");
        if (!folder.createDirectory())
        {
            tunerReadout.setText("No se pudo crear la carpeta de exportación.",
                                 juce::dontSendNotification);
            return;
        }
        auto shiftName = juce::String(semitones, 2)
            .replaceCharacter('.', '_')
            .replaceCharacter('-', 'm');
        destination = folder.getNonexistentChildFile(
            source.getFileNameWithoutExtension() + "_tono_" + shiftName
                + "st_BroncoMax"
                + juce::String(static_cast<int>(broncoAmount)),
            ".wav", false);
    }
    else
    {
        destination = juce::File::getSpecialLocation(
            juce::File::tempDirectory).getNonexistentChildFile(
                "MiguelMusicAssistant_Afinado", ".wav", false);
    }

    auditionTunedButton.setEnabled(false);
    exportTunedButton.setEnabled(false);
    tunerReadout.setText(
        exportFile ? "Exportando sample afinado..."
                   : "Preparando previsualización afinada...",
        juce::dontSendNotification);
    juce::Component::SafePointer<MiguelMusicAssistantAudioProcessorEditor>
        safeThis(this);

    pitchPool.addJob([safeThis, source, destination, semitones,
                      broncoAmount, exportFile]
    {
        const auto success = renderPitchShiftedFile(
            source, destination, semitones, broncoAmount);
        juce::MessageManager::callAsync(
            [safeThis, destination, success, exportFile]
        {
            if (safeThis == nullptr)
                return;
            safeThis->auditionTunedButton.setEnabled(true);
            safeThis->exportTunedButton.setEnabled(true);

            if (!success)
            {
                safeThis->tunerReadout.setText(
                    "No se pudo procesar el sample.",
                    juce::dontSendNotification);
                return;
            }

            if (exportFile)
            {
                safeThis->tunerReadout.setText(
                    "Exportado: " + destination.getFileName(),
                    juce::dontSendNotification);
                destination.revealToUser();
            }
            else if (safeThis->processor.loadSample(destination))
            {
                safeThis->processor.playSample();
                safeThis->updateTunerDisplay();
            }
        });
    });
}

juce::File
MiguelMusicAssistantAudioProcessorEditor::bajoquintoOutputFolder() const
{
    const auto project = juce::File(
        "C:\\Users\\MIGUEL\\OneDrive2\\Desktop\\creador-acordes-bajoquinto");
    return project.getChildFile(
        bajoquintoStyleBox.getSelectedItemIndex() == 1
            ? "acordes_bronco" : "acordes");
}

juce::File
MiguelMusicAssistantAudioProcessorEditor::selectedBajoquintoChord() const
{
    const auto root = chordRootBox.getText();
    const auto quality = chordQualityBox.getSelectedItemIndex() == 1
        ? "menor" : "mayor";
    const auto label = root + " " + quality;
    const auto folder = bajoquintoOutputFolder();

    static const juce::StringArray voicingKeys{
        "", "cerrada_media", "cerrada_aguda", "tercera_alta", "tonica_alta"
    };
    const auto voicing = chordVoicingBox.getSelectedItemIndex();
    if (voicing <= 0)
        return folder.getChildFile(label + ".wav");

    return folder.getChildFile("opciones")
        .getChildFile(label)
        .getChildFile(voicingKeys[voicing] + ".wav");
}

void MiguelMusicAssistantAudioProcessorEditor::updateBajoquintoStatus()
{
    const auto folder = bajoquintoOutputFolder();
    const auto inputFolder = juce::File(
        "C:\\Users\\MIGUEL\\OneDrive2\\Desktop\\tonos");
    const auto inputCount = inputFolder.isDirectory()
        ? inputFolder.findChildFiles(
            juce::File::findFiles, false, "*.wav").size()
        : 0;
    const auto count = folder.isDirectory()
        ? folder.findChildFiles(juce::File::findFiles, false, "*.wav").size()
        : 0;
    const auto selected = selectedBajoquintoChord();
    bajoquintoStatus.setText(
        "WAV monofónicos de entrada: " + juce::String(inputCount)
            + " / Acordes disponibles: " + juce::String(count) + "/24\n"
            + "Selección: " + chordRootBox.getText() + " "
            + chordQualityBox.getText() + " / "
            + chordVoicingBox.getText() + "\n"
            + (selected.existsAsFile()
                ? "Listo para escuchar: " + selected.getFileName()
                : "Esta variante todavía no existe. Pulsa \"Generar 24 acordes\"."),
        juce::dontSendNotification);
}

void MiguelMusicAssistantAudioProcessorEditor::previewBajoquintoChord()
{
    const auto chord = selectedBajoquintoChord();
    if (!chord.existsAsFile())
    {
        updateBajoquintoStatus();
        return;
    }
    if (processor.loadSample(chord, AudioSection::chords))
    {
        processor.playSample();
        bajoquintoStatus.setText(
            "Reproduciendo: " + chord.getFileName(),
            juce::dontSendNotification);
    }
}

void MiguelMusicAssistantAudioProcessorEditor::generateBajoquintoChords()
{
    const auto engine = juce::File(
        "C:\\Users\\MIGUEL\\OneDrive2\\Desktop\\creador-acordes-bajoquinto");
    const auto python = juce::File(
        "C:\\Users\\MIGUEL\\AppData\\Local\\Programs\\Python\\Python312\\python.exe");
    const auto tones = juce::File(
        "C:\\Users\\MIGUEL\\OneDrive2\\Desktop\\tonos");
    const auto examples = juce::File(
        "C:\\Users\\MIGUEL\\OneDrive2\\Desktop\\ejemplos");
    const auto output = bajoquintoOutputFolder();

    if (!python.existsAsFile() || !engine.getChildFile(
            "generar_acordes.py").existsAsFile()
        || !tones.isDirectory() || !examples.isDirectory())
    {
        bajoquintoStatus.setText(
            "Falta Python, el motor, la carpeta tonos o la carpeta ejemplos.",
            juce::dontSendNotification);
        return;
    }

    const auto style = bajoquintoStyleBox.getSelectedItemIndex() == 1
        ? "bronco" : "clean";
    const auto code =
        "import sys; from pathlib import Path; "
        "sys.path.insert(0, r'" + engine.getFullPathName() + "'); "
        "from generar_acordes import generate_all_chords; "
        "generate_all_chords(Path(r'" + tones.getFullPathName()
        + "'), Path(r'" + output.getFullPathName()
        + "'), Path(r'" + examples.getFullPathName()
        + "'), style='" + style + "'); print('GENERACION_COMPLETA')";

    const auto request = ++chordRequest;
    generateChordsButton.setEnabled(false);
    bajoquintoStatus.setText(
        "Generando acordes " + bajoquintoStyleBox.getText()
            + "… El proceso puede tardar varios minutos.",
        juce::dontSendNotification);

    juce::Component::SafePointer<MiguelMusicAssistantAudioProcessorEditor>
        safeThis(this);
    chordPool.addJob([safeThis, python, code, request]
    {
        juce::StringArray arguments;
        arguments.add(python.getFullPathName());
        arguments.add("-c");
        arguments.add(code);

        const auto started = safeThis != nullptr
            && safeThis->chordProcess.start(arguments);
        const auto processOutput = started && safeThis != nullptr
            ? safeThis->chordProcess.readAllProcessOutput()
            : juce::String("No se pudo iniciar el motor.");
        const auto exitCode = started && safeThis != nullptr
            ? safeThis->chordProcess.getExitCode() : 1u;

        juce::MessageManager::callAsync(
            [safeThis, processOutput, exitCode, request]
        {
            if (safeThis == nullptr
                || request != safeThis->chordRequest.load())
                return;

            safeThis->generateChordsButton.setEnabled(true);
            if (exitCode == 0
                && processOutput.contains("GENERACION_COMPLETA"))
            {
                safeThis->updateBajoquintoStatus();
                safeThis->bajoquintoStatus.setText(
                    "Generación terminada correctamente.\n"
                        + safeThis->bajoquintoStatus.getText(),
                    juce::dontSendNotification);
            }
            else
            {
                safeThis->bajoquintoStatus.setText(
                    "Error del motor:\n" + processOutput.substring(
                        juce::jmax(0, processOutput.length() - 500)),
                    juce::dontSendNotification);
            }
        });
    });
}

juce::File
MiguelMusicAssistantAudioProcessorEditor::selectedDrumLibrary() const
{
    switch (drumLibraryBox.getSelectedItemIndex())
    {
        case 0:
            return juce::File(
                "C:\\Users\\MIGUEL\\Downloads\\mis sonidos golpes samples "
                "ruidos redobles-20240722T121717Z-001-20260414T112528Z-3-001"
                "\\mis sonidos golpes samples ruidos redobles-20240722T121717Z-001"
                "\\mis sonidos golpes samples ruidos redobles");
        case 1:
            return juce::File(
                "C:\\Users\\MIGUEL\\Downloads\\mis sonidos golpes samples "
                "ruidos redobles-20240722T121717Z-001-20260414T112528Z-3-001"
                "\\mis sonidos golpes samples ruidos redobles-20240722T121717Z-001"
                "\\wav tambora tarolas");
        case 2:
            return juce::File(
                "C:\\Users\\MIGUEL\\OneDrive2\\Documentos\\Image-Line"
                "\\Downloads\\FL CLOUD Sounds");
        default:
            return juce::File("C:\\Users\\MIGUEL\\Downloads");
    }
}

void MiguelMusicAssistantAudioProcessorEditor::chooseRhythmSample(int channel)
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Carga un sample en la pista " + juce::String(channel + 1),
        selectedDrumLibrary(),
        "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::canSelectMultipleItems,
        [this, channel](const juce::FileChooser& chooser)
        {
            const auto files = chooser.getResults();
            importedSampleList.addFiles(files);
            for (int index = 0; index < files.size(); ++index)
            {
                const auto targetChannel = channel + index;
                if (targetChannel >= GrooveEngine::channelCount)
                    break;
                const auto& file = files.getReference(index);
                if (file.existsAsFile()
                    && processor.getGrooveEngine().loadSample(
                        targetChannel, file))
                    rhythmLoadButtons[static_cast<size_t>(targetChannel)]
                        .setButtonText(file.getFileNameWithoutExtension());
            }
        });
}

void MiguelMusicAssistantAudioProcessorEditor::exportRhythmLoop()
{
    static constexpr std::array<int, 4> bars{ 1, 2, 4, 8 };
    const auto selectedBars = bars[static_cast<size_t>(
        juce::jmax(0, exportBarsBox.getSelectedItemIndex()))];
    auto folder = juce::File::getSpecialLocation(
        juce::File::userDocumentsDirectory)
        .getChildFile("Miguel Music Assistant Exports")
        .getChildFile("Loops");
    folder.createDirectory();
    const auto destination = folder.getNonexistentChildFile(
        "Loop_" + juce::String(processor.getGrooveEngine().getBpm(), 0)
            + "BPM_" + juce::String(selectedBars) + "compases",
        ".wav", false);
    if (processor.getGrooveEngine().exportLoop(
            destination, selectedBars))
        destination.revealToUser();
}

void MiguelMusicAssistantAudioProcessorEditor::prepareBroncoPiano()
{
    const auto python = juce::File(
        "C:\\Users\\MIGUEL\\AppData\\Local\\Programs\\Python"
        "\\Python312\\python.exe");
    const auto engine = juce::File(
        "C:\\Users\\MIGUEL\\OneDrive2\\Desktop\\creador-acordes-bajoquinto");
    const auto tones = juce::File(
        "C:\\Users\\MIGUEL\\OneDrive2\\Desktop\\tonos");
    const auto output = juce::File(
        "C:\\Users\\MIGUEL\\Documents\\Miguel Music Assistant\\Piano Bronco");
    if (!python.existsAsFile() || !engine.isDirectory() || !tones.isDirectory())
    {
        pianoStatus.setText("No se encontró el motor o la carpeta de tonos.",
                            juce::dontSendNotification);
        return;
    }

    const auto code =
        "import sys; from pathlib import Path; "
        "sys.path.insert(0, r'" + engine.getFullPathName() + "'); "
        "from generar_acordes import generate_bronco_piano; "
        "n=generate_bronco_piano(Path(r'" + tones.getFullPathName()
        + "'), Path(r'" + output.getFullPathName()
        + "')); print(f'PIANO_GENERADO={n}')";
    preparePianoButton.setEnabled(false);
    pianoStatus.setText(
        "Creando notas Bronco desde los tonos monofónicos...",
        juce::dontSendNotification);
    juce::Component::SafePointer<MiguelMusicAssistantAudioProcessorEditor>
        safeThis(this);

    studioPool.addJob([safeThis, python, code, output]
    {
        juce::StringArray arguments;
        arguments.add(python.getFullPathName());
        arguments.add("-c");
        arguments.add(code);
        const auto started = safeThis != nullptr
            && safeThis->pianoGenerationProcess.start(arguments);
        const auto processOutput = started && safeThis != nullptr
            ? safeThis->pianoGenerationProcess.readAllProcessOutput()
            : juce::String("No se pudo iniciar Python.");
        const auto exitCode = started && safeThis != nullptr
            ? safeThis->pianoGenerationProcess.getExitCode() : 1u;

        juce::MessageManager::callAsync(
            [safeThis, output, processOutput, exitCode]
        {
            if (safeThis == nullptr)
                return;
            safeThis->preparePianoButton.setEnabled(true);
            if (exitCode == 0)
            {
                const auto count =
                    safeThis->processor.getPianoEngine().loadLibrary(output);
                safeThis->pianoStatus.setText(
                    "Piano Bronco listo: " + juce::String(count) + " notas.",
                    juce::dontSendNotification);
            }
            else
            {
                safeThis->pianoStatus.setText(
                    "Error: " + processOutput.substring(
                        juce::jmax(0, processOutput.length() - 300)),
                    juce::dontSendNotification);
            }
        });
    });
}

void MiguelMusicAssistantAudioProcessorEditor::exportPianoRecording()
{
    auto folder = juce::File::getSpecialLocation(
        juce::File::userDocumentsDirectory)
        .getChildFile("Miguel Music Assistant Exports")
        .getChildFile("Piano Bronco");
    folder.createDirectory();
    const auto destination = folder.getNonexistentChildFile(
        "Interpretacion_BajoSexto_Bronco", ".wav", false);
    if (processor.getPianoEngine().exportRecording(destination))
    {
        pianoStatus.setText("Grabación exportada.",
                            juce::dontSendNotification);
        destination.revealToUser();
    }
    else
    {
        pianoStatus.setText("No hay notas grabadas para exportar.",
                            juce::dontSendNotification);
    }
}

AudioSection
MiguelMusicAssistantAudioProcessorEditor::selectedEqSection() const
{
    return static_cast<AudioSection>(juce::jlimit(
        0, static_cast<int>(AudioSection::count) - 1,
        eqSectionBox.getSelectedItemIndex()));
}

void MiguelMusicAssistantAudioProcessorEditor::updateEqControls()
{
    auto& eq = processor.getSectionEqBank().get(selectedEqSection());
    std::array<float, SectionEq::bandCount> inputGains{};
    std::array<float, SectionEq::bandCount> outputGains{};
    for (int band = 0; band < SectionEq::bandCount; ++band)
    {
        inputGains[static_cast<size_t>(band)] =
            eq.getBandGain(false, band);
        outputGains[static_cast<size_t>(band)] =
            eq.getBandGain(true, band);
        inputEqSliders[static_cast<size_t>(band)].setValue(
            inputGains[static_cast<size_t>(band)],
            juce::dontSendNotification);
        outputEqSliders[static_cast<size_t>(band)].setValue(
            outputGains[static_cast<size_t>(band)],
            juce::dontSendNotification);
    }
    graphicEqDisplay.setGains(inputGains, outputGains);
    sectionVolumeKnob.setValue(
        eq.getVolume(), juce::dontSendNotification);
    sectionBroncoKnob.setValue(
        eq.getBroncoMax() * 100.0f, juce::dontSendNotification);
}

void MiguelMusicAssistantAudioProcessorEditor::updateFoldVisibility()
{
    rhythmTitle.setVisible(false);
    for (auto* component : {
             static_cast<juce::Component*>(&rhythmBpmSlider),
             static_cast<juce::Component*>(&rhythmBpmLabel),
             static_cast<juce::Component*>(&loopLengthBox),
             static_cast<juce::Component*>(&loopLengthLabel),
             static_cast<juce::Component*>(&exportBarsBox),
             static_cast<juce::Component*>(&exportBarsLabel),
             static_cast<juce::Component*>(&rhythmPlayButton),
             static_cast<juce::Component*>(&rhythmStopButton),
             static_cast<juce::Component*>(&rhythmClearButton),
             static_cast<juce::Component*>(&rhythmExportButton),
             static_cast<juce::Component*>(&drumLibraryBox),
             static_cast<juce::Component*>(&drumLibraryLabel),
             static_cast<juce::Component*>(&openDrumLibraryButton),
             static_cast<juce::Component*>(&rhythmGridViewport) })
        component->setVisible(rhythmExpanded);
    for (int channel = 0; channel < GrooveEngine::channelCount; ++channel)
    {
        rhythmLoadButtons[static_cast<size_t>(channel)]
            .setVisible(rhythmExpanded);
        rhythmChannelLabels[static_cast<size_t>(channel)]
            .setVisible(rhythmExpanded);
        rhythmGainSliders[static_cast<size_t>(channel)]
            .setVisible(rhythmExpanded);
        rhythmEqButtons[static_cast<size_t>(channel)]
            .setVisible(rhythmExpanded);
        for (auto& cell : rhythmSteps[static_cast<size_t>(channel)])
            cell.setVisible(rhythmExpanded);
    }

    pianoTitle.setVisible(false);
    for (auto* component : {
             static_cast<juce::Component*>(&pianoKeyboard),
             static_cast<juce::Component*>(&preparePianoButton),
             static_cast<juce::Component*>(&pianoRecordButton),
             static_cast<juce::Component*>(&pianoStopButton),
             static_cast<juce::Component*>(&pianoPlaybackButton),
             static_cast<juce::Component*>(&pianoExportButton),
             static_cast<juce::Component*>(&pianoStatus) })
        component->setVisible(pianoExpanded);

    inputEqLabel.setVisible(false);
    outputEqLabel.setVisible(false);
    for (int band = 0; band < SectionEq::bandCount; ++band)
    {
        inputEqSliders[static_cast<size_t>(band)]
            .setVisible(inputEqExpanded);
        inputEqBandLabels[static_cast<size_t>(band)]
            .setVisible(inputEqExpanded);
        outputEqSliders[static_cast<size_t>(band)]
            .setVisible(outputEqExpanded);
        outputEqBandLabels[static_cast<size_t>(band)]
            .setVisible(outputEqExpanded);
    }
}

void MiguelMusicAssistantAudioProcessorEditor::handleNoteOn(
    juce::MidiKeyboardState*, int, int midiNote, float velocity)
{
    processor.getPianoEngine().noteOn(midiNote, velocity);
}

void MiguelMusicAssistantAudioProcessorEditor::handleNoteOff(
    juce::MidiKeyboardState*, int, int, float)
{
}

GenerationSettings
MiguelMusicAssistantAudioProcessorEditor::currentSettings() const
{
    GenerationSettings settings;
    settings.rootPitchClass = keyBox.getSelectedItemIndex();
    settings.minor = modeBox.getSelectedItemIndex() == 1;
    settings.bars = barsBox.getText().getIntValue();
    settings.bpm = bpmSlider.getValue();
    settings.humanizePercent = static_cast<int>(humanizeSlider.getValue());
    return settings;
}

void MiguelMusicAssistantAudioProcessorEditor::exportMidi()
{
    auto exportFolder = juce::File::getSpecialLocation(
        juce::File::userDocumentsDirectory)
        .getChildFile("Miguel Music Assistant Exports");
    if (!exportFolder.createDirectory())
    {
        generatorStatus.setText("No se pudo crear la carpeta de exportación.",
                                juce::dontSendNotification);
        return;
    }

    const auto settings = currentSettings();
    const auto scaleName = settings.minor ? "Menor" : "Mayor";
    const auto baseName = "Secuencia_"
        + MusicGenerator::noteName(settings.rootPitchClass) + "_"
        + scaleName + "_" + juce::String(settings.bpm, 0) + "BPM";
    const auto destination = exportFolder.getNonexistentChildFile(
        baseName, ".mid", false);

    if (MusicGenerator::writeToFile(destination, settings))
    {
        generatorStatus.setText("Exportado: " + destination.getFullPathName(),
                                juce::dontSendNotification);
        destination.revealToUser();
    }
    else
    {
        generatorStatus.setText("No se pudo escribir el archivo MIDI.",
                                juce::dontSendNotification);
    }
}

void MiguelMusicAssistantAudioProcessorEditor::chooseSampleFiles()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Selecciona uno o varios samples",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.mp3;*.aif;*.aiff;*.flac;*.ogg");
    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::canSelectMultipleItems,
        [this](const juce::FileChooser& chooser)
        {
            importedSampleList.addFiles(chooser.getResults());
        });
}

void MiguelMusicAssistantAudioProcessorEditor::timerCallback()
{
    generatorPianoRoll.setPlayheadProgress(
        processor.getMidiPreviewProgress());
    std::array<float, 4096> analyzerSamples{};
    const auto analyzerCount = processor.popAnalyzerSamples(
        analyzerSamples.data(), static_cast<int>(analyzerSamples.size()));
    mixAnalyzer.pushAudio(analyzerSamples.data(), analyzerCount);
    mixAnalyzer.setLevels(
        processor.getLeftRmsDb(), processor.getRightRmsDb(),
        processor.getLeftPeakDb(), processor.getRightPeakDb(),
        processor.getStereoCorrelation());
    const auto rms = processor.getRmsDb();
    const auto peak = processor.getPeakDb();
    const auto correlation = processor.getStereoCorrelation();
    levelReadout.setText(
        "RMS: " + juce::String(rms, 1) + " dBFS     Pico: "
            + juce::String(peak, 1) + " dBFS     Correlación: "
            + juce::String(correlation, 2),
        juce::dontSendNotification);

    juce::StringArray suggestions;
    if (processor.hasClipped() || peak > -0.3f)
        suggestions.add("- Baja la ganancia: se detectaron picos muy cercanos a 0 dBFS.");
    if (rms > -8.0f)
        suggestions.add("- La señal tiene un nivel RMS alto; revisa limitación o compresión.");
    if (peak - rms > 18.0f)
        suggestions.add("- Hay mucha diferencia entre pico y RMS; una compresión suave podría ayudar.");
    if (correlation < 0.0f)
        suggestions.add("- Posible problema de fase: verifica la mezcla en mono.");
    if (suggestions.isEmpty())
        suggestions.add("- Los niveles básicos se encuentran en un rango razonable.");
    mixSuggestion.setText(suggestions.joinIntoString("\n"),
                          juce::dontSendNotification);

    if (!processor.isMidiPreviewPlaying()
        && generatorStatus.getText().startsWith("Reproduciendo"))
        generatorStatus.setText("Previsualización terminada.",
                                juce::dontSendNotification);
    samplePlayButton.setButtonText(
        processor.isSamplePlaying() ? "Reproduciendo..." : "Escuchar");

    const auto playhead = processor.getGrooveEngine().getCurrentStep();
    if (playhead != previousPlayheadStep)
    {
        if (previousPlayheadStep >= 0)
            for (int channel = 0; channel < GrooveEngine::channelCount;
                 ++channel)
            {
                auto& oldCell =
                    rhythmSteps[static_cast<size_t>(channel)]
                               [static_cast<size_t>(previousPlayheadStep)];
                oldCell.setColour(
                    juce::TextButton::buttonColourId,
                    previousPlayheadStep % 8 < 4
                        ? juce::Colour(0xff171044)
                        : juce::Colour(0xff25206b));
                oldCell.setColour(
                    juce::TextButton::buttonOnColourId,
                    juce::Colour(0xff7c3aed));
            }
        if (playhead >= 0)
            for (int channel = 0; channel < GrooveEngine::channelCount;
                 ++channel)
            {
                auto& cell = rhythmSteps[static_cast<size_t>(channel)]
                                        [static_cast<size_t>(playhead)];
                cell.setColour(juce::TextButton::buttonColourId,
                               juce::Colour(0xffffe066));
                cell.setColour(juce::TextButton::buttonOnColourId,
                               juce::Colour(0xffffb000));
            }
        previousPlayheadStep = playhead;
    }
}

void MiguelMusicAssistantAudioProcessorEditor::syncSessionStateToProcessor()
{
    processor.setUiSessionState(captureUiSessionState());
}

void MiguelMusicAssistantAudioProcessorEditor::refreshRhythmGridFromEngine()
{
    auto& groove = processor.getGrooveEngine();
    const auto activeSteps = groove.getLoopLength();
    for (int channel = 0; channel < GrooveEngine::channelCount; ++channel)
    {
        rhythmLoadButtons[static_cast<size_t>(channel)].setButtonText(
            groove.getSampleName(channel));
        rhythmGainSliders[static_cast<size_t>(channel)].setValue(
            groove.getGain(channel), juce::dontSendNotification);
        for (int step = 0; step < GrooveEngine::stepCount; ++step)
        {
            auto& cell = rhythmSteps[static_cast<size_t>(channel)]
                                    [static_cast<size_t>(step)];
            cell.setToggleState(
                step < activeSteps && groove.getStep(channel, step),
                juce::dontSendNotification);
        }
    }
}

juce::ValueTree MiguelMusicAssistantAudioProcessorEditor::captureUiSessionState() const
{
    juce::ValueTree ui("UiSession");

    juce::ValueTree generator("Generator");
    generator.setProperty("keyId", keyBox.getSelectedId(), nullptr);
    generator.setProperty("modeId", modeBox.getSelectedId(), nullptr);
    generator.setProperty("bars", barsBox.getText(), nullptr);
    generator.setProperty("bpm", bpmSlider.getValue(), nullptr);
    generator.setProperty("humanize", humanizeSlider.getValue(), nullptr);
    ui.appendChild(generator, nullptr);

    juce::ValueTree imported("ImportedSamples");
    for (const auto& path : importedSampleList.getFilePaths())
    {
        juce::ValueTree sample("Sample");
        sample.setProperty("path", path, nullptr);
        imported.appendChild(sample, nullptr);
    }
    ui.appendChild(imported, nullptr);

    juce::ValueTree rhythm("Rhythm");
    rhythm.setProperty("bpm", rhythmBpmSlider.getValue(), nullptr);
    rhythm.setProperty("loopIndex", loopLengthBox.getSelectedItemIndex(), nullptr);
    rhythm.setProperty("exportBarsIndex", exportBarsBox.getSelectedItemIndex(),
                       nullptr);
    rhythm.setProperty("drumLibraryIndex", drumLibraryBox.getSelectedItemIndex(),
                       nullptr);
    ui.appendChild(rhythm, nullptr);

    juce::ValueTree bajoquinto("Bajoquinto");
    bajoquinto.setProperty("styleIndex",
                           bajoquintoStyleBox.getSelectedItemIndex(), nullptr);
    bajoquinto.setProperty("rootIndex", chordRootBox.getSelectedItemIndex(),
                           nullptr);
    bajoquinto.setProperty("qualityIndex",
                           chordQualityBox.getSelectedItemIndex(), nullptr);
    bajoquinto.setProperty("voicingIndex",
                           chordVoicingBox.getSelectedItemIndex(), nullptr);
    ui.appendChild(bajoquinto, nullptr);

    ui.setProperty("selectedSamplePath",
                   selectedSample.getFullPathName(), nullptr);
    ui.setProperty("activeTab", tabs.getCurrentTabIndex(), nullptr);
    return ui;
}

void MiguelMusicAssistantAudioProcessorEditor::restoreUiSessionState(
    const juce::ValueTree& uiState)
{
    if (!uiState.hasType("UiSession"))
        return;

    if (const auto generator = uiState.getChildWithName("Generator");
        generator.isValid())
    {
        keyBox.setSelectedId(
            static_cast<int>(generator.getProperty("keyId", 1)),
            juce::dontSendNotification);
        modeBox.setSelectedId(
            static_cast<int>(generator.getProperty("modeId", 1)),
            juce::dontSendNotification);
        barsBox.setText(
            generator.getProperty("bars", "4").toString(),
            juce::dontSendNotification);
        bpmSlider.setValue(
            static_cast<double>(generator.getProperty("bpm", 120.0)),
            juce::dontSendNotification);
        humanizeSlider.setValue(
            static_cast<double>(generator.getProperty("humanize", 8.0)),
            juce::dontSendNotification);
        generatorPianoRoll.setMidiFile(
            MusicGenerator::createSong(currentSettings()),
            static_cast<int>(barsBox.getText().getIntValue()));
    }

    if (const auto imported = uiState.getChildWithName("ImportedSamples");
        imported.isValid())
    {
        juce::StringArray paths;
        for (int i = 0; i < imported.getNumChildren(); ++i)
        {
            const auto sample = imported.getChild(i);
            if (sample.hasType("Sample"))
                paths.add(sample.getProperty("path", juce::String())
                              .toString());
        }
        importedSampleList.setFilesFromPaths(paths);
    }

    if (const auto rhythm = uiState.getChildWithName("Rhythm");
        rhythm.isValid())
    {
        rhythmBpmSlider.setValue(
            static_cast<double>(rhythm.getProperty("bpm", 120.0)),
            juce::dontSendNotification);
        processor.getGrooveEngine().setBpm(rhythmBpmSlider.getValue());

        const auto loopIndex = static_cast<int>(
            rhythm.getProperty("loopIndex", 1));
        loopLengthBox.setSelectedItemIndex(
            juce::jlimit(0, loopLengthBox.getNumItems() - 1, loopIndex),
            juce::dontSendNotification);
        static constexpr std::array<int, 5> resolutions{ 4, 8, 16, 32, 64 };
        processor.getGrooveEngine().setGridResolution(
            resolutions[static_cast<size_t>(
                juce::jlimit(0, 4, loopIndex))]);

        exportBarsBox.setSelectedItemIndex(
            static_cast<int>(rhythm.getProperty("exportBarsIndex", 2)),
            juce::dontSendNotification);
        drumLibraryBox.setSelectedItemIndex(
            static_cast<int>(rhythm.getProperty("drumLibraryIndex", 0)),
            juce::dontSendNotification);
    }

    if (const auto bajoquinto = uiState.getChildWithName("Bajoquinto");
        bajoquinto.isValid())
    {
        bajoquintoStyleBox.setSelectedItemIndex(
            static_cast<int>(bajoquinto.getProperty("styleIndex", 0)),
            juce::dontSendNotification);
        chordRootBox.setSelectedItemIndex(
            static_cast<int>(bajoquinto.getProperty("rootIndex", 0)),
            juce::dontSendNotification);
        chordQualityBox.setSelectedItemIndex(
            static_cast<int>(bajoquinto.getProperty("qualityIndex", 0)),
            juce::dontSendNotification);
        chordVoicingBox.setSelectedItemIndex(
            static_cast<int>(bajoquinto.getProperty("voicingIndex", 0)),
            juce::dontSendNotification);
    }

    const auto tabIndex = static_cast<int>(uiState.getProperty("activeTab", 0));
    tabs.setCurrentTabIndex(
        juce::jlimit(0, tabs.getNumTabs() - 1, tabIndex));

    refreshRhythmGridFromEngine();
    updateEqControls();
    updateBajoquintoStatus();
    resized();

    const auto samplePath = uiState.getProperty(
        "selectedSamplePath", juce::String()).toString();
    if (samplePath.isNotEmpty())
    {
        const juce::File file(samplePath);
        if (file.existsAsFile())
            updateSelectedSample(file);
    }
}
