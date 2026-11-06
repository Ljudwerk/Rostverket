// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginEditor.h"

namespace pb = rv::stil::panel;

RostverketEditor::RostverketEditor (RostverketProcessor& p) : AudioProcessorEditor (p), panel (p), proc (p)
{
    addAndMakeVisible (panel);
    panel.onStorlek = [this] (float st) { setSize ((int) std::round (pb::bredd * st), (int) std::round (pb::hojd * st)); };
    setResizable (true, true);
    setResizeLimits ((int) (pb::bredd * 0.6f), (int) (pb::hojd * 0.6f), (int) (pb::bredd * 1.6f), (int) (pb::hojd * 1.6f));
    getConstrainer()->setFixedAspectRatio ((double) pb::bredd / (double) pb::hojd);
    const float s = juce::jlimit (0.6f, 1.6f, (float) (double) proc.apvts.state.getProperty ("storlek", 0.85));
    setSize ((int) std::round (pb::bredd * s), (int) std::round (pb::hojd * s));
}

void RostverketEditor::resized()
{
    const float s = (float) getWidth() / (float) pb::bredd;
    proc.apvts.state.setProperty ("storlek", s, nullptr);
    panel.storlek = s;
    panel.setBounds (0, 0, pb::bredd, pb::hojd);
    panel.setTransform (juce::AffineTransform::scale (s));
}
