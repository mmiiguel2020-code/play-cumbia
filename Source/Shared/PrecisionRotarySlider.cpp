#include "PrecisionRotarySlider.h"

#include <cmath>

void FineWheelSlider::setWheelStepMultiplier(double multiplier)
{
    wheelStepMultiplier = juce::jmax(0.01, multiplier);
}

void FineWheelSlider::mouseWheelMove(
    const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (!isEnabled() || !isScrollWheelEnabled())
        return;
    const auto dy = std::abs(wheel.deltaY) >= std::abs(wheel.deltaX)
        ? wheel.deltaY : -wheel.deltaX;
    const auto amount = dy * (wheel.isReversed ? -1.0f : 1.0f);
    if (std::abs(amount) < 1.0e-6f)
        return;
    const auto step = (getInterval() > 0.0 ? getInterval() : 0.01)
        * wheelStepMultiplier;
    setValue(juce::jlimit(getMinimum(), getMaximum(),
                          getValue() + (amount > 0.0f ? step : -step)),
             juce::sendNotificationSync);
}

PrecisionRotarySlider::PrecisionRotarySlider()
{
    setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                        juce::MathConstants<float>::pi * 2.8f, true);
    setMouseDragSensitivity(500);
    setWheelStepMultiplier(5.0);
    setVelocityBasedMode(false);
    setTextBoxStyle(juce::Slider::TextBoxBelow, false, 82, 22);
}

void PrecisionRotarySlider::setHuge(bool huge)
{
    if (huge)
    {
        setTextBoxStyle(juce::Slider::TextBoxBelow, false, 96, 28);
        setMouseDragSensitivity(360);
    }
    else
    {
        setTextBoxStyle(juce::Slider::TextBoxBelow, false, 82, 22);
        setMouseDragSensitivity(500);
    }
}
