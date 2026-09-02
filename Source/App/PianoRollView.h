#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

class PianoRollView final : public juce::Component
{
public:
    PianoRollView() = default;
    void setMidiFile(const juce::MidiFile&, int bars);
    void setPlayheadProgress(double progress);
    void paint(juce::Graphics&) override;
    void mouseWheelMove(const juce::MouseEvent&,
                        const juce::MouseWheelDetails&) override;

private:
    struct Note
    {
        double startBeat = 0.0;
        double lengthBeats = 0.0;
        int pitch = 60;
        int channel = 1;
    };

    std::vector<Note> notes;
    int totalBars = 4;
    int lowestPitch = 40;
    int highestPitch = 84;
    float pixelsPerBeat = 60.0f;
    float verticalOffset = 0.0f;
    float horizontalOffset = 0.0f;
    double playheadProgress = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollView)
};
