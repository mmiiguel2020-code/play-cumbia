#include "PluginEditor.h"

#if JucePlugin_Build_Standalone
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

namespace
{
bool isAudioSampleFile(const juce::File& file)
{
    const auto ext = file.getFileExtension().toLowerCase();
    return ext == ".wav" || ext == ".aif" || ext == ".aiff"
        || ext == ".flac" || ext == ".mp3" || ext == ".ogg";
}

juce::Font pichaTitleFont(float height)
{
    const auto available = juce::Font::findAllTypefaceNames();
    const juce::StringArray preferred{
        "Sitka Banner", "Palatino Linotype", "Georgia", "Constantia"
    };
    for (const auto& face : preferred)
        if (available.contains(face))
            return juce::Font(juce::FontOptions(
                face, height, juce::Font::italic | juce::Font::bold));
    return juce::Font(juce::FontOptions(
        height, juce::Font::italic | juce::Font::bold));
}

void drawAccordionSilhouette(juce::Graphics& g, juce::Rectangle<float> area,
                             juce::Colour fill)
{
    auto r = area.reduced(1.5f);
    const auto x = r.getX();
    const auto y = r.getY();
    const auto w = r.getWidth();
    const auto h = r.getHeight();
    juce::Path body;
    body.addRoundedRectangle(x, y + h * 0.18f, w * 0.24f, h * 0.64f, 2.4f);
    const auto foldsX = x + w * 0.24f;
    const auto foldsW = w * 0.32f;
    const auto foldsY = y + h * 0.24f;
    const auto foldsH = h * 0.52f;
    for (int i = 0; i < 7; ++i)
    {
        const auto t = static_cast<float>(i) / 7.0f;
        const auto inset = (i % 2 == 0) ? 0.0f : foldsH * 0.09f;
        body.addRoundedRectangle(
            foldsX + foldsW * t, foldsY + inset,
            juce::jmax(2.0f, foldsW / 9.5f), foldsH - inset * 2.0f, 0.8f);
    }
    const auto kx = x + w * 0.56f;
    body.addRoundedRectangle(kx, y + h * 0.10f, w * 0.42f, h * 0.80f, 2.6f);
    g.setColour(fill);
    g.fillPath(body);
    g.setColour(MiguelColours::background().withAlpha(0.55f));
    const auto keyY = y + h * 0.16f;
    const auto keyH = h * 0.20f;
    const auto keyW = w * 0.34f;
    for (int i = 0; i < 8; ++i)
    {
        const auto kw = keyW / 8.0f;
        g.fillRect(kx + w * 0.04f + static_cast<float>(i) * kw,
                   keyY, kw * 0.68f, keyH);
    }
}

void drawBajoSextoSilhouette(juce::Graphics& g, juce::Rectangle<float> area,
                             juce::Colour fill)
{
    auto r = area.reduced(1.2f);
    const auto x = r.getX();
    const auto y = r.getY();
    const auto w = r.getWidth();
    const auto h = r.getHeight();
    juce::Path body;
    body.addEllipse(x, y + h * 0.34f, w * 0.54f, h * 0.62f);
    body.addEllipse(x + w * 0.06f, y + h * 0.08f, w * 0.42f, h * 0.48f);
    juce::Path neck;
    neck.addRoundedRectangle(x + w * 0.46f, y + h * 0.42f,
                             w * 0.28f, h * 0.14f, 1.2f);
    juce::Path head;
    const auto hx = x + w * 0.72f;
    const auto hy = y + h * 0.28f;
    head.startNewSubPath(hx, hy + h * 0.08f);
    head.lineTo(hx + w * 0.24f, hy);
    head.lineTo(hx + w * 0.24f, hy + h * 0.42f);
    head.lineTo(hx, hy + h * 0.32f);
    head.closeSubPath();
    g.setColour(fill);
    g.fillPath(body);
    g.fillPath(neck);
    g.fillPath(head);
    g.setColour(MiguelColours::background());
    g.fillEllipse(x + w * 0.16f, y + h * 0.38f, w * 0.20f, h * 0.22f);
}
}

void PichaBrandBar::paint(juce::Graphics& graphics)
{
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient wash(
        MiguelColours::panelHighlight().withAlpha(0.55f),
        bounds.getX(), bounds.getY(),
        MiguelColours::background(),
        bounds.getX(), bounds.getBottom(), false);
    graphics.setGradientFill(wash);
    graphics.fillAll();
    graphics.setColour(MiguelColours::purple().withAlpha(0.55f));
    graphics.fillRect(0.0f, bounds.getBottom() - 1.5f, bounds.getWidth(), 1.5f);
    graphics.setColour(MiguelColours::blue().withAlpha(0.45f));
    graphics.fillRect(0.0f, 0.0f, bounds.getWidth(), 1.0f);

    auto row = getLocalBounds().toFloat().reduced(10.0f, 6.0f);
    auto accordionArea = row.removeFromLeft(78.0f);
    row.removeFromLeft(6.0f);
    auto bajoArea = row.removeFromLeft(92.0f);
    row.removeFromLeft(12.0f);
    drawAccordionSilhouette(graphics, accordionArea, MiguelColours::lilac());
    drawBajoSextoSilhouette(graphics, bajoArea, MiguelColours::purple());

    auto titleArea = row;
    graphics.setFont(pichaTitleFont(38.0f));
    graphics.setColour(MiguelColours::yellow().withAlpha(0.28f));
    graphics.drawText("PICHADAW", titleArea.translated(1.0f, 1.0f),
                      juce::Justification::centredLeft, false);
    graphics.setColour(MiguelColours::lilac());
    graphics.drawText("PICHADAW", titleArea,
                      juce::Justification::centredLeft, false);
    graphics.setFont(juce::FontOptions(12.0f, juce::Font::italic));
    graphics.setColour(MiguelColours::blue());
    graphics.drawText("beta", titleArea.translated(214.0f, 8.0f)
                           .withWidth(48.0f).withHeight(22.0f),
                      juce::Justification::centredLeft, false);
}

void SampleDropCard::paint(juce::Graphics& graphics)
{
    const auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    juce::ColourGradient background(
        MiguelColours::panelRaised(), bounds.getCentreX(), bounds.getY(),
        MiguelColours::panel(), bounds.getCentreX(), bounds.getBottom(),
        false);
    graphics.setGradientFill(background);
    graphics.fillRoundedRectangle(bounds, 14.0f);
    graphics.setColour(MiguelColours::purple().withAlpha(0.7f));
    graphics.drawRoundedRectangle(bounds, 14.0f, 1.6f);
}

