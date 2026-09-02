#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace MiguelColours
{
inline juce::Colour background()       { return juce::Colour(0xff1a1a1f); }
inline juce::Colour panel()            { return juce::Colour(0xff20202a); }
inline juce::Colour panelRaised()      { return juce::Colour(0xff292934); }
inline juce::Colour panelHighlight()   { return juce::Colour(0xff343441); }
inline juce::Colour border()           { return juce::Colour(0xff444553); }
inline juce::Colour text()             { return juce::Colour(0xfff3f5f7); }
inline juce::Colour textMuted()        { return juce::Colour(0xffaeb3bd); }
inline juce::Colour cyan()             { return juce::Colour(0xff3fa9f5); }
inline juce::Colour green()            { return juce::Colour(0xff45c98a); }
inline juce::Colour orange()           { return juce::Colour(0xffffa94d); }
inline juce::Colour purple()           { return juce::Colour(0xffa879ff); }
inline juce::Colour pink()             { return juce::Colour(0xffff6fae); }
inline juce::Colour yellow()           { return juce::Colour(0xffffd166); }
inline juce::Colour danger()           { return juce::Colour(0xffff5f68); }
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

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiguelLookAndFeel)
};
