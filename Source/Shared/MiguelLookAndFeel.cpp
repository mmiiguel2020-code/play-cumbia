#include "MiguelLookAndFeel.h"

MiguelLookAndFeel::MiguelLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId,
              MiguelColours::background());
    setColour(juce::Label::textColourId, MiguelColours::text());
    setColour(juce::TextButton::buttonColourId, MiguelColours::panelRaised());
    setColour(juce::TextButton::buttonOnColourId, MiguelColours::blue());
    setColour(juce::TextButton::textColourOffId, MiguelColours::text());
    setColour(juce::TextButton::textColourOnId, MiguelColours::background());
    setColour(juce::ComboBox::backgroundColourId, MiguelColours::panelRaised());
    setColour(juce::ComboBox::outlineColourId, MiguelColours::border());
    setColour(juce::ComboBox::textColourId, MiguelColours::text());
    setColour(juce::ComboBox::arrowColourId, MiguelColours::blue());
    setColour(juce::PopupMenu::backgroundColourId, MiguelColours::panel());
    setColour(juce::PopupMenu::textColourId, MiguelColours::text());
    setColour(juce::PopupMenu::highlightedBackgroundColourId,
              MiguelColours::green().withAlpha(0.78f));
    setColour(juce::TextEditor::highlightColourId,
              MiguelColours::green().withAlpha(0.40f));
    setColour(juce::ListBox::backgroundColourId, MiguelColours::panel());
    setColour(juce::Slider::trackColourId, MiguelColours::blue());
    setColour(juce::Slider::thumbColourId, MiguelColours::lilac());
    setColour(juce::Slider::rotarySliderFillColourId, MiguelColours::lilac());
    setColour(juce::Slider::rotarySliderOutlineColourId,
              MiguelColours::panelHighlight());
    setColour(juce::Slider::textBoxTextColourId, MiguelColours::text());
    setColour(juce::Slider::textBoxBackgroundColourId, MiguelColours::panel());
    setColour(juce::Slider::textBoxOutlineColourId, MiguelColours::border());
    setColour(juce::TabbedButtonBar::tabTextColourId,
              MiguelColours::textMuted());
    setColour(juce::TabbedButtonBar::frontTextColourId,
              MiguelColours::text());
    setColour(juce::TreeView::backgroundColourId, MiguelColours::panel());
    setColour(juce::ScrollBar::thumbColourId,
              MiguelColours::purple().withAlpha(0.8f));
    setColour(juce::TextEditor::backgroundColourId, MiguelColours::panel());
    setColour(juce::TextEditor::textColourId, MiguelColours::text());
    setColour(juce::TextEditor::outlineColourId, MiguelColours::border());
    setColour(juce::TextEditor::focusedOutlineColourId, MiguelColours::blue());
}

void MiguelLookAndFeel::drawButtonBackground(
    juce::Graphics& graphics, juce::Button& button,
    const juce::Colour& backgroundColour, bool isMouseOver, bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.75f);
    auto base = backgroundColour;
    if (isDown)
        base = base.brighter(0.16f);
    else if (isMouseOver)
        base = base.brighter(0.08f);

    juce::ColourGradient gradient(
        base.brighter(0.06f), bounds.getCentreX(), bounds.getY(),
        base.darker(0.12f), bounds.getCentreX(), bounds.getBottom(), false);
    graphics.setGradientFill(gradient);
    graphics.fillRoundedRectangle(bounds, 5.0f);
    graphics.setColour((button.getToggleState() ? MiguelColours::lilac()
                                                 : MiguelColours::border())
                           .withAlpha(isMouseOver ? 0.95f : 0.72f));
    graphics.drawRoundedRectangle(bounds, 5.0f, 1.0f);
}