bool SampleDropCard::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& path : files)
        if (isAudioSampleFile(juce::File(path)))
            return true;
    return false;
}

void SampleDropCard::filesDropped(const juce::StringArray& files, int, int)
{
    if (onFilesDropped)
        onFilesDropped(files);
}

CapturePad::CapturePad() : juce::Button("REC")
{
    setWantsKeyboardFocus(false);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void CapturePad::setMode(int newMode)
{
    if (mode == newMode)
        return;
    mode = newMode;
    repaint();
}

void CapturePad::paintButton(juce::Graphics& graphics, bool, bool)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    juce::Colour fill = juce::Colours::white;
    if (mode == 2)
        fill = MiguelColours::danger();
    graphics.setColour(juce::Colours::black.withAlpha(0.35f));
    graphics.fillEllipse(bounds.translated(0.0f, 2.0f));
    graphics.setColour(fill);
    graphics.fillEllipse(bounds);
    graphics.setColour(MiguelColours::border());
    graphics.drawEllipse(bounds, 2.0f);
    graphics.setColour(mode == 0 ? juce::Colours::black
                                 : juce::Colours::white);
    graphics.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    graphics.drawText("REC", getLocalBounds(), juce::Justification::centred);
}

void CapturePad::clicked()
{
    if (onToggle)
        onToggle();
}

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
    graphics.setColour(MiguelColours::blue().withAlpha(0.8f));
    graphics.drawRoundedRectangle(bounds, 7.0f, 1.0f);

    if (thumbnail.getTotalLength() > 0.0)
    {
        graphics.setColour(MiguelColours::lilac());
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
    graphics.setColour(MiguelColours::blue().withAlpha(0.8f));
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

void configureSlider(juce::Slider& slider, double minimum, double maximum,
                     double interval, double value, const juce::String& suffix)
{
    slider.setRange(minimum, maximum, interval);
    slider.setValue(value);
    slider.setTextValueSuffix(suffix);
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 24);
}

void configureLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, MiguelColours::text());
}

juce::String foldButtonText(bool expanded, const juce::String& label)
{
    return (expanded ? "[v] " : "[>] ") + label;
}

void disableChildFocus(juce::Component& root)
{
    for (auto* child : root.getChildren())
    {
        if (child == nullptr)
            continue;
        child->setWantsKeyboardFocus(false);
        disableChildFocus(*child);
    }
}
}

