// Den fristående appen: Röstverket i ett eget fönster (krang::app::FristaendeApp). Byggs bara in i appen
// (JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP). Mikrofonen är av från början mot rundgång (JUCE:s "tysta
// ingången", som sparas mellan gångerna); panelens MENY slår på den via funktionerna nedan.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#include <krang/app/Fristaende.h>
#include "../PluginProcessor.h"
#include "../ui/stil/Palett.h"

juce::JUCEApplicationBase* juce_CreateApplication()
{
    RostverketProcessor::app.mikrofonPa = []
    {
        auto* h = juce::StandalonePluginHolder::getInstance();
        return h != nullptr && ! (bool) h->getMuteInputValue().getValue();
    };
    RostverketProcessor::app.sattMikrofon = [] (bool pa)
    {
        if (auto* h = juce::StandalonePluginHolder::getInstance()) h->getMuteInputValue().setValue (! pa);
    };
    return new krang::app::FristaendeApp<RostverketProcessor> (juce::String::fromUTF8 ("Röstverket"), rv::stil::farg::kitt);
}
