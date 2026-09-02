#include "ReferencePiano.h"
#include "MiguelLookAndFeel.h"

namespace
{
bool isBlack(int midiNote)
{
    return juce::MidiMessage::isMidiNoteBlack(midiNote);
}

juce::String scientificName(int midiNote)
{
    static const char* names[] = {
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };
    const auto octave = midiNote / 12 - 1;
    return juce::String(names[midiNote % 12]) + juce::String(octave);
}
}

ReferencePianoComponent::ReferencePianoComponent(juce::MidiKeyboardState& stateToUse)
    : state(stateToUse)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(false);
}

int ReferencePianoComponent::keyHeight()
{
    if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        const auto scale = juce::jmax(0.5, display->scale);
        return juce::jlimit(
            52, 72,
            juce::roundToInt((2.0 / 3.0) * display->dpi / scale));
    }
    return 64;
}

int ReferencePianoComponent::whiteKeyCount() const
{
    int count = 0;
    for (int note = firstMidi; note <= lastMidi; ++note)
        if (!isBlack(note))
            ++count;
    return count;
}

int ReferencePianoComponent::whiteIndexOf(int midiNote) const
{
    int index = 0;
    for (int note = firstMidi; note <= lastMidi; ++note)
    {
        if (isBlack(note))
            continue;
        if (note == midiNote)
            return index;
        ++index;
    }
    return -1;
}

juce::Rectangle<float> ReferencePianoComponent::whiteBounds(int midiNote) const
{
    const auto index = whiteIndexOf(midiNote);
    if (index < 0)
        return {};
    const auto count = juce::jmax(1, whiteKeyCount());
    const auto width = getWidth() / static_cast<float>(count);
    return { index * width, 0.0f, width, static_cast<float>(getHeight()) };
}

juce::Rectangle<float> ReferencePianoComponent::blackBounds(int midiNote) const
{
    if (!isBlack(midiNote))
        return {};
    const auto leftWhite = whiteBounds(midiNote - 1);
    if (leftWhite.isEmpty())
        return {};
    const auto blackW = leftWhite.getWidth() * 0.58f;
    const auto blackH = getHeight() * 0.62f;
    return { leftWhite.getRight() - blackW * 0.5f, 0.0f, blackW, blackH };
}

int ReferencePianoComponent::noteAt(juce::Point<float> pos) const
{
    for (int note = firstMidi; note <= lastMidi; ++note)
        if (isBlack(note) && blackBounds(note).contains(pos))
            return note;

    for (int note = firstMidi; note <= lastMidi; ++note)
        if (!isBlack(note) && whiteBounds(note).contains(pos))
            return note;

    return -1;
}

void ReferencePianoComponent::playNote(int midiNote)
{
    if (midiNote == heldNote)
        return;
    releaseNote();
    if (midiNote < firstMidi || midiNote > lastMidi)
        return;
    heldNote = midiNote;
    state.noteOn(1, midiNote, 0.85f);
    repaint();
}

void ReferencePianoComponent::releaseNote()
{
    if (heldNote < 0)
        return;
    state.noteOff(1, heldNote, 0.0f);
    heldNote = -1;
    repaint();
}

void ReferencePianoComponent::paint(juce::Graphics& graphics)
{
    auto bounds = getLocalBounds().toFloat();
    graphics.setColour(MiguelColours::panelRaised());
    graphics.fillRoundedRectangle(bounds, 3.0f);

    for (int note = firstMidi; note <= lastMidi; ++note)
    {
        if (isBlack(note))
            continue;
        auto key = whiteBounds(note);
        const auto down = state.isNoteOn(1, note) || note == heldNote;
        graphics.setColour(down ? MiguelColours::lilac()
                                : juce::Colour(0xfff4eefc));
        graphics.fillRect(key.reduced(0.4f, 0.0f));
        graphics.setColour(MiguelColours::border().withAlpha(0.55f));
        graphics.drawRect(key, 0.7f);

        const auto pitchClass = note % 12;
        if (pitchClass == 0 || note == 69)
        {
            graphics.setColour(MiguelColours::textMuted());
            graphics.setFont(juce::FontOptions(10.0f, juce::Font::bold));
            const auto label = note == 69 ? juce::String("A4")
                                          : scientificName(note);
            graphics.drawText(label, key.removeFromBottom(14.0f),
                              juce::Justification::centred, false);
        }
    }

    for (int note = firstMidi; note <= lastMidi; ++note)
    {
        if (!isBlack(note))
            continue;
        auto key = blackBounds(note);
        const auto down = state.isNoteOn(1, note) || note == heldNote;
        graphics.setColour(down ? MiguelColours::purple()
                                : juce::Colour(0xff191a20));
        graphics.fillRoundedRectangle(key, 1.5f);
    }
}

void ReferencePianoComponent::mouseDown(const juce::MouseEvent& event)
{
    playNote(noteAt(event.position));
}

void ReferencePianoComponent::mouseDrag(const juce::MouseEvent& event)
{
    playNote(noteAt(event.position));
}

void ReferencePianoComponent::mouseUp(const juce::MouseEvent&)
{
    releaseNote();
}

void ReferencePianoComponent::mouseExit(const juce::MouseEvent& event)
{
    if (!event.mods.isLeftButtonDown())
        releaseNote();
}