MiguelMusicAssistantAudioProcessorEditor::
MiguelMusicAssistantAudioProcessorEditor(
    MiguelMusicAssistantAudioProcessor& processorToUse)
    : AudioProcessorEditor(&processorToUse),
      processor(processorToUse),
      rackPanel(processorToUse.getFxRack()),
      referencePiano(processorToUse.getRefPianoState())
{
    setLookAndFeel(&miguelLookAndFeel);
    setSize(1512, 810);
    setResizable(true, true);
    setResizeLimits(864, 648, 2074, 1260);
    setName("PICHADAW");

    addAndMakeVisible(menuBar);
    addAndMakeVisible(brandBar);
    addAndMakeVisible(libraryPage);
    addAndMakeVisible(capturePad);
    capturePad.setAlwaysOnTop(true);
    capturePad.onToggle = [this] { toggleCapturePad(); };
    setWantsKeyboardFocus(true);
    processor.getGrooveEngine().stop();
#if JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
        holder->getMuteInputValue().setValue(false);
#endif

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
    libraryPage.addAndMakeVisible(samplePlayButton);
    libraryPage.addAndMakeVisible(sampleLoopButton);
    libraryPage.addAndMakeVisible(sampleReverseButton);
    libraryPage.addAndMakeVisible(sampleDragButton);
    libraryPage.addAndMakeVisible(sampleInfo);
    libraryPage.addAndMakeVisible(tunerCell);
    tunerCell.addAndMakeVisible(tunerNeedle);
    tunerCell.addAndMakeVisible(pitchShiftKnob);
    tunerCell.addAndMakeVisible(pitchShiftLabel);
    tunerCell.addAndMakeVisible(broncoMaxKnob);
    tunerCell.addAndMakeVisible(broncoMaxLabel);
    tunerCell.addAndMakeVisible(tunerReadout);
    libraryPage.addAndMakeVisible(eqCell);
    eqCell.addAndMakeVisible(sampleEqResetButton);
    for (int band = 0; band < MiguelMusicAssistantAudioProcessor::sampleEqBandCount; ++band)
    {
        eqCell.addAndMakeVisible(sampleEqBandLabels[static_cast<size_t>(band)]);
        eqCell.addAndMakeVisible(sampleEqSliders[static_cast<size_t>(band)]);
    }
    libraryPage.addAndMakeVisible(trimCell);
    trimCell.addAndMakeVisible(sampleTrimStartKnob);
    trimCell.addAndMakeVisible(sampleTrimStartLabel);
    trimCell.addAndMakeVisible(sampleTrimEndKnob);
    trimCell.addAndMakeVisible(sampleTrimEndLabel);
    libraryPage.addAndMakeVisible(fadeInCell);
    fadeInCell.addAndMakeVisible(sampleFadeInKnob);
    libraryPage.addAndMakeVisible(fadeOutCell);
    fadeOutCell.addAndMakeVisible(sampleFadeOutKnob);
    libraryPage.addAndMakeVisible(rackPanel);
    libraryPage.addAndMakeVisible(importedSampleList);
    libraryPage.addAndMakeVisible(referencePiano);
    libraryPage.addAndMakeVisible(referencePianoCaption);
    configureLabel(libraryTitle, "Biblioteca de Samples");
    libraryTitle.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    configureLabel(folderLabel, "0 samples importados");
    configureLabel(sampleInfo, "Selecciona un archivo; doble clic para escucharlo.");
    configureLabel(referencePianoCaption,
                   "Seno siempre on · C4=261,6 Hz MIDI 60 · A4=440 · FL suele llamar C5 a esta misma nota");
    referencePianoCaption.setFont(juce::FontOptions(12.0f));
    referencePianoCaption.setJustificationType(juce::Justification::centredLeft);
    configureLabel(tunerTitle, "Afinador de samples");
    tunerTitle.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    configureLabel(tunerReadout, "Tono: esperando sample");
    configureLabel(pitchShiftLabel, "Cambio de tono");
    pitchShiftLabel.setJustificationType(juce::Justification::centred);
    pitchShiftKnob.setRange(-12.0, 12.0, 0.005);
    pitchShiftKnob.setNumDecimalPlacesToDisplay(3);
    pitchShiftKnob.setValue(0.0);
    pitchShiftKnob.setTextValueSuffix(" st");
    pitchShiftKnob.setDoubleClickReturnValue(true, 0.0);
    pitchShiftKnob.setWheelStepMultiplier(1.0);
    pitchShiftKnob.setMouseDragSensitivity(2500);
    pitchShiftKnob.setTextBoxStyle(
        juce::Slider::TextBoxBelow, false, 82, 22);
    pitchShiftKnob.setColour(juce::Slider::rotarySliderFillColourId,
                             MiguelColours::purple());
    pitchShiftKnob.onValueChange = [this]
    {
        const auto centsAsSemitones = pitchShiftKnob.getValue();
        const auto slots = importedSampleList.getSelectedSlots();
        if (slots.isEmpty())
            processor.setSamplePitchSemitones(centsAsSemitones, -1);
        else
            for (int i = 0; i < slots.size(); ++i)
                processor.setSamplePitchSemitones(centsAsSemitones, slots[i]);
        updateTunerDisplay();
    };
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
                           MiguelColours::yellow());
    broncoMaxKnob.onValueChange = [this]
    {
        processor.getSectionEqBank().get(AudioSection::samples)
            .setBroncoMax(static_cast<float>(broncoMaxKnob.getValue() / 100.0));
        updateTunerDisplay();
    };
    folderButton.onClick = [this] { chooseSampleFiles(); };
    removeSampleButton.onClick = [this]
    {
        importedSampleList.removeSelected();
    };
    importedSampleList.onSelectionChanged = [this](const juce::File& file)
    {
        if (file.existsAsFile())
        {
            updateSelectedSample(file);
            if (processor.isSamplePlaying())
                processor.playSlots(importedSampleList.getSelectedSlots());
        }
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
        processor.playSlots(importedSampleList.getSelectedSlots());
    };
    importedSampleList.onSampleCountChanged = [this](int count)
    {
        folderLabel.setText(juce::String(count)
                                + (count == 1 ? " sample importado"
                                              : " samples importados"),
                            juce::dontSendNotification);
    };
    importedSampleList.onSlotLoaded = [this](int slot, const juce::File& file)
    {
        if (file.existsAsFile())
            processor.loadLayerSample(slot, file);
        else
            processor.getLayerBank().clearSlot(slot);
    };
    importedSampleList.onMuteChanged = [this](int slot, bool muted)
    {
        processor.getLayerBank().setMuted(slot, muted);
    };
    importedSampleList.onVolumeChanged = [this](int slot, float volume)
    {
        processor.getLayerBank().setVolume(slot, volume);
    };
    importedSampleList.onEqChanged = [this](int slot, float amount)
    {
        processor.getLayerBank().setEqTilt(slot, amount);
    };
    sampleLoopButton.setClickingTogglesState(true);
    sampleLoopButton.setToggleState(true, juce::dontSendNotification);
    sampleLoopButton.setColour(juce::TextButton::buttonOnColourId,
                               MiguelColours::green());
    processor.setSampleLooping(true);
    sampleLoopButton.onClick = [this]
    {
        processor.setSampleLooping(sampleLoopButton.getToggleState());
    };
    sampleReverseButton.setClickingTogglesState(true);
    sampleReverseButton.setToggleState(false, juce::dontSendNotification);
    sampleReverseButton.setColour(juce::TextButton::buttonOnColourId,
                                  MiguelColours::green());
    sampleReverseButton.onClick = [this]
    {
        processor.setSampleReversed(sampleReverseButton.getToggleState());
    };
    samplePlayButton.onClick = [this] { toggleSampleTrigger(); };
    sampleDragButton.onClick = [this]
    {
        if (selectedSample.existsAsFile())
            selectedSample.revealToUser();
    };

    static const juce::String eqNames[]{
        "60", "160", "400", "1k", "2.5k", "6.3k", "12k"
    };
    static const juce::Colour sampleEqColours[]{
        MiguelColours::blue(), MiguelColours::lilac(), MiguelColours::cyan(),
        MiguelColours::green(), MiguelColours::purple(), MiguelColours::blue(),
        MiguelColours::lilac()
    };
    for (int band = 0; band < MiguelMusicAssistantAudioProcessor::sampleEqBandCount; ++band)
    {
        configureLabel(sampleEqBandLabels[static_cast<size_t>(band)],
                       eqNames[band]);
        sampleEqBandLabels[static_cast<size_t>(band)]
            .setJustificationType(juce::Justification::centred);
        auto& slider = sampleEqSliders[static_cast<size_t>(band)];
        slider.setSliderStyle(juce::Slider::LinearVertical);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setRange(-18.0, 18.0, 0.1);
        slider.setWheelStepMultiplier(4.0);
        slider.setValue(processor.getSampleEqGain(band),
                        juce::dontSendNotification);
        slider.setDoubleClickReturnValue(true, 0.0);
        slider.setColour(juce::Slider::thumbColourId, sampleEqColours[band]);
        slider.setColour(juce::Slider::trackColourId, sampleEqColours[band]);
        slider.onValueChange = [this, band]
        {
            processor.setSampleEqGain(
                band,
                static_cast<float>(
                    sampleEqSliders[static_cast<size_t>(band)].getValue()));
        };
    }
    sampleEqResetButton.onClick = [this]
    {
        for (int band = 0; band < MiguelMusicAssistantAudioProcessor::sampleEqBandCount; ++band)
            sampleEqSliders[static_cast<size_t>(band)].setValue(0.0);
    };

    configureLabel(sampleTrimStartLabel, "Inicio");
    sampleTrimStartLabel.setJustificationType(juce::Justification::centred);
    sampleTrimStartKnob.setRange(0.0, 90.0, 1.0);
    sampleTrimStartKnob.setValue(0.0);
    sampleTrimStartKnob.setTextValueSuffix(" %");
    sampleTrimStartKnob.setDoubleClickReturnValue(true, 0.0);
    sampleTrimStartKnob.setWheelStepMultiplier(1.0);
    sampleTrimStartKnob.setMouseDragSensitivity(2500);
    sampleTrimStartKnob.setColour(juce::Slider::rotarySliderFillColourId,
                                  MiguelColours::yellow());
    sampleTrimStartKnob.onValueChange = [this]
    {
        processor.setSampleTrimStart(
            static_cast<float>(sampleTrimStartKnob.getValue() / 100.0));
    };
    configureLabel(sampleTrimEndLabel, "Final");
    sampleTrimEndLabel.setJustificationType(juce::Justification::centred);
    sampleTrimEndKnob.setRange(10.0, 100.0, 1.0);
    sampleTrimEndKnob.setValue(100.0);
    sampleTrimEndKnob.setTextValueSuffix(" %");
    sampleTrimEndKnob.setDoubleClickReturnValue(true, 100.0);
    sampleTrimEndKnob.setWheelStepMultiplier(1.0);
    sampleTrimEndKnob.setMouseDragSensitivity(2500);
    sampleTrimEndKnob.setColour(juce::Slider::rotarySliderFillColourId,
                                MiguelColours::orange());
    sampleTrimEndKnob.onValueChange = [this]
    {
        processor.setSampleTrimEnd(
            static_cast<float>(sampleTrimEndKnob.getValue() / 100.0));
    };
    configureLabel(sampleFadeInLabel, "Entrada");
    sampleFadeInLabel.setJustificationType(juce::Justification::centred);
    sampleFadeInKnob.setRange(0.0, 100.0, 1.0);
    sampleFadeInKnob.setValue(0.0);
    sampleFadeInKnob.setTextValueSuffix(" %");
    sampleFadeInKnob.setDoubleClickReturnValue(true, 0.0);
    sampleFadeInKnob.setWheelStepMultiplier(1.0);
    sampleFadeInKnob.setMouseDragSensitivity(2500);
    sampleFadeInKnob.setColour(juce::Slider::rotarySliderFillColourId,
                               MiguelColours::blue());
    sampleFadeInKnob.onValueChange = [this]
    {
        processor.setSampleFadeIn(
            static_cast<float>(sampleFadeInKnob.getValue() / 100.0));
    };
    configureLabel(sampleFadeOutLabel, "Salida");
    sampleFadeOutLabel.setJustificationType(juce::Justification::centred);
    sampleFadeOutKnob.setRange(0.0, 100.0, 1.0);
    sampleFadeOutKnob.setValue(0.0);
    sampleFadeOutKnob.setTextValueSuffix(" %");
    sampleFadeOutKnob.setDoubleClickReturnValue(true, 0.0);
    sampleFadeOutKnob.setWheelStepMultiplier(1.0);
    sampleFadeOutKnob.setMouseDragSensitivity(2500);
    sampleFadeOutKnob.setColour(juce::Slider::rotarySliderFillColourId,
                                MiguelColours::lilac());
    sampleFadeOutKnob.onValueChange = [this]
    {
        processor.setSampleFadeOut(
            static_cast<float>(sampleFadeOutKnob.getValue() / 100.0));
    };
    rackPanel.setCompact(true);

    for (auto* child : {
             static_cast<juce::Component*>(&bajoquintoTitle),
             static_cast<juce::Component*>(&bajoquintoDescription),
             static_cast<juce::Component*>(&layerPlayButton),
             static_cast<juce::Component*>(&layerLoopButton),
             static_cast<juce::Component*>(&layerStatus) })
        bajoquintoPage.addAndMakeVisible(child);

    configureLabel(bajoquintoTitle, "Acordes Bajoquinto");
    bajoquintoTitle.setFont(juce::FontOptions(25.0f, juce::Font::bold));
    configureLabel(
        bajoquintoDescription,
        "Carga 4 samples y oyelos a la vez. Cada cuadro tiene volumen y tono.");
    configureLabel(layerStatus, "Arrastra un WAV a un cuadro o pulsa Cargar sample.");
    layerStatus.setJustificationType(juce::Justification::centredLeft);

    layerLoopButton.setClickingTogglesState(true);
    layerLoopButton.setToggleState(true, juce::dontSendNotification);
    layerLoopButton.setColour(juce::TextButton::buttonOnColourId,
                              MiguelColours::green());
    processor.getLayerBank().setLooping(true);
    layerLoopButton.onClick = [this]
    {
        processor.getLayerBank().setLooping(layerLoopButton.getToggleState());
    };
    layerPlayButton.onClick = [this]
    {
        processor.getLayerBank().setLooping(layerLoopButton.getToggleState());
        toggleSampleTrigger();
        layerStatus.setText(
            processor.getLayerBank().isPlaying()
                ? "Reproduciendo la seleccion."
                : "Selecciona un sample para escuchar.",
            juce::dontSendNotification);
    };

    for (int slot = 0; slot < 4; ++slot)
    {
        auto& card = layerCards[static_cast<size_t>(slot)];
        auto& title = layerTitles[static_cast<size_t>(slot)];
        auto& load = layerLoadButtons[static_cast<size_t>(slot)];
        auto& volume = layerVolumeKnobs[static_cast<size_t>(slot)];
        auto& pitch = layerPitchKnobs[static_cast<size_t>(slot)];
        auto& volumeLabel = layerVolumeLabels[static_cast<size_t>(slot)];
        auto& muteLed = layerMuteLeds[static_cast<size_t>(slot)];
        auto& pitchLabel = layerPitchLabels[static_cast<size_t>(slot)];

        bajoquintoPage.addAndMakeVisible(card);
        card.addAndMakeVisible(title);
        card.addAndMakeVisible(load);
        card.addAndMakeVisible(volume);
        card.addAndMakeVisible(pitch);
        card.addAndMakeVisible(volumeLabel);
        card.addAndMakeVisible(muteLed);
        card.addAndMakeVisible(pitchLabel);

        configureLabel(title, "Sample " + juce::String(slot + 1));
        title.setFont(juce::FontOptions(18.0f, juce::Font::bold));
        title.setJustificationType(juce::Justification::centred);
        load.setButtonText("Cargar sample");
        load.onClick = [this, slot] { chooseLayerSample(slot); };
        card.onFilesDropped = [this, slot](const juce::StringArray& paths)
        {
            juce::Array<juce::File> files;
            for (const auto& path : paths)
            {
                const juce::File file(path);
                if (file.existsAsFile() && isAudioSampleFile(file))
                    files.add(file);
            }
            applyLayerFiles(slot, files);
        };

        configureLabel(volumeLabel, "Volumen");
        volumeLabel.setJustificationType(juce::Justification::centred);
        volume.setRange(0.0, 1.5, 0.01);
        volume.setValue(1.0);
        volume.setDoubleClickReturnValue(true, 1.0);
        volume.setColour(juce::Slider::rotarySliderFillColourId,
                         MiguelColours::purple());
        volume.onValueChange = [this, slot]
        {
            processor.getLayerBank().setVolume(
                slot,
                static_cast<float>(
                    layerVolumeKnobs[static_cast<size_t>(slot)].getValue()));
        };

        muteLed.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        muteLed.setLevel(0.0f, false);
        muteLed.onClick = [this, slot]
        {
            auto& layers = processor.getLayerBank();
            layers.setMuted(slot, !layers.isMuted(slot));
            layerMuteLeds[static_cast<size_t>(slot)].setLevel(
                layers.getLed(slot), layers.isMuted(slot));
        };

        configureLabel(pitchLabel, "Tono");
        pitchLabel.setJustificationType(juce::Justification::centred);
        pitch.setRange(-12.0, 12.0, 0.01);
        pitch.setValue(0.0);
        pitch.setTextValueSuffix(" st");
        pitch.setDoubleClickReturnValue(true, 0.0);
        pitch.setColour(juce::Slider::rotarySliderFillColourId,
                        MiguelColours::orange());
        pitch.onValueChange = [this, slot]
        {
            processor.getLayerBank().setPitchSemitones(
                slot, layerPitchKnobs[static_cast<size_t>(slot)].getValue());
        };
    }

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
    configureLabel(eqTitle, "EQ 7 bandas");
    eqTitle.setFont(juce::FontOptions(25.0f, juce::Font::bold));
    configureLabel(eqSectionLabel, "Sección de audio");
    eqSectionBox.addItemList(
        { "Generador", "Samples", "Acordes", "Piano" }, 1);
    eqSectionBox.setSelectedId(2);
    configureLabel(inputEqLabel, "EQ 7 bandas");
    inputEqLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
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
            eqPage.addAndMakeVisible(&input);
            eqPage.addAndMakeVisible(&inputLabel);
            eqPage.addAndMakeVisible(&output);
            eqPage.addAndMakeVisible(&outputLabel);

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
    graphicEqDisplay.setActiveStage(false);
    updateEqControls();
    updateFoldVisibility();

    if (const auto ui = processor.getUiSessionState(); ui.isValid()
        && ui.getNumChildren() > 0)
        restoreUiSessionState(ui);
    else if (processor.loadAutosaveSession())
        restoreUiSessionState(processor.getUiSessionState());

    {
        auto& eq = processor.getSectionEqBank().get(AudioSection::samples);
        for (int band = 0; band < SectionEq::bandCount; ++band)
        {
            eq.setBandGain(false, band, 0.0f);
            eq.setBandGain(true, band, 0.0f);
        }
    }

    startTimerHz(24);
    disableChildFocus(*this);
    setWantsKeyboardFocus(true);
    grabKeyboardFocus();
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
    pianoGenerationProcess.kill();
    studioPool.removeAllJobs(true, 3000);
    processor.stopPreviews();
    if (keyListenerHost != nullptr)
        keyListenerHost->removeKeyListener(this);
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
    auto bounds = getLocalBounds();
    menuBar.setBounds(bounds.removeFromTop(22));
    auto brandRow = bounds.removeFromTop(56);
    capturePad.setBounds(brandRow.removeFromRight(64).reduced(4, 4));
    brandBar.setBounds(brandRow);
    libraryPage.setBounds(bounds.reduced(8, 4));
    capturePad.toFront(false);

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
    libraryTitle.setBounds(library.removeFromTop(32));
    library.removeFromTop(4);

    auto listColumn = library.removeFromRight(library.getWidth() / 3);
    library.removeFromRight(10);
    folderButton.setBounds(listColumn.removeFromTop(34).reduced(2, 2));
    removeSampleButton.setBounds(listColumn.removeFromTop(34).reduced(2, 2));
    folderLabel.setBounds(listColumn.removeFromTop(26).reduced(4, 0));
    listColumn.removeFromTop(4);
    importedSampleList.setBounds(listColumn);

    auto left = library;
    auto sampleControls = left.removeFromTop(34);
    samplePlayButton.setBounds(sampleControls.removeFromLeft(34).reduced(1));
    sampleLoopButton.setBounds(sampleControls.removeFromLeft(64).reduced(3));
    sampleReverseButton.setBounds(sampleControls.removeFromLeft(56).reduced(3));
    sampleDragButton.setBounds(sampleControls.removeFromLeft(180).reduced(3));
    sampleInfo.setBounds(sampleControls.reduced(8, 0));
    left.removeFromTop(6);

    auto topPlugins = left.removeFromTop(juce::jlimit(120, 160, left.getHeight() / 4));
    auto eqBounds = topPlugins.removeFromRight(topPlugins.getWidth() * 2 / 5);
    tunerCell.setBounds(topPlugins);
    eqCell.setBounds(eqBounds);
    {
        auto inner = tunerCell.contentArea();
        tunerNeedle.setBounds(inner.removeFromLeft(juce::jmin(220, inner.getWidth() / 2)).reduced(2));
        auto knobArea = inner.removeFromLeft(96);
        pitchShiftLabel.setBounds(knobArea.removeFromTop(16));
        pitchShiftKnob.setBounds(knobArea.reduced(2, 0));
        auto broncoArea = inner.removeFromLeft(96);
        broncoMaxLabel.setBounds(broncoArea.removeFromTop(16));
        broncoMaxKnob.setBounds(broncoArea.reduced(2, 0));
        tunerReadout.setBounds(inner.reduced(4, 0));
    }
    {
        auto inner = eqCell.contentArea();
        sampleEqResetButton.setBounds(inner.removeFromRight(56).reduced(2, 10));
        const auto cellW = juce::jmax(
            1, inner.getWidth() / MiguelMusicAssistantAudioProcessor::sampleEqBandCount);
        for (int band = 0; band < MiguelMusicAssistantAudioProcessor::sampleEqBandCount; ++band)
        {
            auto cell = inner.removeFromLeft(cellW);
            sampleEqBandLabels[static_cast<size_t>(band)].setBounds(
                cell.removeFromBottom(16));
            sampleEqSliders[static_cast<size_t>(band)].setBounds(cell.reduced(2, 0));
        }
    }
    left.removeFromTop(6);
    auto shapeRow = left.removeFromTop(108);
    const auto shapeW = shapeRow.getWidth() / 3;
    trimCell.setBounds(shapeRow.removeFromLeft(shapeW));
    fadeInCell.setBounds(shapeRow.removeFromLeft(shapeW));
    fadeOutCell.setBounds(shapeRow);
    {
        auto inner = trimCell.contentArea();
        auto leftKnob = inner.removeFromLeft(inner.getWidth() / 2);
        sampleTrimStartLabel.setBounds(leftKnob.removeFromTop(16));
        sampleTrimStartKnob.setBounds(leftKnob.reduced(4, 0));
        sampleTrimEndLabel.setBounds(inner.removeFromTop(16));
        sampleTrimEndKnob.setBounds(inner.reduced(4, 0));
    }
    sampleFadeInKnob.setBounds(fadeInCell.contentArea().reduced(8, 0));
    sampleFadeOutKnob.setBounds(fadeOutCell.contentArea().reduced(8, 0));
    left.removeFromTop(6);
    {
        const auto pianoKeysH = ReferencePianoComponent::keyHeight();
        auto pianoStrip = left.removeFromBottom(pianoKeysH + 18);
        left.removeFromBottom(6);
        referencePianoCaption.setBounds(pianoStrip.removeFromBottom(16));
        referencePiano.setBounds(pianoStrip);
    }
    rackPanel.setBounds(left);

    auto bajoquinto = bajoquintoPage.getLocalBounds().reduced(22);
    bajoquintoTitle.setBounds(bajoquinto.removeFromTop(40));
    bajoquintoDescription.setBounds(bajoquinto.removeFromTop(28));
    bajoquinto.removeFromTop(8);
    auto actions = bajoquinto.removeFromTop(44);
    layerPlayButton.setBounds(actions.removeFromLeft(160).reduced(3));
    layerLoopButton.setBounds(actions.removeFromLeft(100).reduced(3));
    layerStatus.setBounds(actions.reduced(8, 4));
    bajoquinto.removeFromTop(10);

    const auto gap = 12;
    const auto cardWidth = (bajoquinto.getWidth() - gap) / 2;
    const auto cardHeight = (bajoquinto.getHeight() - gap) / 2;
    for (int slot = 0; slot < 4; ++slot)
    {
        const auto column = slot % 2;
        const auto rowIndex = slot / 2;
        auto cardBounds = juce::Rectangle<int>(
            bajoquinto.getX() + column * (cardWidth + gap),
            bajoquinto.getY() + rowIndex * (cardHeight + gap),
            cardWidth, cardHeight);
        auto& card = layerCards[static_cast<size_t>(slot)];
        card.setBounds(cardBounds);
        auto inner = card.getLocalBounds().reduced(16);
        layerTitles[static_cast<size_t>(slot)].setBounds(
            inner.removeFromTop(28));
        inner.removeFromTop(6);
        layerLoadButtons[static_cast<size_t>(slot)].setBounds(
            inner.removeFromTop(40).reduced(0, 2));
        inner.removeFromTop(8);
        auto knobs = inner.removeFromTop(juce::jmin(150, inner.getHeight()));
        auto volumeArea = knobs.removeFromLeft(knobs.getWidth() / 2).reduced(8, 0);
        auto pitchArea = knobs.reduced(8, 0);
        auto volumeHeader = volumeArea.removeFromTop(22);
        layerMuteLeds[static_cast<size_t>(slot)].setBounds(
            volumeHeader.removeFromLeft(24).reduced(1));
        layerVolumeLabels[static_cast<size_t>(slot)].setBounds(volumeHeader);
        layerVolumeKnobs[static_cast<size_t>(slot)].setBounds(volumeArea);
        layerPitchLabels[static_cast<size_t>(slot)].setBounds(
            pitchArea.removeFromTop(22));
        layerPitchKnobs[static_cast<size_t>(slot)].setBounds(pitchArea);
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
    processor.setSamplePitchSemitones(0.0, importedSampleList.getSelectedSlot());
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

void MiguelMusicAssistantAudioProcessorEditor::chooseLayerSample(int slot)
{
    auto startFolder = juce::File(
        "C:\\Users\\MIGUEL\\OneDrive2\\Desktop\\creador-acordes-bajoquinto");
    if (!startFolder.isDirectory())
        startFolder = juce::File::getSpecialLocation(juce::File::userMusicDirectory);

    fileChooser = std::make_unique<juce::FileChooser>(
        "Carga un sample en el cuadro " + juce::String(slot + 1),
        startFolder,
        "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::canSelectMultipleItems,
        [this, slot](const juce::FileChooser& chooser)
        {
            applyLayerFiles(slot, chooser.getResults());
        });
}

void MiguelMusicAssistantAudioProcessorEditor::applyLayerFiles(
    int startSlot, const juce::Array<juce::File>& files)
{
    auto loaded = 0;
    for (int index = 0; index < files.size(); ++index)
    {
        const auto target = startSlot + index;
        if (target >= SampleLayerBank::slotCount)
            break;
        const auto& file = files.getReference(index);
        if (!file.existsAsFile() || !isAudioSampleFile(file))
            continue;
        if (processor.loadLayerSample(target, file))
        {
            importedSampleList.addFiles({ file });
            ++loaded;
        }
    }
    refreshLayerSlotButtons();
    if (loaded > 0)
        layerStatus.setText(
            loaded == 1 ? "Sample cargado. Pulsa Escuchar para oírlos juntos."
                        : juce::String(loaded)
                            + " samples cargados. Pulsa Escuchar para oírlos juntos.",
            juce::dontSendNotification);
    else
        layerStatus.setText("No se pudo cargar el audio.",
                            juce::dontSendNotification);
}

void MiguelMusicAssistantAudioProcessorEditor::refreshLayerSlotButtons()
{
    auto& layers = processor.getLayerBank();
    for (int slot = 0; slot < 4; ++slot)
        layerLoadButtons[static_cast<size_t>(slot)].setButtonText(
            layers.getSlotName(slot));
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
    return AudioSection::samples;
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
    pianoTitle.setVisible(false);
    pianoFoldButton.setVisible(false);
    for (auto* component : {
             static_cast<juce::Component*>(&pianoKeyboard),
             static_cast<juce::Component*>(&preparePianoButton),
             static_cast<juce::Component*>(&pianoRecordButton),
             static_cast<juce::Component*>(&pianoStopButton),
             static_cast<juce::Component*>(&pianoPlaybackButton),
             static_cast<juce::Component*>(&pianoExportButton),
             static_cast<juce::Component*>(&pianoStatus) })
        component->setVisible(false);

    inputEqLabel.setVisible(false);
    graphicEqDisplay.setVisible(false);
    resetEqButton.setVisible(false);
    outputEqLabel.setVisible(false);
    inputEqFoldButton.setVisible(false);
    outputEqFoldButton.setVisible(false);
    eqTitle.setVisible(false);
    eqSectionBox.setVisible(false);
    eqSectionLabel.setVisible(false);
    sectionVolumeKnob.setVisible(false);
    sectionVolumeLabel.setVisible(false);
    sectionBroncoKnob.setVisible(false);
    sectionBroncoLabel.setVisible(false);
    for (int band = 0; band < SectionEq::bandCount; ++band)
    {
        inputEqSliders[static_cast<size_t>(band)].setVisible(false);
        inputEqBandLabels[static_cast<size_t>(band)].setVisible(false);
        outputEqSliders[static_cast<size_t>(band)].setVisible(false);
        outputEqBandLabels[static_cast<size_t>(band)].setVisible(false);
    }
}

void MiguelMusicAssistantAudioProcessorEditor::handleNoteOn(
    juce::MidiKeyboardState*, int, int midiNote, float velocity)
{
    processor.getPianoEngine().noteOn(
        midiNote,
        processor.getFxRack().isMuted(FxSlot::velocity)
            ? velocity
            : velocity * processor.getFxRack().velocityGain());
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
    auto startFolder = juce::File::getSpecialLocation(
        juce::File::userDocumentsDirectory)
        .getChildFile("Miguel Music Assistant")
        .getChildFile("Grabaciones");
    if (!startFolder.isDirectory())
        startFolder = juce::File::getSpecialLocation(
            juce::File::userMusicDirectory);
    fileChooser = std::make_unique<juce::FileChooser>(
        "Selecciona uno o varios samples",
        startFolder,
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

void MiguelMusicAssistantAudioProcessorEditor::parentHierarchyChanged()
{
    if (keyListenerHost != nullptr)
        keyListenerHost->removeKeyListener(this);
    keyListenerHost = getTopLevelComponent();
    if (keyListenerHost != nullptr)
        keyListenerHost->addKeyListener(this);
    applyWindowChrome();
}

bool MiguelMusicAssistantAudioProcessorEditor::keyPressed(
    const juce::KeyPress& key, juce::Component*)
{
    return keyPressed(key);
}

bool MiguelMusicAssistantAudioProcessorEditor::keyPressed(
    const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey)
    {
        toggleSampleTrigger();
        return true;
    }
    const auto character = key.getTextCharacter();
    const auto code = key.getKeyCode();
    if (character == 'g' || character == 'G' || code == 'g' || code == 'G')
    {
        toggleCapturePad();
        return true;
    }
    return false;
}

void MiguelMusicAssistantAudioProcessorEditor::mouseDown(
    const juce::MouseEvent& event)
{
    grabKeyboardFocus();
    juce::AudioProcessorEditor::mouseDown(event);
}

void MiguelMusicAssistantAudioProcessorEditor::visibilityChanged()
{
    if (isShowing())
        grabKeyboardFocus();
    applyWindowChrome();
}

void MiguelMusicAssistantAudioProcessorEditor::toggleCapturePad()
{
    processor.toggleCapture();
    capturePad.setMode(processor.getCaptureState());
}

void MiguelMusicAssistantAudioProcessorEditor::toggleSampleTrigger()
{
    processor.setSampleLooping(sampleLoopButton.getToggleState());
    processor.setSampleReversed(sampleReverseButton.getToggleState());
    if (processor.isSamplePlaying())
        processor.stopSamplePlayback();
    else
        processor.playSlots(importedSampleList.getSelectedSlots());
}

void MiguelMusicAssistantAudioProcessorEditor::timerCallback()
{
    rackPanel.pushLeds();
    samplePlayButton.setPlaying(processor.isSamplePlaying());
    layerPlayButton.setButtonText(
        processor.getLayerBank().isPlaying() ? "Reproduciendo..." : "Escuchar");
    capturePad.setMode(processor.getCaptureState() == 3
        ? 0 : processor.getCaptureState());
    juce::File recorded;
    if (processor.takeCompletedCapture(recorded))
    {
        importedSampleList.addOrReplaceLast(recorded);
        sampleInfo.setText("Grabado: " + recorded.getFileName(),
                           juce::dontSendNotification);
        capturePad.setMode(0);
    }
    auto& layers = processor.getLayerBank();
    for (int slot = 0; slot < 4; ++slot)
        layerMuteLeds[static_cast<size_t>(slot)].setLevel(
            importedSampleList.getSelectedSlot() == slot
                && !layers.isMuted(slot) ? 1.0f : 0.0f,
            false);
    for (int slot = 0; slot < SampleLayerBank::slotCount; ++slot)
        importedSampleList.setSlotLed(slot);
}

void MiguelMusicAssistantAudioProcessorEditor::syncSessionStateToProcessor()
{
    processor.setUiSessionState(captureUiSessionState());
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

    juce::ValueTree bajoquinto("Bajoquinto");
    bajoquinto.setProperty(
        "looping", processor.getLayerBank().isLooping() ? 1 : 0, nullptr);
    bajoquinto.setProperty(
        "reversed", processor.getLayerBank().isReversed() ? 1 : 0, nullptr);
    for (int slot = 0; slot < SampleLayerBank::slotCount; ++slot)
    {
        juce::ValueTree layer("Slot");
        layer.setProperty("index", slot, nullptr);
        layer.setProperty("path", processor.getLayerBank().getSlotPath(slot),
                          nullptr);
        layer.setProperty("volume", processor.getLayerBank().getVolume(slot),
                          nullptr);
        layer.setProperty(
            "pitch", processor.getLayerBank().getPitchSemitones(slot), nullptr);
        layer.setProperty(
            "muted", processor.getLayerBank().isMuted(slot) ? 1 : 0, nullptr);
        bajoquinto.appendChild(layer, nullptr);
    }
    ui.appendChild(bajoquinto, nullptr);

    juce::ValueTree sampleEq("SampleEq");
    for (int band = 0; band < MiguelMusicAssistantAudioProcessor::sampleEqBandCount; ++band)
        sampleEq.setProperty("b" + juce::String(band),
                             processor.getSampleEqGain(band), nullptr);
    ui.appendChild(sampleEq, nullptr);

    ui.setProperty("selectedSamplePath",
                   selectedSample.getFullPathName(), nullptr);
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

    if (const auto bajoquinto = uiState.getChildWithName("Bajoquinto");
        bajoquinto.isValid())
    {
        const auto looping = static_cast<int>(
            bajoquinto.getProperty("looping", 1)) != 0;
        layerLoopButton.setToggleState(looping, juce::dontSendNotification);
        sampleLoopButton.setToggleState(looping, juce::dontSendNotification);
        processor.getLayerBank().setLooping(looping);
        const auto reversed = static_cast<int>(
            bajoquinto.getProperty("reversed", 0)) != 0;
        sampleReverseButton.setToggleState(reversed, juce::dontSendNotification);
        processor.setSampleReversed(reversed);

        for (int i = 0; i < bajoquinto.getNumChildren(); ++i)
        {
            const auto layer = bajoquinto.getChild(i);
            if (!layer.hasType("Slot"))
                continue;
            const auto slot = static_cast<int>(layer.getProperty("index", -1));
            if (!juce::isPositiveAndBelow(slot, SampleLayerBank::slotCount))
                continue;

            if (slot < 4)
            {
                layerVolumeKnobs[static_cast<size_t>(slot)].setValue(
                    static_cast<double>(layer.getProperty("volume", 1.0)),
                    juce::dontSendNotification);
                layerPitchKnobs[static_cast<size_t>(slot)].setValue(
                    static_cast<double>(layer.getProperty("pitch", 0.0)),
                    juce::dontSendNotification);
            }
            processor.getLayerBank().setVolume(
                slot, static_cast<float>(layer.getProperty("volume", 1.0)));
            processor.getLayerBank().setPitchSemitones(
                slot, static_cast<double>(layer.getProperty("pitch", 0.0)));
            processor.getLayerBank().setMuted(
                slot, static_cast<int>(layer.getProperty("muted", 0)) != 0);

            const auto path = layer.getProperty("path", juce::String())
                                  .toString();
            if (path.isNotEmpty())
            {
                const juce::File file(path);
                if (file.existsAsFile())
                    processor.loadLayerSample(slot, file);
            }
        }
        refreshLayerSlotButtons();
    }

    if (const auto sampleEq = uiState.getChildWithName("SampleEq");
        sampleEq.isValid())
    {
        for (int band = 0; band < MiguelMusicAssistantAudioProcessor::sampleEqBandCount; ++band)
        {
            const auto key = "b" + juce::String(band);
            const auto fallback = band == 0 ? "low"
                : band == 2 ? "mid"
                : band == 4 ? "high" : juce::String();
            auto value = static_cast<double>(sampleEq.getProperty(key, 0.0));
            if (std::abs(value) < 0.001 && fallback.isNotEmpty())
                value = static_cast<double>(sampleEq.getProperty(fallback, 0.0));
            sampleEqSliders[static_cast<size_t>(band)].setValue(value);
        }
    }

    updateEqControls();
    refreshLayerSlotButtons();
    rackPanel.refreshFromRack();
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

juce::StringArray MiguelMusicAssistantAudioProcessorEditor::getMenuBarNames()
{
    return { "Archivo", "Ventana" };
}

juce::PopupMenu MiguelMusicAssistantAudioProcessorEditor::getMenuForIndex(
    int index, const juce::String&)
{
    juce::PopupMenu menu;
    if (index == 0)
    {
        menu.addItem(1, "Abrir...");
        menu.addItem(2, "Guardar");
        menu.addItem(3, "Guardar como...");
        menu.addSeparator();
        menu.addItem(4, "Exportar samples seleccionados...");
        menu.addSeparator();
        menu.addItem(5, "Salir");
    }
    else if (index == 1)
    {
        menu.addItem(10, "Maximizar");
        menu.addItem(11, "Restaurar");
    }
    return menu;
}

void MiguelMusicAssistantAudioProcessorEditor::menuItemSelected(
    int itemId, int)
{
    switch (itemId)
    {
        case 1: openSession(); break;
        case 2: saveSession(false); break;
        case 3: saveSession(true); break;
        case 4: exportSelectedSamples(); break;
        case 5:
            if (auto* app = juce::JUCEApplicationBase::getInstance())
                app->systemRequestedQuit();
            break;
        case 10: maximiseWindow(); break;
        case 11: restoreWindow(); break;
        default: break;
    }
}

void MiguelMusicAssistantAudioProcessorEditor::openSession()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Abrir sesion",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("Miguel Music Assistant"),
        "*.pichadaw;*.xml");
    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();
            if (!file.existsAsFile())
                return;
            const auto xml = juce::parseXML(file);
            if (xml == nullptr)
                return;
            sessionFile = file;
            processor.restoreFullSessionState(juce::ValueTree::fromXml(*xml));
            restoreUiSessionState(processor.getUiSessionState());
        });
}

