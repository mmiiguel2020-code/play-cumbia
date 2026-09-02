#include "BajxterFxProcessor.h"
#include "BajxterFxEditor.h"

#ifndef BAJXTER_FX_SLOT
#define BAJXTER_FX_SLOT 2
#endif

BajxterFxAudioProcessor::BajxterFxAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("In", juce::AudioChannelSet::stereo(), true)
          .withOutput("Out", juce::AudioChannelSet::stereo(), true)),
      slot(static_cast<FxSlot>(BAJXTER_FX_SLOT))
{
    rack.setAmount(slot, slot == FxSlot::volume ? 1.0f
        : slot == FxSlot::velocity ? 0.49f
        : slot == FxSlot::efecto ? 1.0f : 0.35f);
}

void BajxterFxAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    rack.prepare(sampleRate, samplesPerBlock);
}

bool BajxterFxAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return (output == juce::AudioChannelSet::mono()
            || output == juce::AudioChannelSet::stereo())
        && output == layouts.getMainInputChannelSet();
}

void BajxterFxAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    rack.processOnly(slot, buffer);
}

juce::AudioProcessorEditor* BajxterFxAudioProcessor::createEditor()
{
    return new BajxterFxEditor(*this);
}

void BajxterFxAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = rack.toTree().createXml())
        copyXmlToBinary(*xml, destData);
}

void BajxterFxAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        rack.fromTree(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BajxterFxAudioProcessor();
}
