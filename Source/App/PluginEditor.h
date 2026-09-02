#pragma once

#include "GraphicEqDisplay.h"
#include "ImportedSampleList.h"
#include "MiguelLookAndFeel.h"
#include "MixAnalyzerComponent.h"
#include "MusicGenerator.h"
#include "PianoRollView.h"
#include "PluginProcessor.h"
#include "PrecisionRotarySlider.h"
#include "RackPanel.h"
#include "ReferencePiano.h"

#include <array>
#include <functional>
#include <juce_audio_utils/juce_audio_utils.h>

class SampleWaveform final : public juce::Component,
                             private juce::ChangeListener
{
public:
    SampleWaveform();
    ~SampleWaveform() override;
    void setFile(const juce::File&);
    void paint(juce::Graphics&) override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override { repaint(); }

    juce::AudioFormatManager formats;
    juce::AudioThumbnailCache cache{ 8 };
    juce::AudioThumbnail thumbnail{ 512, formats, cache };
    juce::String fileName;
};

class TunerNeedle final : public juce::Component
{
public:
    void setReading(double centsToUse, const juce::String& noteToUse,
                    double frequencyToUse);
    void clear();
    void paint(juce::Graphics&) override;

private:
    double cents = 0.0;
    double frequency = 0.0;
    juce::String note{ "-" };
    bool hasReading = false;
};

class PichaBrandBar final : public juce::Component
{
public:
    PichaBrandBar() { setInterceptsMouseClicks(false, false); }
    void paint(juce::Graphics&) override;
};

class TabPagePanel final : public juce::Component
{
public:
    explicit TabPagePanel(juce::Colour accentColourToUse = MiguelColours::blue())
        : accentColour(accentColourToUse)
    {
        setOpaque(true);
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        juce::ColourGradient background(
            MiguelColours::panelRaised(), bounds.getCentreX(), bounds.getY(),
            MiguelColours::panel(), bounds.getCentreX(), bounds.getBottom(),
            false);
        g.setGradientFill(background);
        g.fillAll();
        g.setColour(accentColour);
        g.fillRect(bounds.getX(), bounds.getY(), bounds.getWidth(), 2.0f);
    }

private:
    juce::Colour accentColour;
};

class PluginCell final : public juce::Component
{
public:
    explicit PluginCell(juce::String titleToUse,
                        juce::Colour accent = MiguelColours::purple())
        : title(std::move(titleToUse)), accentColour(accent)
    {
        setInterceptsMouseClicks(false, true);
    }

    juce::Rectangle<int> contentArea() const
    {
        return getLocalBounds().reduced(6, 4).withTrimmedTop(20);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        g.setColour(MiguelColours::panel());
        g.fillRoundedRectangle(bounds, 10.0f);
        g.setColour(accentColour.withAlpha(0.8f));
        g.drawRoundedRectangle(bounds, 10.0f, 1.4f);
        g.setColour(accentColour);
        g.fillRoundedRectangle(
            juce::Rectangle<float>(bounds.getX() + 8.0f, bounds.getY() + 5.0f,
                                   16.0f, 3.0f), 1.5f);
        g.setColour(MiguelColours::text());
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText(title, juce::Rectangle<float>(
                       bounds.getX() + 28.0f, bounds.getY() + 2.0f,
                       bounds.getWidth() - 36.0f, 18.0f),
                   juce::Justification::centredLeft, true);
    }

private:
    juce::String title;
    juce::Colour accentColour;
};

class SampleDropCard final : public juce::Component,
                             public juce::FileDragAndDropTarget
{
public:
    std::function<void(const juce::StringArray&)> onFilesDropped;

    void paint(juce::Graphics&) override;
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int, int) override;
};

class CapturePad final : public juce::Button
{
public:
    CapturePad();
    std::function<void()> onToggle;
    void setMode(int newMode);
    void paintButton(juce::Graphics&, bool, bool) override;
    void clicked() override;

private:
    int mode = 0;
};

class PlaySquareButton final : public juce::Button
{
public:
    PlaySquareButton() : juce::Button("play")
    {
        setWantsKeyboardFocus(false);
        setMouseClickGrabsKeyboardFocus(false);
    }

    void setPlaying(bool shouldPlay)
    {
        if (playing == shouldPlay)
            return;
        playing = shouldPlay;
        repaint();
    }

