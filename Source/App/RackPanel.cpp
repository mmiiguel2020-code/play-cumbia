#include "RackPanel.h"

namespace
{
const char* slotTitle(FxSlot slot)
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
        default:                  return "";
    }
}

juce::Colour slotColour(FxSlot slot)
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
}

void SignalLed::setLevel(float level01, bool isMuted, bool reverse)
{
    level = juce::jlimit(0.0f, 1.0f, level01);
    muted = isMuted;
    reverseGlow = reverse;
    repaint();
}

void SignalLed::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    const auto d = juce::jmin(bounds.getWidth(), bounds.getHeight());
    auto lamp = bounds.withSizeKeepingCentre(d, d);
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillEllipse(lamp.translated(0.0f, 1.5f));
    if (!muted && level < 0.04f)
    {
        g.setColour(MiguelColours::panelHighlight());
        g.fillEllipse(lamp);
        g.setColour(MiguelColours::border());
        g.drawEllipse(lamp, 1.2f);
        return;
    }
    auto colour = muted ? MiguelColours::danger()
                        : (reverseGlow ? MiguelColours::purple()
                                       : MiguelColours::green());
    if (!muted)
        colour = colour.interpolatedWith(
            MiguelColours::yellow(), juce::jlimit(0.0f, 1.0f, level));
    g.setColour(colour.withAlpha(0.22f + level * 0.78f));
    g.fillEllipse(lamp);
    g.setColour(colour.brighter(0.4f).withAlpha(0.35f + level * 0.65f));
    g.fillEllipse(lamp.reduced(d * 0.28f));
    g.setColour(MiguelColours::border());
    g.drawEllipse(lamp, 1.2f);
}

void SignalLed::mouseDown(const juce::MouseEvent&)
{
    if (onClick != nullptr)
        onClick();
}

RackPanel::RackPanel(FxRack& rackToUse)
    : rack(rackToUse)
{
    setOpaque(true);
    addAndMakeVisible(title);
    addAndMakeVisible(hint);
    addAndMakeVisible(masterMuteButton);
    addAndMakeVisible(masterLed);

    title.setText("RACK BRONCO / PERILLAS GRANDES", juce::dontSendNotification);
    title.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, MiguelColours::text());
    hint.setText("Mutea cada modulo.", juce::dontSendNotification);
    hint.setColour(juce::Label::textColourId, MiguelColours::textMuted());

    masterMuteButton.setClickingTogglesState(true);
    masterMuteButton.setColour(juce::TextButton::buttonOnColourId,
                               MiguelColours::danger());
    masterMuteButton.onClick = [this]
    {
        rack.setMasterMute(masterMuteButton.getToggleState());
    };

    for (int i = 0; i < FxRack::slotCount; ++i)
    {
        auto& ui = slots[static_cast<size_t>(i)];
        ui.slot = static_cast<FxSlot>(i);
        addAndMakeVisible(ui.knob);
        addAndMakeVisible(ui.name);
        addAndMakeVisible(ui.mute);
        addAndMakeVisible(ui.led);
        ui.knob.setHuge(true);
        ui.knob.setWheelStepMultiplier(7.5);
        ui.knob.setMouseDragSensitivity(240);
        ui.knob.setRange(0.0, 100.0, 1.0);
        ui.knob.setTextValueSuffix(" %");
        ui.knob.setColour(juce::Slider::rotarySliderFillColourId,
                          slotColour(ui.slot));
        ui.knob.setValue(rack.getAmount(ui.slot) * 100.0,
                         juce::dontSendNotification);
        ui.name.setText(slotTitle(ui.slot), juce::dontSendNotification);
        ui.name.setJustificationType(juce::Justification::centred);
        ui.name.setFont(juce::FontOptions(15.0f, juce::Font::bold));
        ui.mute.setClickingTogglesState(true);
        ui.mute.setColour(juce::TextButton::buttonOnColourId,
                          MiguelColours::danger());
        ui.knob.onValueChange = [this, slot = ui.slot]
        {
            rack.setAmount(slot, static_cast<float>(
                slots[static_cast<size_t>(slot)].knob.getValue() / 100.0));
        };
        ui.mute.onClick = [this, slot = ui.slot]
        {
            rack.setMuted(slot,
                slots[static_cast<size_t>(slot)].mute.getToggleState());
        };
    }
}

