// Sändarbordet som bild: tillståndet, geometrin och ritningen, delade av stilprovet och den riktiga panelen.
//
// panel::Tillstand – allt som syns (värden, texter, lampor, mätare, bandmätaren).
// panel::geo       – var allt sitter, så att panelen och (från fas 1) musträffarna aldrig glider isär.
// panel::bakgrund  – det som inte rör sig: valnöten, lacken, duken, skyltarna och modulfälten.
// panel::reglage   – rattar, vippor, lampor och instrument ovanpå.
// panel::Cache     – bakgrunden cachad per fysisk skala.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <vector>
#include "Radiohuset.h"

namespace rv::stil::panel
{

constexpr int bredd = 1200, hojd = 760;

// ---------------------------------------------------------------------------------------------
// Tillståndet

struct Del
{
    enum class Typ { ratt, valjare, vippa };
    Typ typ = Typ::ratt;
    juce::String namn;
    float varde = 0.0f;              // ratt 0..1
    juce::String text;               // ratt: värdet i klartext
    juce::StringArray lagen;         // väljare
    int lage = 0;
    bool pa = false;                 // vippa
    juce::String pid;                // parametern som reglaget styr (tom i stilprovet)

    static Del ratt (const juce::String& n, float v, const juce::String& t)                  { Del d; d.typ = Typ::ratt; d.namn = n; d.varde = v; d.text = t; return d; }
    static Del valjare (const juce::String& n, const juce::StringArray& l, int i)            { Del d; d.typ = Typ::valjare; d.namn = n; d.lagen = l; d.lage = i; return d; }
    static Del vippa (const juce::String& n, bool p)                                         { Del d; d.typ = Typ::vippa; d.namn = n; d.pa = p; return d; }
};

struct Rad { juce::String rubrik; std::vector<Del> delar; };

struct Modul
{
    int nr = 1;
    juce::String namn;
    bool pa = false;
    bool ejInkopplad = false;        // modulens ljud kommer i en senare fas
    std::vector<Rad> rader;
    juce::String paPid;              // modulens huvudbrytare
};

struct Tillstand
{
    std::array<Modul, 6> moduler;
    float omvandlingen = 0.0f; juce::String omvandlingenText;
    float inDb = -120.0f, utDb = -120.0f;
    bool paLuften = true;
    std::array<float, 20> band {};
    int bandAktiva = 20;
    bool fryst = false;
    int ton = 48; juce::String tonRubrik;
    juce::String programNr = "01", programNamn = "Init", kategori = "Grund";
    std::array<Del, 3> utgang;       // in-nivå, torr/våt, utnivå
    bool limiterPa = true; float limiterLampa = 0.0f;
    bool midiIn = false;
};

// ---------------------------------------------------------------------------------------------
// Geometrin

namespace geo
{
    inline const juce::Rectangle<float> panel     { 20.0f, 16.0f, 1160.0f, 728.0f };
    inline const juce::Rectangle<float> duk       { 40.0f, 32.0f, 112.0f, 104.0f };
    inline const juce::Rectangle<float> skylt     { 168.0f, 40.0f, 300.0f, 88.0f };
    inline const juce::Rectangle<float> vuIn      { 492.0f, 36.0f, 132.0f, 94.0f };
    inline const juce::Rectangle<float> vuUt      { 636.0f, 36.0f, 132.0f, 94.0f };
    inline const juce::Point<float>     oga       { 826.0f, 78.0f };
    inline const float                  ogaRadie  = 34.0f;
    inline const juce::Point<float>     spaken    { 912.0f, 78.0f };
    inline const juce::Rectangle<float> paLuften  { 990.0f, 44.0f, 166.0f, 64.0f };
    inline const juce::Rectangle<float> band      { 40.0f, 150.0f, 800.0f, 112.0f };
    inline const juce::Rectangle<float> tonskala  { 856.0f, 150.0f, 300.0f, 112.0f };
    inline const std::array<juce::Rectangle<float>, 6> modul {{
        { 40.0f, 274.0f, 330.0f, 194.0f }, { 380.0f, 274.0f, 440.0f, 194.0f }, { 830.0f, 274.0f, 326.0f, 194.0f },
        { 40.0f, 476.0f, 330.0f, 194.0f }, { 380.0f, 476.0f, 440.0f, 194.0f }, { 830.0f, 476.0f, 326.0f, 194.0f } }};
    inline const juce::Rectangle<float> program   { 40.0f, 682.0f, 220.0f, 44.0f };
    inline const std::array<juce::Rectangle<float>, 4> knappar {{
        { 270.0f, 690.0f, 30.0f, 26.0f }, { 304.0f, 690.0f, 30.0f, 26.0f }, { 342.0f, 690.0f, 58.0f, 26.0f }, { 406.0f, 690.0f, 54.0f, 26.0f } }};
    inline const std::array<juce::Point<float>, 3> utgang {{ { 560.0f, 708.0f }, { 690.0f, 708.0f }, { 820.0f, 708.0f } }};
    inline const juce::Point<float> limiter { 900.0f, 712.0f };
    inline const juce::Point<float> midi    { 960.0f, 712.0f };

