#pragma once

#include "GrooveEngine.h"
#include "SectionEq.h"

#include <juce_data_structures/juce_data_structures.h>

namespace SessionState
{
inline constexpr const char* rootType = "MMASession";
inline constexpr const char* versionProperty = "version";
inline constexpr int currentVersion = 1;

juce::ValueTree buildFromEngines(const GrooveEngine& groove,
                                 const SectionEqBank& eqBank,
                                 const juce::ValueTree& uiState,
                                 const juce::ValueTree& fxState = {});

void applyToEngines(const juce::ValueTree& session,
                      GrooveEngine& groove,
                      SectionEqBank& eqBank);

juce::File autosaveFile();
bool writeAutosave(const juce::ValueTree& session);
juce::ValueTree readAutosave();
}
