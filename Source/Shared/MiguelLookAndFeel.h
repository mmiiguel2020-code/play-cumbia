#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace MiguelColours
{
inline juce::Colour background()       { return juce::Colour(0xff100e16); }
inline juce::Colour panel()            { return juce::Colour(0xff1a1624); }
inline juce::Colour panelRaised()      { return juce::Colour(0xff241e30); }
inline juce::Colour panelHighlight()   { return juce::Colour(0xff322a42); }
inline juce::Colour border()           { return juce::Colour(0xff5c4d78); }
inline juce::Colour text()             { return juce::Colour(0xfff4eefc); }
inline juce::Colour textMuted()        { return juce::Colour(0xffb8a8cc); }
inline juce::Colour lilac()            { return juce::Colour(0xffc9b0ea); }
inline juce::Colour purple()           { return juce::Colour(0xff9b7ad8); }
inline juce::Colour cyan()             { return juce::Colour(0xff7eb4e0); }
inline juce::Colour blue()             { return juce::Colour(0xff7eb4e0); }
inline juce::Colour green()            { return juce::Colour(0xff6ec8a4); }
inline juce::Colour orange()           { return juce::Colour(0xffc9b0ea); }
inline juce::Colour pink()             { return juce::Colour(0xffb894d4); }
inline juce::Colour yellow()           { return juce::Colour(0xffdcc98a); }
inline juce::Colour danger()           { return juce::Colour(0xffe07080); }
}

class MiguelLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    MiguelLookAndFeel();

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour&, bool, bool) override;
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float,
                          float, float, juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float,
                          float, juce::Slider::SliderStyle,
                          juce::Slider&) override;
    void drawTabButton(juce::TabBarButton&, juce::Graphics&,
                       bool, bool) override;
    void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int,
                      juce::ComboBox&) override;
    void drawMenuBarBackground(juce::Graphics&, int, int, bool,
                               juce::MenuBarComponent&) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiguelLookAndFeel)
};
