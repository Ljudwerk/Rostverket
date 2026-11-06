// Radiohusets ritverktyg: valnöt, kittgrå lack, högtalarduk, mässingsskylt, elfenbensplaketter,
// bakelitrattar med kromad skalring, vridomkopplare, tumvippor, glaslampor, VU-mätare, det magiska ögat,
// glimrörens bandmätare, tonskalan och PÅ LUFTEN-skylten. Allt ritas i kod, i designpixlar.
//
// Texturerna (valnöt, lack, duk) är deterministiska: samma frö ger samma bild.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_graphics/juce_graphics.h>
#include <krang/stil/Linjeskrift.h>
#include <krang/stil/Textur.h>
#include "Palett.h"

namespace rv::stil
{
using krang::stil::Slump;
namespace ls = krang::stil::linjeskrift;

inline const float pi = juce::MathConstants<float>::pi;

// ---------------------------------------------------------------------------------------------
// Text

inline ls::Stil text (float hojd, float tjocklek = -1.0f, float spar = 1.6f)
{
    return { hojd, tjocklek > 0.0f ? tjocklek : juce::jmax (0.8f, hojd * 0.13f), spar, 1.0f };
}

inline void skriv (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, float hojd, juce::Colour c,
                   juce::Justification j = juce::Justification::centred, float tjocklek = -1.0f)
{
    g.setColour (c);
    ls::rita (g, t, r, text (hojd, tjocklek), j);
}

// Glödande text (glimrör, bakbelyst glas): bred svag gloria och en tunn ljus kärna.
inline void skrivGlod (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, float hojd, juce::Colour c,
                       juce::Justification j = juce::Justification::centred)
{
    auto s = text (hojd);
    for (int i = 3; i >= 1; --i)
    {
        auto s2 = s; s2.tjocklek = s.tjocklek * (1.0f + (float) i * 1.4f);
        g.setColour (c.withAlpha (0.10f));
        ls::rita (g, t, r, s2, j);
    }
    g.setColour (c);
    ls::rita (g, t, r, s, j);
    auto k = s; k.tjocklek *= 0.45f;
    g.setColour (c.brighter (0.6f).withAlpha (0.8f));
    ls::rita (g, t, r, k, j);
}

// ---------------------------------------------------------------------------------------------
// Ytor

// Valnötsfaner: fladdrande ådring längs x, med porer och mörkare årsringar. Skapas i fysiska pixlar.
inline juce::Image valnotsbild (int w, int h, float skala, uint32_t fro)
{
    const int pw = juce::jmax (1, (int) std::ceil (w * skala)), ph = juce::jmax (1, (int) std::ceil (h * skala));
    juce::Image bild (juce::Image::RGB, pw, ph, false);
    juce::Image::BitmapData bd (bild, juce::Image::BitmapData::writeOnly);
    Slump r (fro);
    const float fas1 = r.enhet() * 6.28f, fas2 = r.enhet() * 6.28f, fas3 = r.enhet() * 6.28f;
    const auto a = farg::valnotMork, b = farg::valnot, c = farg::valnotLjus;
    for (int y = 0; y < ph; ++y)
    {
        const float yd = (float) y / skala;
        for (int x = 0; x < pw; ++x)
        {
            const float xd = (float) x / skala;
            // Ådringen: en fältfunktion som böljar långsamt i x, med en flamma i mitten.
            const float bolj = 9.0f * std::sin (xd * 0.006f + fas1) + 4.0f * std::sin (xd * 0.021f + fas2)
                             + 2.0f * std::sin (xd * 0.05f + yd * 0.02f + fas3);
            const float f = (yd + bolj) * 0.33f;
            float ring = 0.5f + 0.5f * std::sin (f + 0.8f * std::sin (f * 0.37f));
            ring = std::pow (ring, 3.0f);
            const float fin = 0.5f + 0.5f * std::sin ((yd + bolj * 1.3f) * 2.1f);
            float t = 0.55f + 0.35f * ring - 0.18f * fin * 0.5f + r.bipolar() * 0.04f;
            if (r.enhet() < 0.0015f) t -= 0.35f;   // porer
            t = juce::jlimit (0.0f, 1.0f, t);
            const auto col = t < 0.5f ? a.interpolatedWith (b, t * 2.0f) : b.interpolatedWith (c, (t - 0.5f) * 2.0f);
            bd.setPixelColour (x, y, col);
        }
    }
    return bild;
}

// Lackad plåt: en jämn yta med fint korn (plastnarven med låg styrka ger rätt känsla av sprutlack).
inline juce::Image lackbild (int w, int h, float skala, juce::Colour bas, uint32_t fro)
{
    return krang::stil::plastnarv (w, h, skala, bas, 3.0f, fro);
}

// Högtalarduk: vävd i två riktningar, ljusa trådar på mörk botten, med lätt glans i mitten.
inline void duk (juce::Graphics& g, juce::Rectangle<float> r)
{
    juce::Path p; p.addRoundedRectangle (r, 6.0f);
    g.setColour (juce::Colour (0xff2b241b));
    g.fillPath (p);
    g.saveState();
    g.reduceClipRegion (p);
    for (float y = r.getY(); y < r.getBottom(); y += 2.0f)
    {
        g.setColour (juce::Colour (0xffb7a37c).withAlpha (((int) (y * 0.5f) % 2) ? 0.55f : 0.35f));
        g.fillRect (r.getX(), y, r.getWidth(), 0.9f);
    }
    for (float x = r.getX(); x < r.getRight(); x += 2.0f)
    {
        g.setColour (juce::Colour (0xffd2c29a).withAlpha (((int) (x * 0.5f) % 3) ? 0.30f : 0.45f));
        g.fillRect (x, r.getY(), 0.9f, r.getHeight());
    }
    juce::ColourGradient glans (juce::Colours::white.withAlpha (0.10f), r.getCentreX(), r.getY(),
                                juce::Colours::black.withAlpha (0.25f), r.getCentreX(), r.getBottom(), false);
    g.setGradientFill (glans);
    g.fillPath (p);
    g.restoreState();
    // Mässingslist
    g.setColour (farg::massingMork);
    g.strokePath (p, juce::PathStrokeType (2.6f));
    g.setColour (farg::massingLjus.withAlpha (0.7f));
    g.strokePath (p, juce::PathStrokeType (1.0f));
}

// En skruv med spår, i krom eller mässing.
inline void skruv (juce::Graphics& g, juce::Point<float> c, float r, float vinkel, bool massing = false)
{
    const auto bas = massing ? farg::massing : farg::krom;
    juce::ColourGradient gr (bas.brighter (0.4f), c.x - r, c.y - r, bas.darker (0.8f), c.x + r, c.y + r, false);
    g.setGradientFill (gr);
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 0.6f);
    const float dx = std::cos (vinkel) * r * 0.8f, dy = std::sin (vinkel) * r * 0.8f;
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawLine (c.x - dx, c.y - dy, c.x + dx, c.y + dy, juce::jmax (0.7f, r * 0.28f));
}

