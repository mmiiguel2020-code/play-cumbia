#include "GraphicEqDisplay.h"
#include "MiguelLookAndFeel.h"

namespace
{
constexpr float minimumGain = -24.0f;
constexpr float maximumGain = 24.0f;
constexpr double minimumFrequency = 60.0;
constexpr double maximumFrequency = 20000.0;
}

GraphicEqDisplay::GraphicEqDisplay()
{
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
}

void GraphicEqDisplay::setGains(
    const std::array<float, SectionEq::bandCount>& input,
    const std::array<float, SectionEq::bandCount>& output)
{
    inputGains = input;
    outputGains = output;
    repaint();
}

void GraphicEqDisplay::setActiveStage(bool outputStage)
{
    editingOutput = outputStage;
    repaint();
}

void GraphicEqDisplay::setGainChangedCallback(GainChanged callback)
{
    onGainChanged = std::move(callback);
}

juce::Rectangle<float> GraphicEqDisplay::graphBounds() const
{
    return getLocalBounds().toFloat().reduced(42.0f, 18.0f);
}

float GraphicEqDisplay::frequencyToX(double frequency) const
{
    const auto bounds = graphBounds();
    const auto proportion = std::log(frequency / minimumFrequency)
        / std::log(maximumFrequency / minimumFrequency);
    return bounds.getX() + static_cast<float>(proportion) * bounds.getWidth();
}

float GraphicEqDisplay::gainToY(float gain) const
{
    return juce::jmap(juce::jlimit(minimumGain, maximumGain, gain),
                      maximumGain, minimumGain,
                      graphBounds().getY(), graphBounds().getBottom());
}

float GraphicEqDisplay::yToGain(float y) const
{
    const auto bounds = graphBounds();
    return juce::jlimit(minimumGain, maximumGain,
        juce::jmap(y, bounds.getBottom(), bounds.getY(),
                   minimumGain, maximumGain));
}

juce::Path GraphicEqDisplay::createCurve(
    const std::array<float, SectionEq::bandCount>& gains) const
{
    juce::Path path;
    std::array<juce::Point<float>, SectionEq::bandCount> points;
    for (int band = 0; band < SectionEq::bandCount; ++band)
        points[static_cast<size_t>(band)] = {
            frequencyToX(SectionEq::frequencies[static_cast<size_t>(band)]),
            gainToY(gains[static_cast<size_t>(band)])
        };

    path.startNewSubPath(points.front());
    for (int band = 0; band < SectionEq::bandCount - 1; ++band)
    {
        const auto p0 = points[static_cast<size_t>(juce::jmax(0, band - 1))];
        const auto p1 = points[static_cast<size_t>(band)];
        const auto p2 = points[static_cast<size_t>(band + 1)];
        const auto p3 = points[static_cast<size_t>(
            juce::jmin(SectionEq::bandCount - 1, band + 2))];
        const auto control1 = p1 + (p2 - p0) / 6.0f;
        const auto control2 = p2 - (p3 - p1) / 6.0f;
        path.cubicTo(control1, control2, p2);
    }
    return path;
}

