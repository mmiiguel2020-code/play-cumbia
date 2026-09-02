#include "BajxterEqProcessor.h"
#include "BajxterEqEditor.h"

BajxterEqAudioProcessor::BajxterEqAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("In", juce::AudioChannelSet::stereo(), true)
          .withOutput("Out", juce::AudioChannelSet::stereo(), true))
{
}

void BajxterEqAudioProcessor::prepareToPlay(double sampleRate, int)
{
    const juce::ScopedLock sl(lock);
    activeSampleRate = juce::jmax(8000.0, sampleRate);
    updateFilters();
    for (auto& channel : filters)
        for (auto& filter : channel)
            filter.reset();
}

bool BajxterEqAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return (output == juce::AudioChannelSet::mono()
            || output == juce::AudioChannelSet::stereo())
        && output == layouts.getMainInputChannelSet();
}

void BajxterEqAudioProcessor::updateFilters()
{
    const std::array<juce::IIRCoefficients, 3> coefficients{
        juce::IIRCoefficients::makeLowShelf(
            activeSampleRate, 140.0, 0.707,
            juce::Decibels::decibelsToGain(gains[0])),
        juce::IIRCoefficients::makePeakFilter(
            activeSampleRate, 1000.0, 0.9,
            juce::Decibels::decibelsToGain(gains[1])),
        juce::IIRCoefficients::makeHighShelf(
            activeSampleRate, 7000.0, 0.707,
            juce::Decibels::decibelsToGain(gains[2]))
    };
    for (auto& channel : filters)
        for (int band = 0; band < 3; ++band)
            channel[static_cast<size_t>(band)].setCoefficients(
                coefficients[static_cast<size_t>(band)]);
}

void BajxterEqAudioProcessor::setBandDb(int band, float decibels)
{
    if (!juce::isPositiveAndBelow(band, 3))
        return;
    const juce::ScopedLock sl(lock);
    gains[static_cast<size_t>(band)] = juce::jlimit(-18.0f, 18.0f, decibels);
    updateFilters();
}

float BajxterEqAudioProcessor::getBandDb(int band) const
{
    if (!juce::isPositiveAndBelow(band, 3))
        return 0.0f;
    const juce::ScopedLock sl(lock);
    return gains[static_cast<size_t>(band)];
}

void BajxterEqAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const juce::ScopedLock sl(lock);
    const auto channels = juce::jmin(2, buffer.getNumChannels());
    const auto samples = buffer.getNumSamples();
    for (int ch = 0; ch < channels; ++ch)
        for (int band = 0; band < 3; ++band)
            filters[static_cast<size_t>(ch)][static_cast<size_t>(band)]
                .processSamples(buffer.getWritePointer(ch), samples);
}

juce::AudioProcessorEditor* BajxterEqAudioProcessor::createEditor()
{
    return new BajxterEqEditor(*this);
}

void BajxterEqAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ValueTree tree("Eq");
    tree.setProperty("low", getBandDb(0), nullptr);
    tree.setProperty("mid", getBandDb(1), nullptr);
    tree.setProperty("high", getBandDb(2), nullptr);
    if (auto xml = tree.createXml())
        copyXmlToBinary(*xml, destData);
}

void BajxterEqAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        const auto tree = juce::ValueTree::fromXml(*xml);
        setBandDb(0, static_cast<float>(tree.getProperty("low", 0.0)));
        setBandDb(1, static_cast<float>(tree.getProperty("mid", 0.0)));
        setBandDb(2, static_cast<float>(tree.getProperty("high", 0.0)));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BajxterEqAudioProcessor();
}
