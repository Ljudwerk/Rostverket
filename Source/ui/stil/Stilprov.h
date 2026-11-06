// Stilprovet: sändarbordet med fasta exempelvärden (prov::Panel), och närbilder av reglagen och
// instrumenten (prov::Detaljer). Använder varken processorn eller parametrarna – bara bilder.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Panelbild.h"

namespace rv::stil::prov
{
using panel::Del;

inline juce::String u (const char* s) { return juce::String::fromUTF8 (s); }

// Ett sändarbord mitt i en paroll: vokodern på 20 band med Orgeln, Strupen en oktav ned, ekot på.
inline panel::Tillstand exempel()
{
    panel::Tillstand t;
    auto& m = t.moduler;
    m[0] = { 1, "Porten", true, false, { { "", {
        Del::ratt (u ("Tröskel"), 0.25f, u ("−60 dB")), Del::ratt ("Kompr.", 0.3f, "30 %"), Del::ratt (u ("Högpass"), 0.28f, "80 Hz"),
        Del::ratt (u ("Närhet"), 0.3f, "30 %"), Del::vippa ("Avsib.", false) } } } , {} };
    m[1] = { 2, "Vokodern", true, false, {
        { "", { Del::valjare ("Band", { "8", "12", "16", "20" }, 3), Del::valjare (u ("Bärvåg"), { "Orgeln", "Sidokedja", "Brus" }, 0),
                Del::ratt ("Formant", 0.5f, "0 st"), Del::ratt ("Snabbhet", 0.6f, "60 %"), Del::ratt (u ("Skärpa"), 0.5f, "50 %"),
                Del::ratt ("Begripl.", 0.5f, "50 %") } },
        { "Orgeln", { Del::valjare (u ("Våg"), { u ("Såg"), "Puls", u ("S+P") }, 0), Del::valjare ("Oktav", { u ("−2"), u ("−1"), "0", "+1" }, 2),
                      Del::ratt ("Glid", 0.15f, "20 ms"), Del::ratt ("Brus", 0.1f, "10 %"), Del::vippa ("Frys", false),
                      Del::ratt ("Blandning", 1.0f, "100 %") } } } , {} };
    m[2] = { 3, "Strupen", true, false, {
        { "", { Del::valjare (u ("Läge"), { "Skift", "Oktav", "Monoton" }, 1), Del::ratt (u ("Tonhöjd"), 0.0f, u ("−12 st")), Del::ratt ("Formant", 0.5f, "0 st") } },
        { "", { Del::ratt (u ("Följsamhet"), 0.8f, "80 %"), Del::ratt ("Blandning", 0.5f, "50 %"), Del::ratt ("Ton", 0.4f, "C3") } } } , {} };
    m[3] = { 4, u ("Sändaren"), false, false, {
        { "", { Del::valjare (u ("Läge"), { "Talchip", "Ring", u ("Kortvåg") }, 0), Del::ratt ("Styrka", 0.5f, "50 %"), Del::ratt ("Frekvens", 0.82f, "8,0 kHz") } },
        { "", { Del::ratt ("Ton", 0.5f, "50 %"), Del::ratt ("Blandning", 1.0f, "100 %"), Del::vippa ("MIDI", false) } } } , {} };
    m[4] = { 5, u ("Kören"), false, false, { { "", {
        Del::valjare (u ("Läge"), { "Dubbel", u ("Kör"), "Massa" }, 0), Del::ratt ("Spridning", 0.4f, "40 %"), Del::ratt ("Svaj", 0.3f, "30 %"),
        Del::ratt ("Bredd", 0.7f, "70 %"), Del::ratt ("Blandning", 0.5f, "50 %") } } } , {} };
    m[5] = { 6, "Ekot", true, false, {
        { "", { Del::valjare ("Tid", { "1/16", "1/8T", "1/8", "1/8.", "1/4", "1/4.", "1/2" }, 3), Del::ratt (u ("Återkoppl."), 0.37f, "35 %"),
                Del::ratt (u ("Mörker"), 0.5f, "50 %") } },
        { "", { Del::ratt ("Rum", 0.2f, "20 %"), Del::ratt ("Blandning", 0.25f, "25 %"), Del::vippa ("Ping-pong", true) } } } , {} };

    t.omvandlingen = 0.45f; t.omvandlingenText = "45 %";
    t.inDb = -6.5f; t.utDb = -3.0f;
    t.paLuften = true;
    // En vokal med formanter kring 500 Hz, 1,5 kHz och 2,5 kHz, och lite sibilans i toppen.
    for (int i = 0; i < 20; ++i)
    {
        const float f = 100.0f * std::pow (80.0f, (float) i / 19.0f);
        auto topp = [f] (float fc, float bw, float h) { const float d = std::log2 (f / fc) / bw; return h * std::exp (-d * d); };
        t.band[(size_t) i] = juce::jlimit (0.0f, 1.0f, 0.08f + topp (500.0f, 0.6f, 0.9f) + topp (1500.0f, 0.5f, 0.65f) + topp (2600.0f, 0.4f, 0.5f) + topp (7000.0f, 0.3f, 0.3f));
    }
    t.ton = 48; t.tonRubrik = "ORGELN C3";
    t.programNr = "07"; t.programNamn = "Parollen"; t.kategori = "Paroll";
    t.utgang = { Del::ratt (u ("In-nivå"), 0.5f, "0,0 dB"), Del::ratt (u ("Torr/våt"), 1.0f, "100 %"), Del::ratt (u ("Utnivå"), 0.66f, "0,0 dB") };
    t.limiterPa = true; t.limiterLampa = 0.6f;
    t.midiIn = true;
    return t;
}

class Panel : public juce::Component
{
public:
    static constexpr int bredd = panel::bredd, hojd = panel::hojd;
    void paint (juce::Graphics& g) override { panel::rita (g, exempel()); }
};

// Närbilder: reglagen i tre lägen, instrumenten i flera lägen, i dubbel storlek på lacken.
class Detaljer : public juce::Component
{
public:
    static constexpr int bredd = 1200, hojd = 720;
    void paint (juce::Graphics& g) override
    {
        const float skala = krang::stil::fysiskSkala (g);
        juce::Rectangle<float> yta (0.0f, 0.0f, (float) bredd, (float) hojd);
        juce::Path form; form.addRectangle (yta);
        krang::stil::fyllMedTextur (g, form, lackbild (bredd, hojd, skala, farg::kitt, 7), yta, skala);

        auto rubrik = [&] (const char* t, float y) { skriv (g, u (t), { 30.0f, y, 400.0f, 10.0f }, 8.0f, farg::tryck, juce::Justification::centredLeft); };

        rubrik ("RATTEN 0 · 5 · 10", 24.0f);
        for (int i = 0; i < 3; ++i) ratt (g, { 90.0f + 130.0f * (float) i, 110.0f }, 34.0f, 0.5f * (float) i, true);
        for (int i = 0; i < 3; ++i)
            valjare (g, { 530.0f + 150.0f * (float) i, 115.0f }, 24.0f, { "Talchip", "Ring", u ("Kortvåg") }, i, 6.5f);
        skriv (g, u ("VRIDOMKOPPLAREN"), { 470.0f, 24.0f, 400.0f, 10.0f }, 8.0f, farg::tryck, juce::Justification::centredLeft);
        skriv (g, u ("VIPPAN OCH LAMPAN"), { 880.0f, 24.0f, 300.0f, 10.0f }, 8.0f, farg::tryck, juce::Justification::centredLeft);
        vippa (g, { 920.0f, 110.0f }, 28.0f, false);
        vippa (g, { 990.0f, 110.0f }, 28.0f, true);
        lampa (g, { 1060.0f, 90.0f }, 9.0f, farg::barnsten, 0.0f);
        lampa (g, { 1110.0f, 90.0f }, 9.0f, farg::barnsten, 1.0f);
        lampa (g, { 1060.0f, 140.0f }, 9.0f, farg::signalRod, 0.0f);
        lampa (g, { 1110.0f, 140.0f }, 9.0f, farg::signalRod, 1.0f);

        rubrik ("DET MAGISKA ÖGAT – MÄNNISKA · MITTEMELLAN · MASKIN", 200.0f);
        for (int i = 0; i < 3; ++i) magisktOga (g, { 90.0f + 130.0f * (float) i, 290.0f }, 48.0f, 1.0f - 0.5f * (float) i);
        skriv (g, u ("VU-MÄTAREN"), { 470.0f, 200.0f, 300.0f, 10.0f }, 8.0f, farg::tryck, juce::Justification::centredLeft);
        vu (g, { 470.0f, 225.0f, 200.0f, 140.0f }, -20.0f, "IN");
        vu (g, { 690.0f, 225.0f, 200.0f, 140.0f }, 1.5f, "UT");
        skriv (g, juce::String::fromUTF8 ("PÅ LUFTEN – SLÄCKT OCH TÄND"), { 920.0f, 200.0f, 260.0f, 10.0f }, 8.0f, farg::tryck, juce::Justification::centredLeft);
        paLuften (g, { 930.0f, 230.0f, 220.0f, 56.0f }, false);
        paLuften (g, { 930.0f, 304.0f, 220.0f, 56.0f }, true);

        rubrik ("BANDMÄTAREN – 12 BAND I TAL · 20 BAND FRUSNA", 400.0f);
        std::array<float, 20> a {}, b {};
        for (int i = 0; i < 20; ++i)
        {
            a[(size_t) i] = 0.5f + 0.45f * std::sin ((float) i * 0.9f);
            b[(size_t) i] = 0.3f + 0.6f * std::exp (-std::pow (((float) i - 6.0f) / 3.0f, 2.0f));
        }
        bandmatare (g, { 30.0f, 420.0f, 560.0f, 140.0f }, a.data(), 20, 12, false);
        bandmatare (g, { 610.0f, 420.0f, 560.0f, 140.0f }, b.data(), 20, 20, true);

        rubrik ("TONSKALAN, PROGRAMMET OCH KNAPPARNA", 590.0f);
        tonskala (g, { 30.0f, 610.0f, 420.0f, 96.0f }, 55, "STRUPEN G3");
        programfonster (g, { 480.0f, 630.0f, 300.0f, 56.0f }, "12", u ("Kör i tunneln"), u ("Kör"));
        const char* kt[] { "\xe2\x97\x80", "\xe2\x96\xb6", "SPARA", "MENY" };
        for (int i = 0; i < 4; ++i)
            knapp (g, { 810.0f + 92.0f * (float) i, 640.0f, 80.0f, 36.0f }, juce::String::fromUTF8 (kt[i]), i == 1);
    }
};

} // namespace rv::stil::prov
