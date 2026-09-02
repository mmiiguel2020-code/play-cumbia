#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class FineWheelSlider : public juce::Slider
{
public:
    void setWheelStepMultiplier(double multiplier);
    void mouseWheelMove(const juce::MouseEvent&,
                        const juce::MouseWheelDetails&) override;

private:
    double wheelStepMultiplier = 1.0;
};

class PrecisionRotarySlider final : public FineWheelSlider
{
public:
    PrecisionRotarySlider();
    void setHuge(bool huge);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrecisionRotarySlider)
};
