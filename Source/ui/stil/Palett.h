// Radiohusets färger: kittgrå lack, valnöt, elfenben, mässing, glimrörens neonorange, det magiska
// ögats gröna och PÅ LUFTEN-skyltens röda.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_graphics/juce_graphics.h>

namespace rv::stil::farg
{
    // Bordet och panelen
    inline const juce::Colour valnot       { 0xff5b3a22 };
    inline const juce::Colour valnotLjus   { 0xff83583a };
    inline const juce::Colour valnotMork   { 0xff2e1b0e };
    inline const juce::Colour kitt         { 0xff9b9a8f };   // kittgrå lackad plåt
    inline const juce::Colour kittModul    { 0xffa6a59a };   // modulernas fält, en nyans ljusare
    inline const juce::Colour kittMork     { 0xff5f5e56 };   // skuggor, springor
    inline const juce::Colour fog          { 0xff3a3934 };

    // Skyltar och tryck
    inline const juce::Colour elfenben     { 0xffede3c8 };
    inline const juce::Colour elfenbenSkugga { 0xffc4b591 };
    inline const juce::Colour tryck        { 0xff1e1c18 };   // svart text på elfenben
    inline const juce::Colour tryckLjust   { 0xfff4f0e4 };   // vitt tryck på lacken
    inline const juce::Colour tryckMatt    { 0xff4c4b44 };   // skalstreck på lacken
    inline const juce::Colour varde        { 0xff2a2822 };   // värdetext under rattarna

    // Metaller
    inline const juce::Colour massing      { 0xffbb9444 };
    inline const juce::Colour massingLjus  { 0xffeccf86 };
    inline const juce::Colour massingMork  { 0xff6e5220 };
    inline const juce::Colour krom         { 0xffdcdee0 };
    inline const juce::Colour kromMork     { 0xff70757a };

    // Glas och ljus
    inline const juce::Colour glasMorkt    { 0xff17140f };
    inline const juce::Colour neon         { 0xffff7418 };   // glimrörens orange
    inline const juce::Colour neonGlod     { 0xffffb468 };
    inline const juce::Colour neonSlackt   { 0xff3b2214 };
    inline const juce::Colour oga          { 0xff3fe08a };
    inline const juce::Colour ogaMorkt     { 0xff0d2e1c };
    inline const juce::Colour signalRod    { 0xffd2261c };
    inline const juce::Colour rodGlod      { 0xffff6a4a };
    inline const juce::Colour tavla        { 0xfff0e2b6 };   // VU-mätarens gulnade tavla
    inline const juce::Colour skala        { 0xfff6ead0 };   // tonskalans bakbelysta glas
    inline const juce::Colour barnsten     { 0xffffa823 };
}