void MiguelMusicAssistantAudioProcessorEditor::saveSession(bool saveAs)
{
    syncSessionStateToProcessor();
    const auto writeTo = [this](const juce::File& file)
    {
        sessionFile = file;
        file.getParentDirectory().createDirectory();
        if (auto xml = processor.buildFullSessionState().createXml())
            xml->writeTo(file);
    };
    if (!saveAs && sessionFile.existsAsFile())
    {
        writeTo(sessionFile);
        return;
    }
    fileChooser = std::make_unique<juce::FileChooser>(
        "Guardar sesion",
        sessionFile.existsAsFile()
            ? sessionFile
            : juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                  .getChildFile("Miguel Music Assistant")
                  .getChildFile("sesion.pichadaw"),
        "*.pichadaw;*.xml");
    fileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::warnAboutOverwriting,
        [this, writeTo](const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (file.getFileName().isEmpty())
                return;
            if (file.getFileExtension().isEmpty())
                file = file.withFileExtension(".pichadaw");
            writeTo(file);
        });
}

void MiguelMusicAssistantAudioProcessorEditor::exportSelectedSamples()
{
    auto files = importedSampleList.getSelectedFiles();
    if (files.isEmpty() && selectedSample.existsAsFile())
        files.add(selectedSample);
    if (files.isEmpty())
        return;
    fileChooser = std::make_unique<juce::FileChooser>(
        "Carpeta para exportar samples",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*");
    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectDirectories,
        [this, files](const juce::FileChooser& chooser)
        {
            auto folder = chooser.getResult();
            if (!folder.isDirectory())
                return;
            auto copied = 0;
            for (const auto& file : files)
                if (file.copyFileTo(folder.getChildFile(file.getFileName())))
                    ++copied;
            sampleInfo.setText("Exportados " + juce::String(copied) + " samples.",
                               juce::dontSendNotification);
        });
}

void MiguelMusicAssistantAudioProcessorEditor::applyWindowChrome()
{
#if JucePlugin_Build_Standalone
    if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
    {
        window->setName("PICHADAW");
        window->setUsingNativeTitleBar(true);
        window->setTitleBarButtonsRequired(juce::DocumentWindow::allButtons, false);
    }
#endif
}

void MiguelMusicAssistantAudioProcessorEditor::maximiseWindow()
{
    if (auto* window = findParentComponentOfClass<juce::ResizableWindow>())
        window->setFullScreen(true);
}

void MiguelMusicAssistantAudioProcessorEditor::restoreWindow()
{
    if (auto* window = findParentComponentOfClass<juce::ResizableWindow>())
        window->setFullScreen(false);
}
