#pragma once

#include "FxRack.h"
#include "MiguelLookAndFeel.h"
#include "PrecisionRotarySlider.h"

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class SignalLed final : public juce::Component
{
public:
    void setLevel(float level01, bool muted, bool reverseGlow = false);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    std::function<void()> onClick;

private:
    float level = 0.0f;
    bool muted = false;
    bool reverseGlow = false;
};

class RackPanel final : public juce::Component
{
public:
    explicit RackPanel(FxRack& rackToUse);

    void paint(juce::Graphics&) override;
    void resized() override;
    void refreshFromRack();
    void pushLeds();
    void setCompact(bool compact);

private:
    bool compactMode = false;
    struct SlotUi
    {
        PrecisionRotarySlider knob;
        juce::Label name;
        juce::TextButton mute{ "MUTE" };
        SignalLed led;
        FxSlot slot = FxSlot::filterHp;
        juce::Rectangle<int> cellBounds;
    };

    FxRack& rack;
    juce::Label title;
    juce::Label hint;
    juce::TextButton masterMuteButton{ "MUTE MASTER" };
    SignalLed masterLed;
    std::array<SlotUi, FxRack::slotCount> slots;
};