// Mässingsplatta med präglad text (modellskylten).
inline void massingsplatta (juce::Graphics& g, juce::Rectangle<float> r, float horn = 4.0f)
{
    juce::Path p; p.addRoundedRectangle (r, horn);
    juce::ColourGradient gr (farg::massingLjus, r.getX(), r.getY(), farg::massingMork, r.getX(), r.getBottom(), false);
    gr.addColour (0.45, farg::massing);
    gr.addColour (0.55, farg::massing.brighter (0.15f));
    g.setGradientFill (gr);
    g.fillPath (p);
    // Borstning
    Slump s (77);
    for (float y = r.getY() + 1.0f; y < r.getBottom() - 1.0f; y += 0.8f)
    {
        g.setColour ((s.enhet() > 0.5f ? juce::Colours::white : juce::Colours::black).withAlpha (0.05f + 0.05f * s.enhet()));
        g.fillRect (r.getX() + 1.0f, y, r.getWidth() - 2.0f, 0.4f);
    }
    g.setColour (farg::massingMork.darker (0.6f));
    g.strokePath (p, juce::PathStrokeType (1.2f));
}

// Präglad text: ljus kant nedåt-höger, mörk uppåt-vänster, och själva texten.
inline void praglat (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, float hojd, juce::Colour c,
                     juce::Justification j = juce::Justification::centred)
{
    auto s = text (hojd, hojd * 0.16f, 2.2f);
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    ls::rita (g, t, r.translated (0.8f, 0.8f), s, j);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    ls::rita (g, t, r.translated (-0.6f, -0.6f), s, j);
    g.setColour (c);
    ls::rita (g, t, r, s, j);
}

