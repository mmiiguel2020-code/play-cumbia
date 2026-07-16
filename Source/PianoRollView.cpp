#include "PianoRollView.h"
#include "MiguelLookAndFeel.h"

namespace
{
constexpr float keyboardWidth = 52.0f;
constexpr float noteHeight = 16.0f;
constexpr double ticksPerQuarter = 960.0;

bool isBlackKey(int note)
{
    const auto pitchClass = note % 12;
    return pitchClass == 1 || pitchClass == 3 || pitchClass == 6
        || pitchClass == 8 || pitchClass == 10;
}
}

void PianoRollView::setMidiFile(const juce::MidiFile& file, int bars)
{
    notes.clear();
    totalBars = juce::jmax(1, bars);
    lowestPitch = 127;
    highestPitch = 0;

    for (int track = 0; track < file.getNumTracks(); ++track)
    {
        const auto* sequence = file.getTrack(track);
        if (sequence == nullptr)
            continue;
        for (int eventIndex = 0; eventIndex < sequence->getNumEvents();
             ++eventIndex)
        {
            const auto* event = sequence->getEventPointer(eventIndex);
            if (!event->message.isNoteOn())
                continue;
            const auto* off = event->noteOffObject;
            const auto start = event->message.getTimeStamp() / ticksPerQuarter;
            const auto end = off != nullptr
                ? off->message.getTimeStamp() / ticksPerQuarter
                : start + 0.25;
            const auto pitch = event->message.getNoteNumber();
            notes.push_back({ start, juce::jmax(0.05, end - start),
                              pitch, event->message.getChannel() });
            lowestPitch = juce::jmin(lowestPitch, pitch);
            highestPitch = juce::jmax(highestPitch, pitch);
        }
    }

    if (notes.empty())
    {
        lowestPitch = 40;
        highestPitch = 84;
    }
    else
    {
        lowestPitch = juce::jmax(0, lowestPitch - 3);
        highestPitch = juce::jmin(127, highestPitch + 3);
    }
    horizontalOffset = 0.0f;
    verticalOffset = 0.0f;
    playheadProgress = 0.0;
    repaint();
}

void PianoRollView::setPlayheadProgress(double progress)
{
    const auto limited = juce::jlimit(0.0, 1.0, progress);
    if (std::abs(limited - playheadProgress) > 0.0001)
    {
        playheadProgress = limited;
        repaint();
    }
}