    void paintButton(juce::Graphics& g, bool over, bool down) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.5f);
        auto fill = playing ? MiguelColours::green()
                            : MiguelColours::panelRaised();
        if (down)
            fill = fill.brighter(0.14f);
        else if (over)
            fill = fill.brighter(0.08f);
        g.setColour(fill);
        g.fillRoundedRectangle(bounds, 5.0f);
        g.setColour(playing ? MiguelColours::green().darker(0.25f)
                            : MiguelColours::border());
        g.drawRoundedRectangle(bounds, 5.0f, 1.2f);

        auto triangle = bounds.reduced(bounds.getWidth() * 0.30f,
                                       bounds.getHeight() * 0.26f);
        juce::Path play;
        play.addTriangle(triangle.getX(), triangle.getY(),
                         triangle.getX(), triangle.getBottom(),
                         triangle.getRight(), triangle.getCentreY());
        g.setColour(playing ? MiguelColours::background()
                            : MiguelColours::text());
        g.fillPath(play);
    }

private:
    bool playing = false;
};

class MiguelMusicAssistantAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      public juce::MenuBarModel,
      private juce::Timer,
      private juce::MidiKeyboardStateListener,
      private juce::KeyListener
{
public:
    explicit MiguelMusicAssistantAudioProcessorEditor(
        MiguelMusicAssistantAudioProcessor&);
    ~MiguelMusicAssistantAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;
    bool keyPressed(const juce::KeyPress&) override;
    bool keyPressed(const juce::KeyPress&, juce::Component*) override;
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int, const juce::String&) override;
    void menuItemSelected(int, int) override;
    void syncSessionStateToProcessor();