void MiguelLookAndFeel::drawRotarySlider(
    juce::Graphics& graphics, int x, int y, int width, int height,
    float sliderPosition, float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider& slider)
{
    auto area = juce::Rectangle<float>(static_cast<float>(x),
                                       static_cast<float>(y),
                                       static_cast<float>(width),
                                       static_cast<float>(height));
    const auto diameter = juce::jmin(area.getWidth(), area.getHeight()) - 12.0f;
    auto knob = area.withSizeKeepingCentre(diameter, diameter);
    const auto radius = diameter * 0.5f;
    const auto lineWidth = juce::jmax(2.0f, radius * 0.10f);
    const auto arcRadius = radius - lineWidth * 0.7f;
    const auto angle = rotaryStartAngle
        + sliderPosition * (rotaryEndAngle - rotaryStartAngle);

    graphics.setColour(juce::Colours::black.withAlpha(0.35f));
    graphics.fillEllipse(knob.translated(0.0f, 3.0f));

    juce::ColourGradient face(
        MiguelColours::panelHighlight(), knob.getX(), knob.getY(),
        MiguelColours::panel(), knob.getRight(), knob.getBottom(), false);
    graphics.setGradientFill(face);
    graphics.fillEllipse(knob);
    graphics.setColour(MiguelColours::border());
    graphics.drawEllipse(knob, 1.0f);

    juce::Path backgroundArc;
    backgroundArc.addCentredArc(knob.getCentreX(), knob.getCentreY(),
                                arcRadius, arcRadius, 0.0f,
                                rotaryStartAngle, rotaryEndAngle, true);
    graphics.setColour(slider.findColour(
        juce::Slider::rotarySliderOutlineColourId));
    graphics.strokePath(backgroundArc,
                        juce::PathStrokeType(lineWidth,
                            juce::PathStrokeType::curved,
                            juce::PathStrokeType::rounded));

    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc(knob.getCentreX(), knob.getCentreY(),
                              arcRadius, arcRadius, 0.0f,
                              rotaryStartAngle, angle, true);
        graphics.setColour(slider.findColour(
            juce::Slider::rotarySliderFillColourId));
        graphics.strokePath(valueArc,
                            juce::PathStrokeType(lineWidth,
                                juce::PathStrokeType::curved,
                                juce::PathStrokeType::rounded));
    }

    juce::Path pointer;
    const auto pointerLength = radius * 0.48f;
    const auto pointerThickness = juce::jmax(2.0f, radius * 0.07f);
    pointer.addRoundedRectangle(-pointerThickness * 0.5f,
                                -radius * 0.68f,
                                pointerThickness, pointerLength,
                                pointerThickness * 0.5f);
    graphics.setColour(MiguelColours::text());
    graphics.fillPath(pointer, juce::AffineTransform::rotation(angle)
                                   .translated(knob.getCentreX(),
                                               knob.getCentreY()));
}

void MiguelLookAndFeel::drawLinearSlider(
    juce::Graphics& graphics, int x, int y, int width, int height,
    float sliderPosition, float minSliderPosition, float maxSliderPosition,
    juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style == juce::Slider::LinearBar
        || style == juce::Slider::LinearBarVertical)
    {
        juce::LookAndFeel_V4::drawLinearSlider(
            graphics, x, y, width, height, sliderPosition,
            minSliderPosition, maxSliderPosition, style, slider);
        return;
    }

    const bool vertical = style == juce::Slider::LinearVertical;
    auto track = vertical
        ? juce::Rectangle<float>(static_cast<float>(x + width / 2 - 2),
                                 static_cast<float>(y + 4), 4.0f,
                                 static_cast<float>(height - 8))
        : juce::Rectangle<float>(static_cast<float>(x + 4),
                                 static_cast<float>(y + height / 2 - 2),
                                 static_cast<float>(width - 8), 4.0f);
    graphics.setColour(MiguelColours::panelHighlight());
    graphics.fillRoundedRectangle(track, 2.0f);

    auto active = vertical
        ? juce::Rectangle<float>(track.getX(), sliderPosition,
                                 track.getWidth(),
                                 track.getBottom() - sliderPosition)
        : juce::Rectangle<float>(track.getX(), track.getY(),
                                 sliderPosition - track.getX(),
                                 track.getHeight());
    graphics.setColour(slider.findColour(juce::Slider::trackColourId));
    graphics.fillRoundedRectangle(active, 2.0f);

    const auto thumb = vertical
        ? juce::Rectangle<float>(track.getCentreX() - 8.0f,
                                 sliderPosition - 4.0f, 16.0f, 8.0f)
        : juce::Rectangle<float>(sliderPosition - 4.0f,
                                 track.getCentreY() - 8.0f, 8.0f, 16.0f);
    graphics.setColour(slider.findColour(juce::Slider::thumbColourId));
    graphics.fillRoundedRectangle(thumb, 3.0f);
}