void PianoRollView::paint(juce::Graphics& graphics)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    graphics.setColour(MiguelColours::panel());
    graphics.fillRoundedRectangle(bounds, 7.0f);
    graphics.saveState();
    graphics.reduceClipRegion(bounds.toNearestInt());

    const auto grid = bounds.withTrimmedLeft(keyboardWidth);
    const auto pitchCount = highestPitch - lowestPitch + 1;
    const auto contentHeight = pitchCount * noteHeight;
    const auto totalBeats = totalBars * 4.0f;
    const auto contentWidth = totalBeats * pixelsPerBeat;
    verticalOffset = juce::jlimit(0.0f,
        juce::jmax(0.0f, contentHeight - grid.getHeight()), verticalOffset);
    horizontalOffset = juce::jlimit(0.0f,
        juce::jmax(0.0f, contentWidth - grid.getWidth()), horizontalOffset);

    for (int pitch = lowestPitch; pitch <= highestPitch; ++pitch)
    {
        const auto row = highestPitch - pitch;
        const auto y = grid.getY() + row * noteHeight - verticalOffset;
        if (y + noteHeight < grid.getY() || y > grid.getBottom())
            continue;
        graphics.setColour(isBlackKey(pitch)
                               ? juce::Colour(0xff191a20)
                               : juce::Colour(0xff242630));
        graphics.fillRect(grid.getX(), y, grid.getWidth(), noteHeight);
        graphics.setColour(MiguelColours::border().withAlpha(0.28f));
        graphics.drawHorizontalLine(juce::roundToInt(y),
                                    grid.getX(), grid.getRight());

        graphics.setColour(isBlackKey(pitch)
                               ? juce::Colour(0xff15161a)
                               : MiguelColours::textMuted());
        graphics.fillRect(bounds.getX(), y, keyboardWidth - 2.0f, noteHeight);
        if (pitch % 12 == 0)
        {
            graphics.setColour(MiguelColours::background());
            graphics.setFont(juce::FontOptions(9.0f));
            graphics.drawText("C" + juce::String(pitch / 12 - 1),
                              juce::roundToInt(bounds.getX() + 3.0f),
                              juce::roundToInt(y),
                              static_cast<int>(keyboardWidth - 6.0f),
                              static_cast<int>(noteHeight),
                              juce::Justification::centredLeft);
        }
    }

    for (int beat = 0; beat <= totalBars * 4; ++beat)
    {
        const auto x = grid.getX() + beat * pixelsPerBeat - horizontalOffset;
        if (x < grid.getX() || x > grid.getRight())
            continue;
        graphics.setColour((beat % 4 == 0 ? MiguelColours::textMuted()
                                          : MiguelColours::border())
                               .withAlpha(beat % 4 == 0 ? 0.5f : 0.3f));
        graphics.drawVerticalLine(juce::roundToInt(x),
                                  grid.getY(), grid.getBottom());
        if (beat % 4 == 0)
        {
            graphics.setColour(MiguelColours::textMuted());
            graphics.setFont(juce::FontOptions(10.0f));
            graphics.drawText(juce::String(beat / 4 + 1),
                              juce::roundToInt(x + 3.0f),
                              juce::roundToInt(grid.getY() + 2.0f),
                              24, 12, juce::Justification::centredLeft);
        }
    }

    for (const auto& note : notes)
    {
        const auto x = grid.getX()
            + static_cast<float>(note.startBeat) * pixelsPerBeat
            - horizontalOffset;
        const auto y = grid.getY() + (highestPitch - note.pitch) * noteHeight
            - verticalOffset + 2.0f;
        const auto width = juce::jmax(
            4.0f, static_cast<float>(note.lengthBeats) * pixelsPerBeat - 2.0f);
        auto noteBounds = juce::Rectangle<float>(
            x, y, width, noteHeight - 4.0f);
        if (!noteBounds.intersects(grid))
            continue;
        const auto colour = note.channel == 1 ? MiguelColours::purple()
                                              : MiguelColours::cyan();
        graphics.setColour(colour.withAlpha(0.88f));
        graphics.fillRoundedRectangle(noteBounds, 2.5f);
        graphics.setColour(colour.brighter(0.35f));
        graphics.drawRoundedRectangle(noteBounds, 2.5f, 1.0f);
    }

    const auto playheadX = grid.getX()
        + static_cast<float>(playheadProgress) * contentWidth
        - horizontalOffset;
    if (playheadX >= grid.getX() && playheadX <= grid.getRight())
    {
        graphics.setColour(MiguelColours::yellow());
        graphics.drawVerticalLine(juce::roundToInt(playheadX),
                                  grid.getY(), grid.getBottom());
    }

    graphics.restoreState();
    graphics.setColour(MiguelColours::border());
    graphics.drawRoundedRectangle(bounds, 7.0f, 1.0f);
}

void PianoRollView::mouseWheelMove(
    const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (event.mods.isCtrlDown())
    {
        pixelsPerBeat = juce::jlimit(
            24.0f, 180.0f, pixelsPerBeat + wheel.deltaY * 24.0f);
    }
    else if (event.mods.isShiftDown())
    {
        horizontalOffset -= wheel.deltaY * 160.0f;
    }
    else
    {
        verticalOffset -= wheel.deltaY * 100.0f;
        horizontalOffset -= wheel.deltaX * 160.0f;
    }
    repaint();
}
