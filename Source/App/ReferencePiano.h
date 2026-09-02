#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

/** Teclado de referencia: C4–C7 (octavas 4, 5 y 6). Tono científico, no FL. */
class ReferencePianoComponent final : public juce::Component
{
public:
    static constexpr int firstMidi = 60; // C4 = 261.63 Hz
    static constexpr int lastMidi = 96;  // C7

    explicit ReferencePianoComponent(juce::MidiKeyboardState& stateToUse);

    static int keyHeight();

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;

private:
    int noteAt(juce::Point<float> pos) const;
    juce::Rectangle<float> whiteBounds(int midiNote) const;
    juce::Rectangle<float> blackBounds(int midiNote) const;
    int whiteIndexOf(int midiNote) const;
    int whiteKeyCount() const;
    void playNote(int midiNote);
    void releaseNote();

    juce::MidiKeyboardState& state;
    int heldNote = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReferencePianoComponent)
};
