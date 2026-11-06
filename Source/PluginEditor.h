// Röstverket – editorn: sändarbordet (ui/Panelen.h) i vald storlek, 60–160 % (85 % från början).
// Storleken ändras genom att dra i hörnet och sparas med projektet.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "PluginProcessor.h"
#include "ui/Panelen.h"

class RostverketEditor : public juce::AudioProcessorEditor
{
public:
    explicit RostverketEditor (RostverketProcessor& p);
    void resized() override;

    rv::ui::Panelen panel;

private:
    RostverketProcessor& proc;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RostverketEditor)
};