void GraphicEqDisplay::paint(juce::Graphics& graphics)
{
    const auto outer = getLocalBounds().toFloat().reduced(1.0f);
    juce::ColourGradient background(
        MiguelColours::panelRaised(), outer.getCentreX(), outer.getY(),
        MiguelColours::panel(), outer.getCentreX(), outer.getBottom(), false);
    graphics.setGradientFill(background);
    graphics.fillRoundedRectangle(outer, 8.0f);
    graphics.setColour(MiguelColours::border());
    graphics.drawRoundedRectangle(outer, 8.0f, 1.0f);

    const auto bounds = graphBounds();
    static constexpr std::array<double, 9> frequencyLines{
        60.0, 100.0, 250.0, 500.0, 1000.0,
        2500.0, 5000.0, 10000.0, 20000.0
    };
    graphics.setFont(juce::FontOptions(10.5f));
    for (const auto frequency : frequencyLines)
    {
        const auto x = frequencyToX(frequency);
        graphics.setColour(MiguelColours::border().withAlpha(0.5f));
        graphics.drawVerticalLine(juce::roundToInt(x),
                                  bounds.getY(), bounds.getBottom());
        const auto label = frequency >= 1000.0
            ? juce::String(frequency / 1000.0, frequency < 10000.0 ? 1 : 0)
                + "k"
            : juce::String(juce::roundToInt(frequency));
        graphics.setColour(MiguelColours::textMuted());
        graphics.drawText(label, juce::roundToInt(x) - 18,
                          juce::roundToInt(bounds.getBottom()) + 2,
                          36, 14, juce::Justification::centred);
    }

    for (int gain = -24; gain <= 24; gain += 6)
    {
        const auto y = gainToY(static_cast<float>(gain));
        graphics.setColour(gain == 0 ? MiguelColours::textMuted().withAlpha(0.5f)
                                     : MiguelColours::border().withAlpha(0.35f));
        graphics.drawHorizontalLine(juce::roundToInt(y),
                                    bounds.getX(), bounds.getRight());
        graphics.setColour(MiguelColours::textMuted());
        graphics.drawText((gain > 0 ? "+" : "") + juce::String(gain),
                          3, juce::roundToInt(y) - 7, 35, 14,
                          juce::Justification::centredRight);
    }

    const auto drawCurve = [&](
        const std::array<float, SectionEq::bandCount>& gains,
        juce::Colour colour, bool active)
    {
        const auto curve = createCurve(gains);
        auto fill = curve;
        fill.lineTo(bounds.getRight(), gainToY(0.0f));
        fill.lineTo(bounds.getX(), gainToY(0.0f));
        fill.closeSubPath();
        juce::ColourGradient curveFill(
            colour.withAlpha(active ? 0.28f : 0.12f),
            bounds.getCentreX(), bounds.getY(),
            colour.withAlpha(0.0f), bounds.getCentreX(), bounds.getBottom(),
            false);
        graphics.setGradientFill(curveFill);
        graphics.fillPath(fill);
        graphics.setColour(colour.withAlpha(active ? 1.0f : 0.55f));
        graphics.strokePath(curve, juce::PathStrokeType(active ? 2.5f : 1.5f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };

    drawCurve(inputGains, MiguelColours::cyan(), !editingOutput);
    drawCurve(outputGains, MiguelColours::orange(), editingOutput);

    const auto& activeGains = editingOutput ? outputGains : inputGains;
    const auto activeColour = editingOutput ? MiguelColours::orange()
                                            : MiguelColours::cyan();
    for (int band = 0; band < SectionEq::bandCount; ++band)
    {
        const juce::Point<float> centre{
            frequencyToX(SectionEq::frequencies[static_cast<size_t>(band)]),
            gainToY(activeGains[static_cast<size_t>(band)])
        };
        graphics.setColour(juce::Colours::black.withAlpha(0.45f));
        graphics.fillEllipse(centre.x - 7.0f, centre.y - 5.0f, 14.0f, 14.0f);
        graphics.setColour(activeColour);
        graphics.fillEllipse(centre.x - 6.0f, centre.y - 7.0f, 12.0f, 12.0f);
        graphics.setColour(MiguelColours::text());
        graphics.drawEllipse(centre.x - 6.0f, centre.y - 7.0f,
                             12.0f, 12.0f, 1.0f);
    }

    graphics.setColour(MiguelColours::cyan());
    graphics.drawText("ENTRADA", 48, 2, 65, 16,
                      juce::Justification::centredLeft);
    graphics.setColour(MiguelColours::orange());
    graphics.drawText("SALIDA", 116, 2, 60, 16,
                      juce::Justification::centredLeft);
}

int GraphicEqDisplay::nearestBand(juce::Point<float> position) const
{
    const auto& gains = editingOutput ? outputGains : inputGains;
    auto bestDistance = 16.0f;
    auto result = -1;
    for (int band = 0; band < SectionEq::bandCount; ++band)
    {
        const juce::Point<float> node{
            frequencyToX(SectionEq::frequencies[static_cast<size_t>(band)]),
            gainToY(gains[static_cast<size_t>(band)])
        };
        const auto distance = position.getDistanceFrom(node);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            result = band;
        }
    }
    return result;
}

void GraphicEqDisplay::mouseDown(const juce::MouseEvent& event)
{
    draggedBand = nearestBand(event.position);
    if (draggedBand >= 0)
        updateDraggedNode(event);
}

void GraphicEqDisplay::mouseDrag(const juce::MouseEvent& event)
{
    if (draggedBand >= 0)
        updateDraggedNode(event);
}

void GraphicEqDisplay::mouseUp(const juce::MouseEvent&)
{
    draggedBand = -1;
}

void GraphicEqDisplay::updateDraggedNode(const juce::MouseEvent& event)
{
    const auto gain = std::round(yToGain(event.position.y) * 10.0f) / 10.0f;
    auto& gains = editingOutput ? outputGains : inputGains;
    gains[static_cast<size_t>(draggedBand)] = gain;
    if (onGainChanged)
        onGainChanged(editingOutput, draggedBand, gain);
    repaint();
}
