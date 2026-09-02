#include "BajxterFxEditor.h"

juce::String BajxterFxEditor::slotTitle(FxSlot slot)
{
    switch (slot)
    {
        case FxSlot::filterHp:    return "Filtro HP";
        case FxSlot::filterLp:    return "Filtro LP";
        case FxSlot::compressor:  return "Compresor";
        case FxSlot::exciter:     return "Excitador";
        case FxSlot::doubler:     return "Duplicador";
        case FxSlot::distortion:  return "Distorsion";
        case FxSlot::delay:       return "Delay";
        case FxSlot::efecto:      return "Reverb";
        case FxSlot::volume:      return "Volumen";
        case FxSlot::velocity:    return "Velocity";
        default:                  return "FX";
    }
}

juce::Colour BajxterFxEditor::slotColour(FxSlot slot)
{
    switch (slot)
    {
        case FxSlot::filterHp:
        case FxSlot::filterLp:    return MiguelColours::cyan();
        case FxSlot::compressor:  return MiguelColours::green();
        case FxSlot::exciter:     return MiguelColours::yellow();
        case FxSlot::doubler:     return MiguelColours::purple();
        case FxSlot::distortion:  return MiguelColours::orange();
        case FxSlot::delay:       return MiguelColours::pink();
        case FxSlot::efecto:      return MiguelColours::purple();
        case FxSlot::volume:      return MiguelColours::green();
        case FxSlot::velocity:    return MiguelColours::orange();
        default:                  return MiguelColours::cyan();
    }
}

BajxterFxEditor::BajxterFxEditor(BajxterFxAudioProcessor& processorToUse)
    : AudioProcessorEditor(&processorToUse),
      processor(processorToUse)
{
    setLookAndFeel(&lookAndFeel);
    setSize(280, 340);
    addAndMakeVisible(title);
    addAndMakeVisible(knob);
    addAndMakeVisible(valueLabel);
    title.setText(slotTitle(processor.getSlot()), juce::dontSendNotification);
    title.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    title.setJustificationType(juce::Justification::centred);
    title.setColour(juce::Label::textColourId, MiguelColours::text());
    knob.setRange(0.0, 100.0, 1.0);
    knob.setTextValueSuffix(" %");
    knob.setColour(juce::Slider::rotarySliderFillColourId,
                   slotColour(processor.getSlot()));
    knob.setValue(processor.getRack().getAmount(processor.getSlot()) * 100.0,
                  juce::dontSendNotification);
    knob.onValueChange = [this]
    {
        processor.getRack().setAmount(
            processor.getSlot(),
            static_cast<float>(knob.getValue() / 100.0));
    };
    valueLabel.setJustificationType(juce::Justification::centred);
    valueLabel.setColour(juce::Label::textColourId, MiguelColours::textMuted());
    startTimerHz(20);
}

BajxterFxEditor::~BajxterFxEditor()
{
    setLookAndFeel(nullptr);
}

void BajxterFxEditor::paint(juce::Graphics& g)
{
    g.fillAll(MiguelColours::background());
    g.setColour(MiguelColours::orange());
    g.fillRect(0, 0, getWidth(), 3);
}

void BajxterFxEditor::resized()
{
    auto bounds = getLocalBounds().reduced(18);
    title.setBounds(bounds.removeFromTop(36));
    valueLabel.setBounds(bounds.removeFromBottom(28));
    knob.setBounds(bounds.reduced(12));
}

void BajxterFxEditor::timerCallback()
{
    valueLabel.setText(
        juce::String(processor.getRack().getLed(processor.getSlot()) * 100.0, 0)
            + " senal",
        juce::dontSendNotification);
}
