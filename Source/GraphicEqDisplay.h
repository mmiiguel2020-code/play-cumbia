#pragma once

#include "SectionEq.h"

#include <juce_gui_basics/juce_gui_basics.h>

class GraphicEqDisplay final : public juce::Component
{
public:
    using GainChanged = std::function<void(bool outputStage, int band,
                                           float decibels)>;

    GraphicEqDisplay();

    void setGains(const std::array<float, SectionEq::bandCount>& input,
                  const std::array<float, SectionEq::bandCount>& output);
    void setActiveStage(bool outputStage);
    void setGainChangedCallback(GainChanged callback);

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> graphBounds() const;
    float frequencyToX(double frequency) const;
    float gainToY(float gain) const;
    float yToGain(float y) const;
    int nearestBand(juce::Point<float>) const;
    juce::Path createCurve(
        const std::array<float, SectionEq::bandCount>& gains) const;
    void updateDraggedNode(const juce::MouseEvent&);

    std::array<float, SectionEq::bandCount> inputGains{};
    std::array<float, SectionEq::bandCount> outputGains{};
    GainChanged onGainChanged;
    int draggedBand = -1;
    bool editingOutput = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphicEqDisplay)
};
