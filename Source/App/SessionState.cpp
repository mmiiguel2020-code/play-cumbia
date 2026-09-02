#include "SessionState.h"

namespace
{
juce::ValueTree saveGrooveState(const GrooveEngine& groove)
{
    juce::ValueTree tree("GrooveEngine");
    tree.setProperty("bpm", groove.getBpm(), nullptr);
    tree.setProperty("gridResolution", groove.getGridResolution(), nullptr);
    tree.setProperty("loopLength", groove.getLoopLength(), nullptr);
    tree.setProperty("pattern", groove.getPatternData(), nullptr);

    for (int channel = 0; channel < GrooveEngine::channelCount; ++channel)
    {
        juce::ValueTree track("Track");
        track.setProperty("index", channel, nullptr);
        track.setProperty("gain", groove.getGain(channel), nullptr);
        track.setProperty("samplePath", groove.getSamplePath(channel), nullptr);
        track.setProperty("eqLow", groove.getTrackEqGain(channel, 0), nullptr);
        track.setProperty("eqMid", groove.getTrackEqGain(channel, 1), nullptr);
        track.setProperty("eqHigh", groove.getTrackEqGain(channel, 2), nullptr);
        tree.appendChild(track, nullptr);
    }
    return tree;
}

void applyGrooveState(const juce::ValueTree& tree, GrooveEngine& groove)
{
    if (!tree.isValid())
        return;

    groove.setBpm(tree.getProperty("bpm", groove.getBpm()));
    groove.setGridResolution(
        static_cast<int>(tree.getProperty("gridResolution", 8)));
    groove.setLoopLength(
        static_cast<int>(tree.getProperty("loopLength", 32)));
    groove.setPatternData(tree.getProperty("pattern", juce::String()).toString());

    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        const auto track = tree.getChild(i);
        if (!track.hasType("Track"))
            continue;
        const auto channel = static_cast<int>(track.getProperty("index", -1));
        if (!juce::isPositiveAndBelow(channel, GrooveEngine::channelCount))
            continue;

        groove.setGain(channel,
                       static_cast<float>(track.getProperty("gain", 0.85)));
        groove.setTrackEqGain(channel, 0,
            static_cast<float>(track.getProperty("eqLow", 0.0)));
        groove.setTrackEqGain(channel, 1,
            static_cast<float>(track.getProperty("eqMid", 0.0)));
        groove.setTrackEqGain(channel, 2,
            static_cast<float>(track.getProperty("eqHigh", 0.0)));

        const auto path = track.getProperty("samplePath", juce::String())
                              .toString();
        if (path.isNotEmpty())
        {
            const juce::File file(path);
            if (file.existsAsFile())
                groove.loadSample(channel, file);
        }
    }
}

juce::ValueTree saveSectionEq(const SectionEq& eq, const juce::Identifier& type)
{
    juce::ValueTree tree(type);
    tree.setProperty("volume", eq.getVolume(), nullptr);
    tree.setProperty("broncoMax", eq.getBroncoMax(), nullptr);
    for (int band = 0; band < SectionEq::bandCount; ++band)
    {
        tree.setProperty("in" + juce::String(band),
                         eq.getBandGain(false, band), nullptr);
        tree.setProperty("out" + juce::String(band),
                         eq.getBandGain(true, band), nullptr);
    }
    return tree;
}

void applySectionEq(const juce::ValueTree& tree, SectionEq& eq)
{
    if (!tree.isValid())
        return;

    eq.setVolume(static_cast<float>(tree.getProperty("volume", 1.0)));
    eq.setBroncoMax(static_cast<float>(tree.getProperty("broncoMax", 0.0)));
    for (int band = 0; band < SectionEq::bandCount; ++band)
    {
        eq.setBandGain(false, band,
            static_cast<float>(tree.getProperty(
                "in" + juce::String(band), 0.0)));
        eq.setBandGain(true, band,
            static_cast<float>(tree.getProperty(
                "out" + juce::String(band), 0.0)));
    }
}

juce::ValueTree saveEqBank(const SectionEqBank& bank)
{
    juce::ValueTree tree("SectionEqBank");
    static constexpr std::array<const char*, 5> names{
        "Generator", "Samples", "Chords", "Rhythms", "Piano"
    };
    for (size_t section = 0;
         section < static_cast<size_t>(AudioSection::count); ++section)
        tree.appendChild(
            saveSectionEq(bank.get(static_cast<AudioSection>(section)),
                          names[section]),
            nullptr);
    return tree;
}

void applyEqBank(const juce::ValueTree& tree, SectionEqBank& bank)
{
    static constexpr std::array<const char*, 5> names{
        "Generator", "Samples", "Chords", "Rhythms", "Piano"
    };
    for (size_t section = 0;
         section < static_cast<size_t>(AudioSection::count); ++section)
    {
        const auto child = tree.getChildWithName(names[section]);
        applySectionEq(child, bank.get(static_cast<AudioSection>(section)));
    }
}
}

namespace SessionState
{
juce::ValueTree buildFromEngines(const GrooveEngine& groove,
                                 const SectionEqBank& eqBank,
                                 const juce::ValueTree& uiState,
                                 const juce::ValueTree& fxState)
{
    juce::ValueTree session(rootType);
    session.setProperty(versionProperty, currentVersion, nullptr);
    session.appendChild(saveGrooveState(groove), nullptr);
    session.appendChild(saveEqBank(eqBank), nullptr);
    if (fxState.isValid())
        session.appendChild(fxState.createCopy(), nullptr);
    if (uiState.isValid())
        session.appendChild(uiState.createCopy(), nullptr);
    return session;
}

void applyToEngines(const juce::ValueTree& session,
                    GrooveEngine& groove,
                    SectionEqBank& eqBank)
{
    if (!session.hasType(rootType))
        return;

    applyGrooveState(session.getChildWithName("GrooveEngine"), groove);
    applyEqBank(session.getChildWithName("SectionEqBank"), eqBank);
}

juce::File autosaveFile()
{
    return juce::File::getSpecialLocation(
               juce::File::userApplicationDataDirectory)
        .getChildFile("Miguel Music Assistant")
        .getChildFile("Sessions")
        .getChildFile("autosave.mmas");
}

bool writeAutosave(const juce::ValueTree& session)
{
    const auto file = autosaveFile();
    if (!file.getParentDirectory().createDirectory())
        return false;
    if (const auto xml = session.createXml())
        return xml->writeTo(file);
    return false;
}

juce::ValueTree readAutosave()
{
    const auto file = autosaveFile();
    if (!file.existsAsFile())
        return {};
    if (const auto xml = juce::parseXML(file))
        return juce::ValueTree::fromXml(*xml);
    return {};
}
}