    inline juce::Rectangle<float> modulHuvud (int i) { return modul[(size_t) i].withHeight (28.0f); }
    inline juce::Rectangle<float> modulInnehall (int i) { return modul[(size_t) i].withTrimmedTop (32.0f).reduced (6.0f, 4.0f); }
}

// ---------------------------------------------------------------------------------------------
// Bakgrunden

inline void bakgrund (juce::Graphics& g, const Tillstand& t, float skala)
{
    // Bordet i valnöt
    {
        auto bild = valnotsbild (bredd, hojd, skala, 1983);
        g.drawImageTransformed (bild, juce::AffineTransform::scale (1.0f / skala));
    }
    // Panelen i kittgrå lack, infälld med mässingslist
    {
        const auto p = geo::panel;
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (p.expanded (3.0f).translated (0.0f, 2.0f), 9.0f);
        juce::Path form; form.addRoundedRectangle (p, 8.0f);
        auto lack = lackbild ((int) p.getWidth(), (int) p.getHeight(), skala, farg::kitt, 1958);
        krang::stil::fyllMedTextur (g, form, lack, p, skala);
        juce::ColourGradient ljus (juce::Colours::white.withAlpha (0.10f), p.getX(), p.getY(), juce::Colours::black.withAlpha (0.12f), p.getX(), p.getBottom(), false);
        g.setGradientFill (ljus);
        g.fillPath (form);
        juce::ColourGradient list (farg::massingLjus, p.getX(), p.getY(), farg::massingMork, p.getRight(), p.getBottom(), false);
        g.setGradientFill (list);
        g.strokePath (form, juce::PathStrokeType (3.0f));
        for (auto c : { p.getTopLeft(), p.getTopRight(), p.getBottomLeft(), p.getBottomRight() })
            skruv (g, { c.x + (c.x < 600.0f ? 9.0f : -9.0f), c.y + (c.y < 380.0f ? 9.0f : -9.0f) }, 3.4f, c.x * 0.01f + c.y * 0.02f);
    }
    // Duken och modellskylten
    duk (g, geo::duk);
    {
        const auto s = geo::skylt;
        massingsplatta (g, s, 5.0f);
        praglat (g, juce::String::fromUTF8 ("RÖSTVERKET"), s.withTrimmedTop (14.0f).withHeight (30.0f).withTrimmedLeft (50.0f), 20.0f, farg::massingMork.darker (0.6f));
        praglat (g, "RV-83", s.withTrimmedTop (52.0f).withHeight (16.0f).withTrimmedLeft (50.0f), 10.0f, farg::massingMork.darker (0.6f));
        mikrofon (g, { s.getX() + 14.0f, s.getY() + 16.0f, 34.0f, 58.0f }, farg::massingMork.darker (0.6f));
        for (auto c : { juce::Point<float> { s.getX() + 7.0f, s.getY() + 7.0f }, { s.getRight() - 7.0f, s.getY() + 7.0f },
                        { s.getX() + 7.0f, s.getBottom() - 7.0f }, { s.getRight() - 7.0f, s.getBottom() - 7.0f } })
            skruv (g, c, 2.6f, c.x * 0.3f, true);
    }
    // Text i toppen
    skriv (g, "OMVANDLINGEN", { geo::oga.x - 50.0f, 122.0f, 186.0f, 9.0f }, 7.0f, farg::tryck);
    skriv (g, juce::String::fromUTF8 ("MÄNNISKA"), { geo::spaken.x - 52.0f, 104.0f, 40.0f, 7.0f }, 5.0f, farg::tryckMatt);
    skriv (g, "MASKIN", { geo::spaken.x + 14.0f, 104.0f, 40.0f, 7.0f }, 5.0f, farg::tryckMatt);

    // Modulfälten
    for (int i = 0; i < 6; ++i)
    {
        const auto& m = t.moduler[(size_t) i];
        const auto r = geo::modul[(size_t) i];
        juce::Path f; f.addRoundedRectangle (r, 5.0f);
        g.setColour (farg::kittMork.withAlpha (0.55f));
        g.strokePath (f, juce::PathStrokeType (2.0f));
        g.setColour (farg::kittModul.withAlpha (0.55f));
        g.fillPath (f);
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.drawLine (r.getX() + 5.0f, r.getY() + 1.0f, r.getRight() - 5.0f, r.getY() + 1.0f, 0.8f);
        const auto h = geo::modulHuvud (i);
        plakett (g, { h.getX() + 6.0f, h.getY() + 6.0f, juce::jmin (190.0f, h.getWidth() - 110.0f), 20.0f },
                 juce::String (m.nr) + "  " + m.namn.toUpperCase(), 8.0f);
        // Radernas rubriker och skiljelinjer
        const auto inn = geo::modulInnehall (i);
        const int n = (int) m.rader.size();
        for (int ri = 0; ri < n; ++ri)
        {
            const auto rr = inn.withHeight (inn.getHeight() / (float) n).translated (0.0f, inn.getHeight() / (float) n * (float) ri);
            if (ri > 0)
            {
                g.setColour (farg::kittMork.withAlpha (0.6f));
                g.drawLine (rr.getX() + 6.0f, rr.getY(), rr.getRight() - 6.0f, rr.getY(), 0.7f);
                g.setColour (juce::Colours::white.withAlpha (0.3f));
                g.drawLine (rr.getX() + 6.0f, rr.getY() + 0.8f, rr.getRight() - 6.0f, rr.getY() + 0.8f, 0.6f);
            }
            if (m.rader[(size_t) ri].rubrik.isNotEmpty())
            {
                const auto rub = m.rader[(size_t) ri].rubrik.toUpperCase();
                const float b = ls::bredd (rub, text (5.5f)) + 10.0f;
                auto ruta = juce::Rectangle<float> (rr.getX() + 4.0f, rr.getY() + 2.0f, b, 9.0f);
                g.setColour (farg::kittModul);
                g.fillRect (ruta);
                skriv (g, rub, ruta, 5.5f, farg::tryck);
            }
        }
    }
    // Nederkanten
    skriv (g, "PROGRAM", { geo::program.getX(), geo::program.getY() - 10.0f, 80.0f, 8.0f }, 5.5f, farg::tryck, juce::Justification::centredLeft);
    skriv (g, "LIMITER", { geo::limiter.x - 30.0f, geo::limiter.y - 24.0f, 60.0f, 8.0f }, 5.5f, farg::tryck);
    skriv (g, "MIDI IN", { geo::midi.x - 30.0f, geo::midi.y - 24.0f, 60.0f, 8.0f }, 5.5f, farg::tryck);
    skriv (g, juce::String::fromUTF8 ("SÄNDARBORD RV-83 · RÖST IN · MASKIN UT"), { 980.0f, 700.0f, 176.0f, 9.0f }, 5.0f, farg::tryckMatt, juce::Justification::centredRight);
    skriv (g, juce::String::fromUTF8 ("TILLV. NR 0001"), { 980.0f, 712.0f, 176.0f, 9.0f }, 5.0f, farg::tryckMatt, juce::Justification::centredRight);
}

// ---------------------------------------------------------------------------------------------
// Reglagen och instrumenten

// Var ett reglage sitter i sin cell, och dess radie (samma för ritning och musträffar).
inline juce::Point<float> centrum (const Del& d, juce::Rectangle<float> cell)
{
    return { cell.getCentreX(), cell.getCentreY() + (d.typ == Del::Typ::valjare ? 5.0f : 1.0f) };
}
inline float radie (const Del& d, float r) { return d.typ == Del::Typ::valjare ? r * 0.8f : (d.typ == Del::Typ::vippa ? 9.0f : r); }

inline juce::Point<float> modulVippa (int i) { const auto h = geo::modulHuvud (i); return { h.getRight() - 24.0f, h.getCentreY() + 3.0f }; }

// Går igenom modulernas reglage med cell och radie: f (modul, del, cell, r).
template <typename F>
void forVarjeDel (const Tillstand& t, F&& f)
{
    for (int i = 0; i < 6; ++i)
    {
        const auto& m = t.moduler[(size_t) i];
        const auto inn = geo::modulInnehall (i);
        const int n = (int) m.rader.size();
        const float r = n > 1 ? 13.5f : 20.0f;
        for (int ri = 0; ri < n; ++ri)
        {
            const auto rr = inn.withHeight (inn.getHeight() / (float) n).translated (0.0f, inn.getHeight() / (float) n * (float) ri);
            const auto& delar = m.rader[(size_t) ri].delar;
            const float w = rr.getWidth() / (float) juce::jmax<size_t> (1, delar.size());
            for (size_t k = 0; k < delar.size(); ++k)
                f (i, delar[k], rr.withX (rr.getX() + w * (float) k).withWidth (w).withTrimmedTop (n > 1 ? 6.0f : 0.0f), r);
        }
    }
}

inline void del (juce::Graphics& g, const Del& d, juce::Rectangle<float> cell, float r)
{
    const auto namn = d.namn.toUpperCase();
    switch (d.typ)
    {
        case Del::Typ::ratt:
        {
            const auto c = centrum (d, cell);
            skriv (g, namn, { cell.getX(), c.y - r * 1.55f - 11.0f, cell.getWidth(), 8.0f }, 5.6f, farg::tryck);
            ratt (g, c, r, d.varde, true);
            skriv (g, d.text, { cell.getX(), c.y + r * 1.4f + 1.0f, cell.getWidth(), 8.0f }, 5.2f, farg::varde);
            break;
        }
        case Del::Typ::valjare:
        {
            const float rv = radie (d, r);
            const auto c = centrum (d, cell);
            valjare (g, c, rv, d.lagen, d.lage, 5.0f);
            skriv (g, namn, { cell.getX(), c.y + rv * 1.25f + 3.0f, cell.getWidth(), 8.0f }, 5.6f, farg::tryck);
            break;
        }
        case Del::Typ::vippa:
        {
            const auto c = centrum (d, cell);
            skriv (g, namn, { cell.getX(), c.y - 36.0f, cell.getWidth(), 8.0f }, 5.6f, farg::tryck);
            skriv (g, juce::String::fromUTF8 ("PÅ"), { c.x - 10.0f, c.y - 24.0f, 20.0f, 7.0f }, 5.0f, d.pa ? farg::tryck : farg::tryckMatt);
            skriv (g, "AV", { c.x - 10.0f, c.y + 17.0f, 20.0f, 7.0f }, 5.0f, d.pa ? farg::tryckMatt : farg::tryck);
            vippa (g, c, 14.0f, d.pa);
            break;
        }
    }
}

inline void reglage (juce::Graphics& g, const Tillstand& t)
{
    // Instrumenten i toppen
    vu (g, geo::vuIn, t.inDb, "IN");
    vu (g, geo::vuUt, t.utDb, "UT");
    magisktOga (g, geo::oga, geo::ogaRadie, 1.0f - t.omvandlingen);
    ratt (g, geo::spaken, 22.0f, t.omvandlingen, false);
    skriv (g, t.omvandlingenText, { geo::spaken.x - 30.0f, geo::spaken.y + 28.0f, 60.0f, 7.0f }, 5.0f, farg::varde);
    paLuften (g, geo::paLuften, t.paLuften);
    bandmatare (g, geo::band, t.band.data(), 20, t.bandAktiva, t.fryst);
    tonskala (g, geo::tonskala, t.ton, t.tonRubrik);

    // Modulerna
    for (int i = 0; i < 6; ++i)
    {
        const auto& m = t.moduler[(size_t) i];
        const auto h = geo::modulHuvud (i);
        lampa (g, { h.getRight() - 54.0f, h.getCentreY() + 3.0f }, 4.5f, farg::barnsten, m.pa && ! m.ejInkopplad ? 1.0f : 0.0f);
        vippa (g, modulVippa (i), 12.0f, m.pa);
        if (m.ejInkopplad)
        {
            auto st = juce::Rectangle<float> (h.getX() + 202.0f, h.getY() + 8.0f, juce::jmin (150.0f, h.getWidth() - 268.0f), 16.0f);
            if (st.getWidth() > 54.0f)
            {
                g.setColour (farg::signalRod.withAlpha (0.85f));
                g.drawRoundedRectangle (st, 2.0f, 1.2f);
                skriv (g, "EJ INKOPPLAD", st, 6.0f, farg::signalRod);
            }
        }
    }
    forVarjeDel (t, [&] (int, const Del& d, juce::Rectangle<float> cell, float r) { del (g, d, cell, r); });

    // Nederkanten
    programfonster (g, geo::program, t.programNr, t.programNamn, t.kategori);
    const char* kt[] { "\xe2\x97\x80", "\xe2\x96\xb6", "SPARA", "MENY" };
    for (int i = 0; i < 4; ++i) knapp (g, geo::knappar[(size_t) i], juce::String::fromUTF8 (kt[i]));
    for (int i = 0; i < 3; ++i)
    {
        const auto& d = t.utgang[(size_t) i];
        const auto c = geo::utgang[(size_t) i];
        skriv (g, d.namn.toUpperCase(), { c.x - 70.0f, c.y - 4.0f, 46.0f, 8.0f }, 5.5f, farg::tryck, juce::Justification::centredRight);
        ratt (g, c, 13.0f, d.varde, true);
        skriv (g, d.text, { c.x + 24.0f, c.y - 4.0f, 48.0f, 8.0f }, 5.2f, farg::varde, juce::Justification::centredLeft);
    }
    lampa (g, geo::limiter, 5.0f, farg::signalRod, t.limiterPa ? juce::jmax (0.15f, t.limiterLampa) : 0.0f);
    vippa (g, geo::midi, 12.0f, t.midiIn);
}

inline void rita (juce::Graphics& g, const Tillstand& t)
{
    bakgrund (g, t, krang::stil::fysiskSkala (g));
    reglage (g, t);
}

// Bakgrunden cachad per fysisk skala. Bygg om med 'ogiltig()' om modulernas namn eller rader ändras.
class Cache
{
public:
    void rita (juce::Graphics& g, const Tillstand& t)
    {
        const float skala = krang::stil::fysiskSkala (g);
        if (! bild.isValid() || std::abs (skala - bildSkala) > 0.01f)
        {
            bildSkala = skala;
            bild = juce::Image (juce::Image::RGB, (int) std::ceil (bredd * skala), (int) std::ceil (hojd * skala), true);
            juce::Graphics bg (bild);
            bg.addTransform (juce::AffineTransform::scale (skala));
            bakgrund (bg, t, skala);
        }
        g.drawImageTransformed (bild, juce::AffineTransform::scale (1.0f / bildSkala));
        reglage (g, t);
    }
    void ogiltig() { bild = {}; }

private:
    juce::Image bild;
    float bildSkala = 0.0f;
};

} // namespace rv::stil::panel
