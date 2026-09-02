#include "MusicGenerator.h"

#include <random>

namespace
{
constexpr int ticksPerQuarterNote = 960;

void addNote(juce::MidiMessageSequence& track, int channel, int note,
             double startBeat, double lengthBeats, juce::uint8 velocity)
{
    track.addEvent(juce::MidiMessage::noteOn(channel, note, velocity),
                   startBeat * ticksPerQuarterNote);
    track.addEvent(juce::MidiMessage::noteOff(channel, note),
                   (startBeat + lengthBeats) * ticksPerQuarterNote);
}
}

std::vector<int> MusicGenerator::scaleFor(bool minor)
{
    return minor ? std::vector<int>{ 0, 2, 3, 5, 7, 8, 10 }
                 : std::vector<int>{ 0, 2, 4, 5, 7, 9, 11 };
}

juce::String MusicGenerator::noteName(int pitchClass)
{
    static const juce::StringArray names{
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };
    return names[juce::jlimit(0, 11, pitchClass)];
}

juce::MidiFile MusicGenerator::createSong(const GenerationSettings& settings)
{
    juce::MidiFile file;
    file.setTicksPerQuarterNote(ticksPerQuarterNote);

    juce::MidiMessageSequence tempoTrack;
    tempoTrack.addEvent(juce::MidiMessage::tempoMetaEvent(
        static_cast<int>(60000000.0 / juce::jmax(40.0, settings.bpm))), 0.0);
    tempoTrack.addEvent(juce::MidiMessage::timeSignatureMetaEvent(4, 4), 0.0);
    file.addTrack(tempoTrack);

    juce::MidiMessageSequence chords;
    juce::MidiMessageSequence melody;
    const auto scale = scaleFor(settings.minor);
    const std::array<int, 4> progression = settings.minor
        ? std::array<int, 4>{ 0, 5, 2, 6 }
        : std::array<int, 4>{ 0, 4, 5, 3 };

    std::mt19937 random{ std::random_device{}() };
    std::uniform_int_distribution<int> scaleChoice(0, 6);
    std::uniform_int_distribution<int> velocityChoice(-10, 10);
    std::uniform_real_distribution<double> timingChoice(-1.0, 1.0);

    for (int bar = 0; bar < settings.bars; ++bar)
    {
        const auto degree = progression[static_cast<size_t>(bar % progression.size())];
        const auto root = 48 + settings.rootPitchClass + scale[static_cast<size_t>(degree)];
        const auto thirdDegree = (degree + 2) % 7;
        const auto fifthDegree = (degree + 4) % 7;
        const auto third = 48 + settings.rootPitchClass
            + scale[static_cast<size_t>(thirdDegree)] + (thirdDegree < degree ? 12 : 0);
        const auto fifth = 48 + settings.rootPitchClass
            + scale[static_cast<size_t>(fifthDegree)] + (fifthDegree < degree ? 12 : 0);

        addNote(chords, 1, root, bar * 4.0, 3.8, 82);
        addNote(chords, 1, third, bar * 4.0, 3.8, 76);
        addNote(chords, 1, fifth, bar * 4.0, 3.8, 72);

        for (int step = 0; step < 8; ++step)
        {
            const auto chosenDegree = step == 0 ? degree : scaleChoice(random);
            const auto octave = step % 4 == 3 ? 12 : 0;
            const auto note = 60 + settings.rootPitchClass
                + scale[static_cast<size_t>(chosenDegree)] + octave;
            const auto humanize = settings.humanizePercent / 100.0;
            const auto start = bar * 4.0 + step * 0.5
                + timingChoice(random) * 0.025 * humanize;
            const auto velocity = static_cast<juce::uint8>(
                juce::jlimit(45, 115, 86 + velocityChoice(random)));
            addNote(melody, 2, note, start, 0.42, velocity);
        }
    }

    chords.updateMatchedPairs();
    melody.updateMatchedPairs();
    file.addTrack(chords);
    file.addTrack(melody);
    return file;
}

bool MusicGenerator::writeToFile(const juce::File& destination,
                                 const GenerationSettings& settings)
{
    destination.deleteFile();
    auto stream = destination.createOutputStream();
    return stream != nullptr && createSong(settings).writeTo(*stream);
}