private:
    void timerCallback() override;
    GenerationSettings currentSettings() const;
    void exportMidi();
    void chooseSampleFiles();
    void updateSelectedSample(const juce::File&);
    void analyseSelectedSamplePitch();
    void updateTunerDisplay();
    void chooseLayerSample(int slot);
    void applyLayerFiles(int startSlot, const juce::Array<juce::File>& files);
    void refreshLayerSlotButtons();
    void prepareBroncoPiano();
    void exportPianoRecording();
    AudioSection selectedEqSection() const;
    void updateEqControls();
    void updateFoldVisibility();
    juce::ValueTree captureUiSessionState() const;
    void restoreUiSessionState(const juce::ValueTree& uiState);
    void toggleCapturePad();
    void toggleSampleTrigger();
    void openSession();
    void saveSession(bool saveAs);
    void exportSelectedSamples();
    void applyWindowChrome();
    void maximiseWindow();
    void restoreWindow();
    void handleNoteOn(
        juce::MidiKeyboardState*, int, int midiNote, float velocity) override;
    void handleNoteOff(
        juce::MidiKeyboardState*, int, int midiNote, float velocity) override;

    MiguelMusicAssistantAudioProcessor& processor;
    MiguelLookAndFeel miguelLookAndFeel;
    juce::MenuBarComponent menuBar{ this };
    PichaBrandBar brandBar;
    juce::File sessionFile;

    CapturePad capturePad;
    TabPagePanel generatorPage{ MiguelColours::blue() };
    TabPagePanel mixPage{ MiguelColours::green() };
    TabPagePanel libraryPage{ MiguelColours::purple() };
    TabPagePanel bajoquintoPage{ MiguelColours::lilac() };
    TabPagePanel eqPage{ MiguelColours::yellow() };

    juce::ComboBox keyBox;
    juce::ComboBox modeBox;
    juce::ComboBox barsBox;
    juce::Slider bpmSlider;
    juce::Slider humanizeSlider;
    juce::Label generatorTitle;
    juce::Label keyLabel;
    juce::Label modeLabel;
    juce::Label barsLabel;
    juce::Label bpmLabel;
    juce::Label humanizeLabel;
    juce::TextButton exportButton{ "Exportar MIDI..." };
    juce::TextButton dragButton{ "Abrir carpeta MIDI" };
    juce::TextButton previewMidiButton{ "Escuchar" };
    juce::TextButton stopPreviewButton{ "Detener" };
    juce::Label generatorStatus;
    PianoRollView generatorPianoRoll;

    juce::Label levelTitle;
    juce::Label levelReadout;
    juce::Label mixSuggestion;
    MixAnalyzerComponent mixAnalyzer;

    juce::Label libraryTitle;
    PluginCell tunerCell{ "Afinador", MiguelColours::purple() };
    PluginCell eqCell{ "EQ 7 bandas", MiguelColours::blue() };
    PluginCell trimCell{ "Recortar", MiguelColours::yellow() };
    PluginCell fadeInCell{ "Entrada", MiguelColours::blue() };
    PluginCell fadeOutCell{ "Salida", MiguelColours::lilac() };
    ImportedSampleList importedSampleList;
    juce::TextButton folderButton{ "Agregar samples..." };
    juce::TextButton removeSampleButton{ "Quitar seleccionado" };
    juce::Label folderLabel;
    SampleWaveform sampleWaveform;
    PlaySquareButton samplePlayButton;
    juce::TextButton sampleLoopButton{ "Loop" };
    juce::TextButton sampleReverseButton{ "Rev" };
    juce::TextButton sampleDragButton{ "Mostrar en Explorador" };
    juce::Label sampleInfo;
    juce::Label tunerTitle;
    juce::Label tunerReadout;
    TunerNeedle tunerNeedle;
    PrecisionRotarySlider pitchShiftKnob;
    juce::Label pitchShiftLabel;
    PrecisionRotarySlider broncoMaxKnob;
    juce::Label broncoMaxLabel;
    juce::Label sampleEqTitle;
    std::array<juce::Label, MiguelMusicAssistantAudioProcessor::sampleEqBandCount>
        sampleEqBandLabels;
    std::array<FineWheelSlider, MiguelMusicAssistantAudioProcessor::sampleEqBandCount>
        sampleEqSliders;
    juce::TextButton sampleEqResetButton{ "Reset EQ" };
    PrecisionRotarySlider sampleTrimStartKnob;
    juce::Label sampleTrimStartLabel;
    PrecisionRotarySlider sampleTrimEndKnob;
    juce::Label sampleTrimEndLabel;
    PrecisionRotarySlider sampleFadeInKnob;
    juce::Label sampleFadeInLabel;
    PrecisionRotarySlider sampleFadeOutKnob;
    juce::Label sampleFadeOutLabel;
    juce::File selectedSample;
    juce::ThreadPool pitchPool{ 1 };
    std::atomic<juce::uint64> pitchRequest{ 0 };
    bool hasDetectedPitch = false;
    double detectedFrequency = 0.0;
    double detectedMidiExact = 0.0;

    juce::Label bajoquintoTitle;
    juce::Label bajoquintoDescription;
    juce::TextButton layerPlayButton{ "Escuchar" };
    juce::TextButton layerLoopButton{ "Loop" };
    juce::Label layerStatus;
    std::array<SampleDropCard, 4> layerCards;
    std::array<juce::Label, 4> layerTitles;
    std::array<juce::TextButton, 4> layerLoadButtons;
    std::array<PrecisionRotarySlider, 4>
        layerVolumeKnobs;
    std::array<PrecisionRotarySlider, 4>
        layerPitchKnobs;
    std::array<juce::Label, 4> layerVolumeLabels;
    std::array<SignalLed, 4> layerMuteLeds;
    std::array<juce::Label, 4> layerPitchLabels;

    juce::Label pianoTitle;
    juce::TextButton pianoFoldButton{ "[v] Piano Bajo Sexto Bronco" };
    juce::MidiKeyboardState pianoKeyboardState;
    juce::MidiKeyboardComponent pianoKeyboard{
        pianoKeyboardState,
        juce::MidiKeyboardComponent::horizontalKeyboard
    };
    juce::TextButton preparePianoButton{ "Preparar sonidos Bronco" };
    juce::TextButton pianoRecordButton{ "Grabar" };
    juce::TextButton pianoStopButton{ "Detener" };
    juce::TextButton pianoPlaybackButton{ "Reproducir grabación" };
    juce::TextButton pianoExportButton{ "Exportar grabación" };
    juce::Label pianoStatus;
    juce::ThreadPool studioPool{ 1 };
    juce::ChildProcess pianoGenerationProcess;

    juce::Label eqTitle;
    juce::ComboBox eqSectionBox;
    juce::Label eqSectionLabel;
    juce::Label inputEqLabel;
    juce::Label outputEqLabel;
    GraphicEqDisplay graphicEqDisplay;
    juce::TextButton inputEqFoldButton{ "[v] EQ de entrada" };
    juce::TextButton outputEqFoldButton{ "[v] EQ de salida" };
    std::array<juce::Slider, SectionEq::bandCount> inputEqSliders;
    std::array<juce::Slider, SectionEq::bandCount> outputEqSliders;
    std::array<juce::Label, SectionEq::bandCount> inputEqBandLabels;
    std::array<juce::Label, SectionEq::bandCount> outputEqBandLabels;
    PrecisionRotarySlider sectionVolumeKnob;
    juce::Label sectionVolumeLabel;
    PrecisionRotarySlider sectionBroncoKnob;
    juce::Label sectionBroncoLabel;
    juce::TextButton resetEqButton{ "Restablecer EQ" };
    RackPanel rackPanel;
    ReferencePianoComponent referencePiano;
    juce::Label referencePianoCaption;
    juce::Component* keyListenerHost = nullptr;
    bool pianoExpanded = false;
    bool inputEqExpanded = true;
    bool outputEqExpanded = true;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        MiguelMusicAssistantAudioProcessorEditor)
};