// En bandmikrofon i profil (generisk): ett rundat galler på en bygel och en fot.
inline void mikrofon (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    const float w = r.getWidth(), h = r.getHeight();
    auto huvud = juce::Rectangle<float> (r.getX() + w * 0.28f, r.getY(), w * 0.44f, h * 0.62f);
    juce::Path p; p.addRoundedRectangle (huvud, w * 0.2f);
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (1.3f));
    for (int i = 1; i < 6; ++i)
    {
        const float y = huvud.getY() + huvud.getHeight() * (float) i / 6.0f;
        g.drawLine (huvud.getX() + 2.0f, y, huvud.getRight() - 2.0f, y, 0.8f);
    }
    juce::Path bygel;
    bygel.startNewSubPath (huvud.getX() - 2.0f, huvud.getCentreY());
    bygel.quadraticTo (huvud.getX() - 2.0f, huvud.getBottom() + h * 0.12f, huvud.getCentreX(), huvud.getBottom() + h * 0.12f);
    bygel.quadraticTo (huvud.getRight() + 2.0f, huvud.getBottom() + h * 0.12f, huvud.getRight() + 2.0f, huvud.getCentreY());
    g.strokePath (bygel, juce::PathStrokeType (1.3f));
    g.drawLine (huvud.getCentreX(), huvud.getBottom() + h * 0.12f, huvud.getCentreX(), r.getBottom() - h * 0.06f, 1.5f);
    g.drawLine (r.getX() + w * 0.25f, r.getBottom() - 0.8f, r.getRight() - w * 0.25f, r.getBottom() - 0.8f, 1.8f);
}

// Elfenbensplakett med svart text och två små skruvar.
inline void plakett (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, float hojd = 9.0f)
{
    juce::Path p; p.addRoundedRectangle (r, 2.5f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (r.translated (0.8f, 1.2f), 2.5f);
    juce::ColourGradient gr (farg::elfenben.brighter (0.1f), r.getX(), r.getY(), farg::elfenbenSkugga, r.getX(), r.getBottom(), false);
    g.setGradientFill (gr);
    g.fillPath (p);
    g.setColour (farg::elfenbenSkugga.darker (0.5f));
    g.strokePath (p, juce::PathStrokeType (0.8f));
    skruv (g, { r.getX() + 6.0f, r.getCentreY() }, 2.2f, 0.6f, true);
    skruv (g, { r.getRight() - 6.0f, r.getCentreY() }, 2.2f, 2.1f, true);
    skriv (g, t, r.reduced (14.0f, 0.0f), hojd, farg::tryck);
}

// ---------------------------------------------------------------------------------------------
// Reglage

// Vinklar för rattar: 0 = −135° från tolv, 1 = +135°.
inline float rattVinkel (float v) { return (-0.75f + 1.5f * juce::jlimit (0.0f, 1.0f, v)) * pi; }

// Elfenbensvit bakelitratt med räfflad kjol, kromad skalring och tryckt skala 0–10 på lacken.
inline void ratt (juce::Graphics& g, juce::Point<float> c, float r, float varde, bool skala = true)
{
    // Skalan 0–10 på lacken
    if (skala)
    {
        for (int i = 0; i <= 10; ++i)
        {
            const float a = rattVinkel ((float) i / 10.0f);
            const float r1 = r * 1.30f, r2 = r * (i % 5 == 0 ? 1.52f : 1.42f);
            g.setColour (farg::tryckMatt);
            g.drawLine (c.x + std::sin (a) * r1, c.y - std::cos (a) * r1, c.x + std::sin (a) * r2, c.y - std::cos (a) * r2,
                        i % 5 == 0 ? 1.1f : 0.7f);
            if (i % 5 == 0 && r > 18.0f)
            {
                const float rt = r * 1.78f;
                skriv (g, juce::String (i), { c.x + std::sin (a) * rt - 8.0f, c.y - std::cos (a) * rt - 4.0f, 16.0f, 8.0f },
                       r * 0.24f, farg::tryckMatt);
            }
        }
    }
    // Kromad skalring
    {
        const float rr = r * 1.18f;
        juce::ColourGradient gr (farg::krom.brighter (0.3f), c.x - rr, c.y - rr, farg::kromMork, c.x + rr, c.y + rr, false);
        g.setGradientFill (gr);
        g.fillEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2, 0.6f);
    }
    // Skugga och kjol med räfflor
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (c.x - r + 1.2f, c.y - r + 2.0f, r * 2, r * 2);
    {
        juce::ColourGradient gr (farg::elfenben.brighter (0.15f), c.x - r, c.y - r, farg::elfenbenSkugga.darker (0.25f), c.x + r, c.y + r, false);
        g.setGradientFill (gr);
        g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
        const int n = 28;
        for (int i = 0; i < n; ++i)
        {
            const float a = (float) i / (float) n * 2.0f * pi;
            const float ljus = 0.5f + 0.5f * std::cos (a + 2.3f);
            g.setColour (juce::Colours::black.withAlpha (0.10f + 0.12f * (1.0f - ljus)));
            g.drawLine (c.x + std::sin (a) * r * 0.80f, c.y - std::cos (a) * r * 0.80f,
                        c.x + std::sin (a) * r * 0.98f, c.y - std::cos (a) * r * 0.98f, juce::jmax (0.6f, r * 0.06f));
        }
    }
    // Toppen: en kupad elfenbensyta
    {
        const float rt = r * 0.74f;
        juce::ColourGradient gr (juce::Colours::white.withAlpha (0.95f), c.x - rt * 0.5f, c.y - rt * 0.7f,
                                 farg::elfenben.darker (0.08f), c.x + rt * 0.4f, c.y + rt * 0.6f, true);
        g.setGradientFill (gr);
        g.fillEllipse (c.x - rt, c.y - rt, rt * 2, rt * 2);
        g.setColour (farg::elfenbenSkugga.darker (0.3f).withAlpha (0.6f));
        g.drawEllipse (c.x - rt, c.y - rt, rt * 2, rt * 2, 0.6f);
    }
    // Visaren: svart ingraverad linje
    const float a = rattVinkel (varde);
    g.setColour (farg::tryck);
    g.drawLine (c.x + std::sin (a) * r * 0.25f, c.y - std::cos (a) * r * 0.25f,
                c.x + std::sin (a) * r * 0.96f, c.y - std::cos (a) * r * 0.96f, juce::jmax (1.2f, r * 0.11f));
}

