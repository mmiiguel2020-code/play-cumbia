#include "PrecisionRotarySlider.h"

PrecisionRotarySlider::PrecisionRotarySlider()
{
    setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                        juce::MathConstants<float>::pi * 2.8f, true);
    setMouseDragSensitivity(2500);
    setVelocityBasedMode(false);
    setTextBoxStyle(juce::Slider::TextBoxBelow, false, 82, 22);
}
