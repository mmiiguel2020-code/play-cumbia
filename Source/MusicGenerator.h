#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

struct GenerationSettings
{
    int rootPitchClass = 0;
    bool minor = false;
    int bars = 4;
    double bpm = 120.0;
    int humanizePercent = 8;
};

class MusicGenerator
{
public:
    static juce::MidiFile createSong(const GenerationSettings& settings);
    static bool writeToFile(const juce::File& destination,
                            const GenerationSettings& settings);
    static juce::String noteName(int pitchClass);

private:
    static std::vector<int> scaleFor(bool minor);
};