// Vridomkopplare: elfenbensratt med räfflad spets och lägena tryckta på en båge runt om.
inline void valjare (juce::Graphics& g, juce::Point<float> c, float r, const juce::StringArray& lagen, int lage, float texthojd = 5.5f)
{
    const int n = juce::jmax (2, lagen.size());
    const float span = juce::jmin (1.5f * pi, (float) (n - 1) * 0.42f * pi);
    auto vinkel = [&] (int i) { return -span * 0.5f + span * (float) i / (float) (n - 1); };
    for (int i = 0; i < n; ++i)
    {
        const float a = vinkel (i);
        const bool vald = i == lage;
        g.setColour (vald ? farg::tryck : farg::tryckMatt);
        g.drawLine (c.x + std::sin (a) * r * 1.15f, c.y - std::cos (a) * r * 1.15f,
                    c.x + std::sin (a) * r * 1.38f, c.y - std::cos (a) * r * 1.38f, vald ? 1.3f : 0.8f);
        const auto t = lagen[i].toUpperCase();
        const float tb = ls::bredd (t, text (texthojd));
        const float rt = r * 1.55f + tb * 0.5f * std::abs (std::sin (a)) + texthojd * 0.6f;
        auto ruta = juce::Rectangle<float> (tb + 2.0f, texthojd + 2.0f).withCentre ({ c.x + std::sin (a) * rt, c.y - std::cos (a) * rt });
        skriv (g, t, ruta, texthojd, vald ? farg::tryck : farg::tryckMatt, juce::Justification::centred, vald ? texthojd * 0.17f : -1.0f);
    }
    // Kroppen: rund bas och en spetsig "pekare"
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (c.x - r + 1.0f, c.y - r + 1.8f, r * 2, r * 2);
    juce::ColourGradient gr (farg::elfenben.brighter (0.1f), c.x - r, c.y - r, farg::elfenbenSkugga.darker (0.2f), c.x + r, c.y + r, false);
    g.setGradientFill (gr);
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    const float a = vinkel (juce::jlimit (0, n - 1, lage));
    juce::Path spets;
    spets.startNewSubPath (0.0f, -r * 1.12f);
    spets.lineTo (r * 0.42f, -r * 0.1f);
    spets.lineTo (r * 0.30f, r * 0.62f);
    spets.lineTo (-r * 0.30f, r * 0.62f);
    spets.lineTo (-r * 0.42f, -r * 0.1f);
    spets.closeSubPath();
    spets.applyTransform (juce::AffineTransform::rotation (a).translated (c));
    juce::ColourGradient sg (juce::Colours::white, c.x - r, c.y - r, farg::elfenbenSkugga, c.x + r, c.y + r, false);
    g.setGradientFill (sg);
    g.fillPath (spets);
    g.setColour (farg::elfenbenSkugga.darker (0.5f));
    g.strokePath (spets, juce::PathStrokeType (0.7f));
    g.setColour (farg::tryck);
    g.drawLine (c.x + std::sin (a) * r * 0.2f, c.y - std::cos (a) * r * 0.2f,
                c.x + std::sin (a) * r * 0.95f, c.y - std::cos (a) * r * 0.95f, 1.2f);
}