void RackPanel::resized()
{
    const auto compact = compactMode;
    auto bounds = getLocalBounds().reduced(compact ? 8 : 18, compact ? 6 : 18);
    title.setBounds(bounds.removeFromTop(compact ? 20 : 36));
    if (!compact)
    {
        hint.setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(8);
    }
    else
        bounds.removeFromTop(2);

    auto lights = bounds.removeFromTop(compact ? 28 : 48);
    masterLed.setBounds(lights.removeFromLeft(compact ? 26 : 36).reduced(2));
    masterMuteButton.setBounds(lights.removeFromLeft(compact ? 140 : 170).reduced(2));

    bounds.removeFromTop(compact ? 4 : 10);
    const auto cols = 5;
    const auto rows = 2;
    const auto cellW = juce::jmax(1, bounds.getWidth() / cols);
    const auto cellH = juce::jmax(1, bounds.getHeight() / rows);
    for (int i = 0; i < FxRack::slotCount; ++i)
    {
        auto& ui = slots[static_cast<size_t>(i)];
        const auto col = i % cols;
        const auto row = i / cols;
        auto cell = juce::Rectangle<int>(
            bounds.getX() + col * cellW,
            bounds.getY() + row * cellH,
            cellW, cellH).reduced(compact ? 3 : 8, compact ? 3 : 4);
        ui.cellBounds = cell;
        auto inner = cell.reduced(compact ? 6 : 8, compact ? 4 : 6);
        if (compact)
        {
            auto header = inner.removeFromTop(18);
            ui.led.setBounds(header.removeFromLeft(16).reduced(1));
            ui.mute.setBounds(header.removeFromRight(50).reduced(1));
            ui.name.setBounds(header);
            ui.knob.setBounds(inner.reduced(2, 0));
        }
        else
        {
            ui.name.setBounds(inner.removeFromTop(22));
            auto ledRow = inner.removeFromTop(28);
            ui.led.setBounds(ledRow.removeFromLeft(28).reduced(2));
            ui.mute.setBounds(ledRow.reduced(4, 1));
            ui.knob.setBounds(inner.reduced(6, 0));
        }
    }
}

void RackPanel::refreshFromRack()
{
    masterMuteButton.setToggleState(rack.isMasterMute(),
                                    juce::dontSendNotification);
    for (auto& ui : slots)
    {
        ui.knob.setValue(rack.getAmount(ui.slot) * 100.0,
                         juce::dontSendNotification);
        ui.mute.setToggleState(rack.isMuted(ui.slot),
                               juce::dontSendNotification);
    }
}

void RackPanel::pushLeds()
{
    masterLed.setLevel(rack.isMasterMute() ? 1.0f : rack.getOutputLed(0),
                       rack.isMasterMute());
    for (auto& ui : slots)
        ui.led.setLevel(rack.getLed(ui.slot), rack.isMuted(ui.slot));
}

void RackPanel::setCompact(bool compact)
{
    compactMode = compact;
    hint.setVisible(!compact);
    title.setText(compact ? "Rack" : "RACK BRONCO / PERILLAS GRANDES",
                  juce::dontSendNotification);
    title.setFont(juce::FontOptions(compact ? 16.0f : 26.0f, juce::Font::bold));
    for (auto& ui : slots)
    {
        ui.knob.setHuge(!compact);
        ui.knob.setWheelStepMultiplier(7.5);
        ui.knob.setMouseDragSensitivity(compact ? 333 : 240);
        if (compact)
            ui.knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
        ui.name.setFont(juce::FontOptions(compact ? 12.0f : 15.0f, juce::Font::bold));
        ui.mute.setButtonText(compact ? "MUTE" : "MUTE");
    }
    resized();
}

void RackPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(MiguelColours::background());
    g.fillAll();
    g.setColour(MiguelColours::orange());
    g.fillRect(bounds.getX(), bounds.getY(), bounds.getWidth(), 2.0f);
    for (auto& ui : slots)
    {
        auto cell = ui.cellBounds.toFloat();
        if (cell.isEmpty())
            continue;
        g.setColour(MiguelColours::panel());
        g.fillRoundedRectangle(cell, 10.0f);
        g.setColour(MiguelColours::orange().withAlpha(0.75f));
        g.drawRoundedRectangle(cell, 10.0f, 1.3f);
    }
}
