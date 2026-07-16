#pragma once

#include "GraphicEqDisplay.h"
#include "ImportedSampleList.h"
#include "MiguelLookAndFeel.h"
#include "MixAnalyzerComponent.h"
#include "MusicGenerator.h"
#include "PianoRollView.h"
#include "PluginProcessor.h"
#include "PrecisionRotarySlider.h"

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

class TabPagePanel final : public juce::Component
{
public:
    explicit TabPagePanel(juce::Colour accentColourToUse = MiguelColours::cyan())
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

class MiguelMusicAssistantAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer,
      private juce::MidiKeyboardStateListener
{
public:
    explicit MiguelMusicAssistantAudioProcessorEditor(
        MiguelMusicAssistantAudioProcessor&);
    ~MiguelMusicAssistantAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void syncSessionStateToProcessor();

private:
    void timerCallback() override;
    GenerationSettings currentSettings() const;
    void exportMidi();
    void chooseSampleFiles();
    void updateSelectedSample(const juce::File&);
    void analyseSelectedSamplePitch();
    void updateTunerDisplay();
    void processTunedSample(bool exportFile);
    void generateBajoquintoChords();
    void previewBajoquintoChord();
    juce::File selectedBajoquintoChord() const;
    juce::File bajoquintoOutputFolder() const;
    void updateBajoquintoStatus();
    void chooseRhythmSample(int channel);
    juce::File selectedDrumLibrary() const;
    void exportRhythmLoop();
    void prepareBroncoPiano();
    void exportPianoRecording();
    AudioSection selectedEqSection() const;
    void updateEqControls();
    void updateFoldVisibility();
    void refreshRhythmGridFromEngine();
    juce::ValueTree captureUiSessionState() const;
    void restoreUiSessionState(const juce::ValueTree& uiState);
    void handleNoteOn(
        juce::MidiKeyboardState*, int, int midiNote, float velocity) override;
    void handleNoteOff(
        juce::MidiKeyboardState*, int, int midiNote, float velocity) override;

    MiguelMusicAssistantAudioProcessor& processor;
    MiguelLookAndFeel miguelLookAndFeel;

    juce::TabbedComponent tabs{ juce::TabbedButtonBar::TabsAtTop };
    TabPagePanel generatorPage{ MiguelColours::cyan() };
    TabPagePanel mixPage{ MiguelColours::green() };
    TabPagePanel libraryPage{ MiguelColours::orange() };
    TabPagePanel bajoquintoPage{ MiguelColours::purple() };
    TabPagePanel studioPage{ MiguelColours::pink() };
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
    ImportedSampleList importedSampleList;
    juce::TextButton folderButton{ "Agregar samples..." };
    juce::TextButton removeSampleButton{ "Quitar seleccionado" };
    juce::Label folderLabel;
    SampleWaveform sampleWaveform;
    juce::TextButton samplePlayButton{ "Escuchar" };
    juce::TextButton sampleStopButton{ "Detener" };
    juce::TextButton sampleDragButton{ "Mostrar en Explorador" };
    juce::Label sampleInfo;
    juce::Label tunerTitle;
    juce::Label tunerReadout;
    TunerNeedle tunerNeedle;
    PrecisionRotarySlider pitchShiftKnob;
    juce::Label pitchShiftLabel;
    PrecisionRotarySlider broncoMaxKnob;
    juce::Label broncoMaxLabel;
    juce::TextButton analysePitchButton{ "Analizar tono" };
    juce::TextButton auditionTunedButton{ "Escuchar ajustado" };
    juce::TextButton exportTunedButton{ "Exportar afinado" };
    juce::File selectedSample;
    juce::ThreadPool pitchPool{ 1 };
    std::atomic<juce::uint64> pitchRequest{ 0 };
    bool hasDetectedPitch = false;
    double detectedFrequency = 0.0;
    double detectedMidiExact = 0.0;

    juce::Label bajoquintoTitle;
    juce::Label bajoquintoDescription;
    juce::ComboBox bajoquintoStyleBox;
    juce::ComboBox chordRootBox;
    juce::ComboBox chordQualityBox;
    juce::ComboBox chordVoicingBox;
    juce::Label bajoquintoStyleLabel;
    juce::Label chordRootLabel;
    juce::Label chordQualityLabel;
    juce::Label chordVoicingLabel;
    juce::TextButton generateChordsButton{ "Generar 24 acordes" };
    juce::TextButton previewChordButton{ "Escuchar acorde" };
    juce::TextButton stopChordButton{ "Detener" };
    juce::TextButton openToneInputButton{ "Abrir entrada WAV" };
    juce::TextButton openChordsButton{ "Abrir carpeta" };
    juce::Label toneInputLabel;
    juce::Label bajoquintoStatus;
    juce::ThreadPool chordPool{ 1 };
    std::atomic<juce::uint64> chordRequest{ 0 };
    juce::ChildProcess chordProcess;

    juce::Label studioTitle;
    juce::Label rhythmTitle;
    juce::TextButton rhythmFoldButton{ "[v] Ritmos / Piano Roll" };
    juce::Slider rhythmBpmSlider;
    juce::Label rhythmBpmLabel;
    juce::ComboBox loopLengthBox;
    juce::Label loopLengthLabel;
    juce::ComboBox exportBarsBox;
    juce::Label exportBarsLabel;
    juce::TextButton rhythmPlayButton{ "Play Loop" };
    juce::TextButton rhythmStopButton{ "Stop" };
    juce::TextButton rhythmClearButton{ "Limpiar" };
    juce::TextButton rhythmExportButton{ "Exportar Loop" };
    juce::ComboBox drumLibraryBox;
    juce::Label drumLibraryLabel;
    juce::TextButton openDrumLibraryButton{ "Abrir biblioteca" };
    std::array<juce::TextButton, GrooveEngine::channelCount>
        rhythmLoadButtons;
    std::array<juce::TextButton, GrooveEngine::channelCount>
        rhythmEqButtons;
    std::array<juce::Label, GrooveEngine::channelCount> rhythmChannelLabels;
    std::array<juce::Slider, GrooveEngine::channelCount> rhythmGainSliders;
    std::array<std::array<juce::TextButton, GrooveEngine::stepCount>,
               GrooveEngine::channelCount> rhythmSteps;
    juce::Viewport rhythmGridViewport;
    juce::Component rhythmGridContent;
    int previousPlayheadStep = -1;

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
    bool rhythmExpanded = true;
    bool pianoExpanded = true;
    bool inputEqExpanded = true;
    bool outputEqExpanded = true;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        MiguelMusicAssistantAudioProcessorEditor)
};
