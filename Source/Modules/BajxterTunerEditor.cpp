#include "BajxterTunerEditor.h"

BajxterTunerEditor::BajxterTunerEditor(BajxterTunerAudioProcessor& processorToUse)
    : AudioProcessorEditor(&processorToUse),
      processor(processorToUse)
{
    setLookAndFeel(&lookAndFeel);
    setSize(360, 220);
    addAndMakeVisible(title);
    addAndMakeVisible(readout);
    title.setText("Afinador", juce::dontSendNotification);
    title.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    title.setJustificationType(juce::Justification::centred);
    title.setColour(juce::Label::textColourId, MiguelColours::text());
    readout.setJustificationType(juce::Justification::centred);
    readout.setColour(juce::Label::textColourId, MiguelColours::text());
    startTimerHz(24);
}

BajxterTunerEditor::~BajxterTunerEditor()
{
    setLookAndFeel(nullptr);
}

void BajxterTunerEditor::paint(juce::Graphics& g)
{
    g.fillAll(MiguelColours::background());
    g.setColour(MiguelColours::orange());
    g.fillRect(0, 0, getWidth(), 3);

    auto meter = juce::Rectangle<float>(20.0f, 78.0f,
                                        static_cast<float>(getWidth() - 40), 56.0f);
    g.setColour(MiguelColours::panel());
    g.fillRoundedRectangle(meter, 10.0f);
    const auto mid = meter.getCentreX();
    g.setColour(MiguelColours::border());
    g.drawLine(mid, meter.getY() + 6.0f, mid, meter.getBottom() - 6.0f, 1.2f);
    if (hasReading)
    {
        const auto x = mid + static_cast<float>(juce::jlimit(-1.0, 1.0, cents / 50.0))
            * (meter.getWidth() * 0.45f);
        g.setColour(std::abs(cents) < 4.0 ? MiguelColours::green()
                                          : MiguelColours::yellow());
        g.fillEllipse(x - 8.0f, meter.getCentreY() - 8.0f, 16.0f, 16.0f);
    }
}

void BajxterTunerEditor::resized()
{
    auto bounds = getLocalBounds().reduced(16);
    title.setBounds(bounds.removeFromTop(32));
    readout.setBounds(bounds.removeFromBottom(40));
}

void BajxterTunerEditor::timerCallback()
{
    hasReading = processor.hasReading();
    cents = processor.getCents();
    if (hasReading)
        readout.setText(
            processor.getNote() + "  /  "
                + juce::String(processor.getFrequency(), 2) + " Hz  /  "
                + juce::String(cents, 1) + " cents",
            juce::dontSendNotification);
    else
        readout.setText("Esperando senal...", juce::dontSendNotification);
    repaint();
}
