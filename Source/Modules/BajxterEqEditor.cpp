#include "BajxterEqEditor.h"

BajxterEqEditor::BajxterEqEditor(BajxterEqAudioProcessor& processorToUse)
    : AudioProcessorEditor(&processorToUse),
      processor(processorToUse)
{
    setLookAndFeel(&lookAndFeel);
    setSize(420, 240);
    addAndMakeVisible(title);
    title.setText("EQ", juce::dontSendNotification);
    title.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    title.setJustificationType(juce::Justification::centred);
    title.setColour(juce::Label::textColourId, MiguelColours::text());
    const char* names[] = { "LOW", "MID", "HIGH" };
    const juce::Colour colours[]{
        MiguelColours::green(), MiguelColours::cyan(), MiguelColours::orange()
    };
    for (int band = 0; band < 3; ++band)
    {
        addAndMakeVisible(labels[static_cast<size_t>(band)]);
        addAndMakeVisible(knobs[static_cast<size_t>(band)]);
        labels[static_cast<size_t>(band)].setText(names[band],
                                                  juce::dontSendNotification);
        labels[static_cast<size_t>(band)].setJustificationType(
            juce::Justification::centred);
        labels[static_cast<size_t>(band)].setColour(
            juce::Label::textColourId, MiguelColours::text());
        auto& knob = knobs[static_cast<size_t>(band)];
        knob.setRange(-18.0, 18.0, 0.1);
        knob.setTextValueSuffix(" dB");
        knob.setColour(juce::Slider::rotarySliderFillColourId, colours[band]);
        knob.setValue(processor.getBandDb(band), juce::dontSendNotification);
        knob.onValueChange = [this, band]
        {
            processor.setBandDb(
                band,
                static_cast<float>(knobs[static_cast<size_t>(band)].getValue()));
        };
    }
}

BajxterEqEditor::~BajxterEqEditor()
{
    setLookAndFeel(nullptr);
}

void BajxterEqEditor::paint(juce::Graphics& g)
{
    g.fillAll(MiguelColours::background());
    g.setColour(MiguelColours::orange());
    g.fillRect(0, 0, getWidth(), 3);
}

void BajxterEqEditor::resized()
{
    auto bounds = getLocalBounds().reduced(16);
    title.setBounds(bounds.removeFromTop(32));
    bounds.removeFromTop(8);
    const auto cellW = bounds.getWidth() / 3;
    for (int band = 0; band < 3; ++band)
    {
        auto cell = juce::Rectangle<int>(
            bounds.getX() + band * cellW, bounds.getY(),
            cellW, bounds.getHeight()).reduced(8, 0);
        labels[static_cast<size_t>(band)].setBounds(cell.removeFromTop(22));
        knobs[static_cast<size_t>(band)].setBounds(cell);
    }
}