// Tumvippa i krom på en liten rund platta, med PÅ uppe och AV nere.
inline void vippa (juce::Graphics& g, juce::Point<float> c, float s, bool pa)
{
    const float r = s * 0.5f;
    juce::ColourGradient gr (farg::krom.brighter (0.2f), c.x - r, c.y - r, farg::kromMork, c.x + r, c.y + r, false);
    g.setGradientFill (gr);
    g.fillEllipse (c.x - r, c.y - r, s, s);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (c.x - r, c.y - r, s, s, 0.6f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.fillEllipse (c.x - r * 0.38f, c.y - r * 0.38f, r * 0.76f, r * 0.76f);
    const float dir = pa ? -1.0f : 1.0f;
    const juce::Point<float> topp { c.x, c.y + dir * s * 0.95f };
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.drawLine (c.x + 1.0f, c.y + 1.0f, topp.x + 1.5f, topp.y + 1.5f, s * 0.24f);
    juce::ColourGradient lg (juce::Colours::white, c.x - 2.0f, c.y, farg::kromMork, c.x + 2.0f, c.y, false);
    g.setGradientFill (lg);
    juce::Path arm;
    arm.addLineSegment ({ c, topp }, s * 0.22f);
    g.fillPath (arm);
    g.fillEllipse (topp.x - s * 0.17f, topp.y - s * 0.17f, s * 0.34f, s * 0.34f);
}

// Glasad signallampa i krominfattning. styrka 0 = släckt.
inline void lampa (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour fargen, float styrka)
{
    juce::ColourGradient ring (farg::krom, c.x - r, c.y - r, farg::kromMork, c.x + r, c.y + r, false);
    g.setGradientFill (ring);
    g.fillEllipse (c.x - r * 1.35f, c.y - r * 1.35f, r * 2.7f, r * 2.7f);
    const auto bas = fargen.darker (2.2f).interpolatedWith (fargen, juce::jlimit (0.0f, 1.0f, styrka));
    if (styrka > 0.05f)
    {
        g.setColour (fargen.withAlpha (0.25f * styrka));
        g.fillEllipse (c.x - r * 2.4f, c.y - r * 2.4f, r * 4.8f, r * 4.8f);
    }
    juce::ColourGradient glas (bas.brighter (0.6f + styrka), c.x - r * 0.3f, c.y - r * 0.4f, bas.darker (0.5f), c.x + r, c.y + r, true);
    g.setGradientFill (glas);
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.fillEllipse (c.x - r * 0.55f, c.y - r * 0.6f, r * 0.45f, r * 0.32f);
}

// Tryckknapp i elfenben (◀ ▶ SPARA MENY).
inline void knapp (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, bool nere = false)
{
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (r.translated (0.0f, nere ? 0.6f : 1.6f), 3.0f);
    auto k = nere ? r.translated (0.0f, 0.8f) : r;
    juce::ColourGradient gr (farg::elfenben.brighter (0.12f), k.getX(), k.getY(), farg::elfenbenSkugga, k.getX(), k.getBottom(), false);
    g.setGradientFill (gr);
    g.fillRoundedRectangle (k, 3.0f);
    g.setColour (farg::elfenbenSkugga.darker (0.5f));
    g.drawRoundedRectangle (k, 3.0f, 0.7f);
    skriv (g, t, k, juce::jmin (7.0f, k.getHeight() * 0.42f), farg::tryck);
}

// ---------------------------------------------------------------------------------------------
// Instrumenten

// Glasfönster: mörk ram med en reflex överst.
inline void glas (juce::Graphics& g, juce::Rectangle<float> r, float horn = 4.0f)
{
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    juce::Path refl;
    refl.addRoundedRectangle (r.getX() + 2.0f, r.getY() + 2.0f, r.getWidth() - 4.0f, r.getHeight() * 0.35f, horn);
    g.fillPath (refl);
}

inline void kromram (juce::Graphics& g, juce::Rectangle<float> r, float horn = 5.0f, float tjocklek = 3.0f)
{
    juce::Path p; p.addRoundedRectangle (r, horn);
    juce::ColourGradient gr (farg::krom.brighter (0.4f), r.getX(), r.getY(), farg::kromMork, r.getRight(), r.getBottom(), false);
    g.setGradientFill (gr);
    g.strokePath (p, juce::PathStrokeType (tjocklek));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.strokePath (p, juce::PathStrokeType (0.6f));
}

// VU-mätare med gulnad tavla, skala −20 … +3 dB, rött fält och en svart visare.
inline void vu (juce::Graphics& g, juce::Rectangle<float> r, float db, const juce::String& namn)
{
    juce::ColourGradient tv (farg::tavla.brighter (0.1f), r.getCentreX(), r.getY(), farg::tavla.darker (0.25f), r.getCentreX(), r.getBottom(), false);
    g.setGradientFill (tv);
    g.fillRoundedRectangle (r, 5.0f);
    const juce::Point<float> nav { r.getCentreX(), r.getBottom() + r.getHeight() * 0.18f };
    const float radie = r.getHeight() * 0.86f;
    auto vinkelFor = [] (float d) { return juce::jmap (juce::jlimit (-20.0f, 3.0f, d), -20.0f, 3.0f, -0.23f * pi, 0.23f * pi); };
    // Rött fält 0 … +3
    juce::Path rott;
    rott.addCentredArc (nav.x, nav.y, radie, radie, 0.0f, vinkelFor (0.0f), vinkelFor (3.0f), true);
    g.setColour (farg::signalRod);
    g.strokePath (rott, juce::PathStrokeType (3.2f));
    juce::Path bage;
    bage.addCentredArc (nav.x, nav.y, radie - 2.0f, radie - 2.0f, 0.0f, vinkelFor (-20.0f), vinkelFor (3.0f), true);
    g.setColour (farg::tryck);
    g.strokePath (bage, juce::PathStrokeType (0.8f));
    for (int di : { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 })
    {
        const float d = (float) di;
        const float a = vinkelFor (d);
        const float r1 = radie - 2.0f, r2 = radie + (di % 5 == 0 || di >= 0 ? 5.0f : 3.0f);
        g.setColour (d >= 0.0f ? farg::signalRod : farg::tryck);
        g.drawLine (nav.x + std::sin (a) * r1, nav.y - std::cos (a) * r1, nav.x + std::sin (a) * r2, nav.y - std::cos (a) * r2, 0.9f);
        if (di == -20 || di == -10 || di == -5 || di == 0 || di == 3)
        {
            const float rt = radie + 10.0f;
            const auto t = di > 0 ? "+" + juce::String (di) : (di < 0 ? juce::String::fromUTF8 ("−") + juce::String (-di) : juce::String ("0"));
            skriv (g, t, { nav.x + std::sin (a) * rt - 8.0f, nav.y - std::cos (a) * rt - 3.0f, 16.0f, 6.0f }, 5.0f,
                   d >= 0.0f ? farg::signalRod : farg::tryck);
        }
    }
    skriv (g, "VU", { r.getCentreX() - 20.0f, r.getBottom() - 30.0f, 40.0f, 10.0f }, 8.0f, farg::tryck);
    skriv (g, namn, { r.getX() + 6.0f, r.getBottom() - 14.0f, 30.0f, 9.0f }, 6.0f, farg::tryck, juce::Justification::centredLeft);
    // Visaren
    g.saveState();
    g.reduceClipRegion (r.toNearestInt());
    const float a = vinkelFor (db);
    g.setColour (juce::Colours::black.withAlpha (0.2f));
    g.drawLine (nav.x + 2.0f, nav.y, nav.x + 2.0f + std::sin (a) * radie, nav.y - std::cos (a) * radie + 2.0f, 1.4f);
    g.setColour (farg::tryck);
    g.drawLine (nav.x, nav.y, nav.x + std::sin (a) * (radie + 4.0f), nav.y - std::cos (a) * (radie + 4.0f), 1.1f);
    g.restoreState();
    // Glas och ram
    juce::ColourGradient glans (juce::Colours::white.withAlpha (0.22f), r.getX(), r.getY(), juce::Colours::transparentWhite, r.getX(), r.getCentreY(), false);
    g.setGradientFill (glans);
    g.fillRoundedRectangle (r, 5.0f);
    kromram (g, r, 5.0f, 3.0f);
}

// Det magiska ögat: en rund avstämningsindikator med grön lysande solfjäder. oppning 1 = vid öppen
// skuggkil (människa), 0 = stängd (maskin).
inline void magisktOga (juce::Graphics& g, juce::Point<float> c, float r, float oppning)
{
    kromram (g, juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c).expanded (2.0f), r + 2.0f, 3.5f);
    g.setColour (farg::glasMorkt);
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    const float ri = r * 0.86f;
    const float kil = juce::jlimit (0.02f, 1.0f, oppning) * 0.42f * pi;   // halva skuggkilens vinkel
    // Glöden
    g.setColour (farg::oga.withAlpha (0.14f));
    g.fillEllipse (c.x - ri, c.y - ri, ri * 2, ri * 2);
    juce::Path sol;
    sol.addPieSegment (c.x - ri, c.y - ri, ri * 2, ri * 2, kil, 2.0f * pi - kil, 0.28f);
    juce::ColourGradient gr (farg::oga.brighter (0.5f), c.x, c.y, farg::oga.darker (0.6f), c.x + ri, c.y, true);
    g.setGradientFill (gr);
    g.fillPath (sol);
    // Strålar
    g.setColour (farg::ogaMorkt.withAlpha (0.35f));
    for (int i = 0; i < 24; ++i)
    {
        const float a = kil + (2.0f * pi - 2.0f * kil) * (float) i / 23.0f;
        g.drawLine (c.x + std::sin (a) * ri * 0.3f, c.y - std::cos (a) * ri * 0.3f, c.x + std::sin (a) * ri, c.y - std::cos (a) * ri, 0.5f);
    }
    // Mittkåpan
    g.setColour (juce::Colour (0xff221d16));
    g.fillEllipse (c.x - ri * 0.3f, c.y - ri * 0.3f, ri * 0.6f, ri * 0.6f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.fillEllipse (c.x - r * 0.6f, c.y - r * 0.85f, r * 0.9f, r * 0.45f);
}

// Bandmätaren: glimrör i rad bakom mörkt glas. nivaer 0..1 per band.
inline void bandmatare (juce::Graphics& g, juce::Rectangle<float> r, const float* nivaer, int antal, int aktiva, bool fryst)
{
    g.setColour (farg::glasMorkt);
    g.fillRoundedRectangle (r, 6.0f);
    auto inre = r.reduced (16.0f, 12.0f).withTrimmedBottom (12.0f);
    const float steg = inre.getWidth() / (float) antal;
    const float bw = steg * 0.46f;
    for (int i = 0; i < antal; ++i)
    {
        const float x = inre.getX() + steg * ((float) i + 0.5f);
        auto ror = juce::Rectangle<float> (x - bw * 0.5f, inre.getY(), bw, inre.getHeight());
        // Glaset
        g.setColour (juce::Colour (0xff2a2520));
        g.fillRoundedRectangle (ror.expanded (2.0f, 3.0f), bw * 0.5f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawRoundedRectangle (ror.expanded (2.0f, 3.0f), bw * 0.5f, 0.6f);
        // Den släckta katoden
        g.setColour (farg::neonSlackt);
        g.fillRect (ror.reduced (bw * 0.18f, 0.0f));
        if (i >= aktiva) continue;
        const float v = juce::jlimit (0.0f, 1.0f, nivaer[i]);
        if (v <= 0.002f) continue;
        auto tant = ror.reduced (bw * 0.18f, 0.0f).withTop (ror.getBottom() - ror.getHeight() * v);
        g.setColour (farg::neon.withAlpha (0.18f));
        g.fillRoundedRectangle (tant.expanded (bw * 0.55f, 4.0f), bw * 0.4f);
        g.setColour (farg::neon.withAlpha (0.35f));
        g.fillRoundedRectangle (tant.expanded (bw * 0.25f, 1.5f), bw * 0.3f);
        juce::ColourGradient gl (farg::neonGlod, tant.getCentreX(), tant.getY(), farg::neon, tant.getRight(), tant.getY(), false);
        g.setGradientFill (gl);
        g.fillRect (tant);
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.fillRect (tant.getCentreX() - 0.4f, tant.getY(), 0.8f, tant.getHeight());
    }
    // Frekvenser under rören (logaritmiskt 100 Hz – 8 kHz)
    for (int i = 0; i < antal; i += (antal > 12 ? 3 : 2))
    {
        const double f = 100.0 * std::pow (80.0, (double) i / (double) (antal - 1));
        const auto t = f < 1000.0 ? juce::String (juce::roundToInt (f / 10.0) * 10) : juce::String (f / 1000.0, 1).replace (".", ",") + "K";
        const float x = inre.getX() + steg * ((float) i + 0.5f);
        skriv (g, t, { x - 16.0f, r.getBottom() - 15.0f, 32.0f, 8.0f }, 5.0f, farg::neon.withAlpha (0.6f));
    }
    if (fryst) skrivGlod (g, "FRYST", { r.getRight() - 70.0f, r.getY() + 4.0f, 60.0f, 10.0f }, 6.0f, farg::neonGlod, juce::Justification::centredRight);
    glas (g, r, 6.0f);
    kromram (g, r, 6.0f, 3.0f);
}

// Tonskalan: ett bakbelyst glas med tonnamn som på en radio, och en röd visare. ton i MIDI (24–84).
inline void tonskala (juce::Graphics& g, juce::Rectangle<float> r, int ton, const juce::String& rubrik)
{
    juce::ColourGradient bg (farg::skala, r.getCentreX(), r.getCentreY(), farg::skala.darker (0.35f), r.getX(), r.getY(), true);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, 6.0f);
    auto skalan = r.reduced (14.0f, 0.0f);
    const float y0 = r.getY() + r.getHeight() * 0.38f;
    g.setColour (farg::tryck);
    g.drawLine (skalan.getX(), y0, skalan.getRight(), y0, 0.8f);
    for (int t = 24; t <= 84; ++t)
    {
        const float x = juce::jmap ((float) t, 24.0f, 84.0f, skalan.getX(), skalan.getRight());
        const bool c = t % 12 == 0;
        g.drawLine (x, y0, x, y0 + (c ? 9.0f : (t % 12 == 7 ? 6.0f : 3.5f)), c ? 1.1f : 0.6f);
        if (c) skriv (g, "C" + juce::String (t / 12 - 1), { x - 10.0f, y0 + 11.0f, 20.0f, 8.0f }, 6.0f, farg::tryck);
    }
    skriv (g, rubrik, { r.getX(), r.getY() + 7.0f, r.getWidth(), 9.0f }, 6.5f, farg::tryck);
    skriv (g, juce::String::fromUTF8 ("ORGELNS TON · STRUPENS TON"), { r.getX(), r.getBottom() - 16.0f, r.getWidth(), 8.0f }, 5.0f, farg::tryckMatt);
    // Visaren
    const float x = juce::jmap ((float) juce::jlimit (24, 84, ton), 24.0f, 84.0f, skalan.getX(), skalan.getRight());
    g.setColour (farg::signalRod.withAlpha (0.25f));
    g.fillRect (x - 2.5f, r.getY() + 4.0f, 5.0f, r.getHeight() - 8.0f);
    g.setColour (farg::signalRod);
    g.fillRect (x - 0.9f, r.getY() + 4.0f, 1.8f, r.getHeight() - 8.0f);
    glas (g, r, 6.0f);
    kromram (g, r, 6.0f, 3.0f);
}

// PÅ LUFTEN: en röd glasskylt i krom som lyser när bordet sänder.
inline void paLuften (juce::Graphics& g, juce::Rectangle<float> r, bool tand)
{
    if (tand)
    {
        g.setColour (farg::rodGlod.withAlpha (0.22f));
        g.fillRoundedRectangle (r.expanded (8.0f), 12.0f);
    }
    juce::ColourGradient gr (tand ? farg::rodGlod : farg::signalRod.darker (1.4f), r.getCentreX(), r.getCentreY(),
                             tand ? farg::signalRod.darker (0.2f) : farg::signalRod.darker (2.4f), r.getX(), r.getY(), true);
    g.setGradientFill (gr);
    g.fillRoundedRectangle (r, 5.0f);
    if (tand) skrivGlod (g, juce::String::fromUTF8 ("PÅ LUFTEN"), r, r.getHeight() * 0.32f, juce::Colour (0xfffff0e0));
    else      skriv (g, juce::String::fromUTF8 ("PÅ LUFTEN"), r, r.getHeight() * 0.32f, farg::signalRod.darker (0.6f));
    glas (g, r, 5.0f);
    kromram (g, r, 5.0f, 3.5f);
}

// Programfönstret: mörkt glas med glimrörsglödande text (nummer, namn och kategori).
inline void programfonster (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& nr, const juce::String& namn, const juce::String& kategori)
{
    g.setColour (farg::glasMorkt);
    g.fillRoundedRectangle (r, 4.0f);
    skrivGlod (g, nr, { r.getX() + 8.0f, r.getY(), 26.0f, r.getHeight() }, 10.0f, farg::neon, juce::Justification::centredLeft);
    skrivGlod (g, namn.toUpperCase(), { r.getX() + 40.0f, r.getY() + 4.0f, r.getWidth() - 48.0f, r.getHeight() * 0.55f }, 8.0f, farg::neonGlod, juce::Justification::centredLeft);
    skriv (g, kategori.toUpperCase(), { r.getX() + 40.0f, r.getBottom() - r.getHeight() * 0.42f, r.getWidth() - 48.0f, r.getHeight() * 0.35f }, 5.5f, farg::neon.withAlpha (0.6f), juce::Justification::centredLeft);
    glas (g, r, 4.0f);
    kromram (g, r, 4.0f, 2.5f);
}

} // namespace rv::stil