void MiguelLookAndFeel::drawTabButton(
    juce::TabBarButton& button, juce::Graphics& graphics,
    bool isMouseOver, bool isMouseDown)
{
    auto area = button.getLocalBounds().toFloat().reduced(1.0f, 2.0f);
    auto colour = button.getTabBackgroundColour();
    if (isMouseDown)
        colour = colour.brighter(0.12f);
    else if (isMouseOver)
        colour = colour.brighter(0.07f);

    graphics.setColour(button.getToggleState()
                           ? colour.withAlpha(0.32f)
                           : MiguelColours::panel().withAlpha(0.96f));
    graphics.fillRoundedRectangle(area, 5.0f);

    if (button.getToggleState())
    {
        graphics.setColour(colour);
        graphics.fillRoundedRectangle(
            area.removeFromBottom(3.0f), 1.5f);
    }

    graphics.setColour(button.getToggleState()
                           ? MiguelColours::text()
                           : MiguelColours::textMuted());
    graphics.setFont(juce::FontOptions(14.0f,
        button.getToggleState() ? juce::Font::bold
                                : juce::Font::plain));
    graphics.drawFittedText(button.getButtonText(),
                            button.getLocalBounds().reduced(8, 2),
                            juce::Justification::centred, 1);
}

void MiguelLookAndFeel::drawComboBox(
    juce::Graphics& graphics, int width, int height, bool isButtonDown,
    int buttonX, int buttonY, int buttonWidth, int buttonHeight,
    juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f,
        static_cast<float>(width), static_cast<float>(height)).reduced(0.75f);
    juce::ColourGradient gradient(
        MiguelColours::panelRaised().brighter(0.04f), 0.0f, bounds.getY(),
        MiguelColours::panel().darker(0.06f), 0.0f, bounds.getBottom(), false);
    graphics.setGradientFill(gradient);
    graphics.fillRoundedRectangle(bounds, 4.0f);
    graphics.setColour(box.hasKeyboardFocus(true) ? MiguelColours::blue()
                                                   : MiguelColours::border());
    graphics.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    const auto arrowArea = juce::Rectangle<float>(
        static_cast<float>(buttonX), static_cast<float>(buttonY),
        static_cast<float>(buttonWidth), static_cast<float>(buttonHeight));
    juce::Path arrow;
    arrow.addTriangle(arrowArea.getCentreX() - 4.0f,
                      arrowArea.getCentreY() - 2.0f,
                      arrowArea.getCentreX() + 4.0f,
                      arrowArea.getCentreY() - 2.0f,
                      arrowArea.getCentreX(),
                      arrowArea.getCentreY() + 3.0f);
    graphics.setColour(MiguelColours::blue()
                           .withAlpha(isButtonDown ? 1.0f : 0.85f));
    graphics.fillPath(arrow);
}

void MiguelLookAndFeel::drawMenuBarBackground(juce::Graphics& graphics,
                                              int width,
                                              int height,
                                              bool /*isMouseOverBar*/,
                                              juce::MenuBarComponent&)
{
    graphics.fillAll(MiguelColours::panel());
    graphics.setColour(MiguelColours::purple().withAlpha(0.35f));
    graphics.fillRect(0, height - 1, width, 1);
}
