// Vokodern – kroppen i Röstverket.
//
// Analysen: rösten (mono) delas i 8–20 band, logaritmiskt mellan 100 Hz och 8 kHz, med bandpass av
// fjärde ordningen (två biquads i rad). Varje band får en följare vars attack och släpp styrs av
// Snabbhet. Syntesen: bärvågen (per kanal) går genom samma bandindelning – förskjuten med Formant – och
// varje band skalas med analysens följare. Skärpa styr bandens Q.
//
// Begripligheten: förhållandet mellan rösten över 4 kHz och under 1 kHz känner igen tonlösa ljud
// (s, f, t). Då blandas rösten själv, högpassad, in – annars försvinner konsonanterna i en sågtand.
//
// FRYS håller följarna. Nivån förs tillbaka till röstens med en långsam nivåkompensation, så att
// band, bärvåg och skärpa ändrar klangen, inte volymen.
//
// Byte av bandantal tonas ut och in (12 ms), så att filtren kan nollställas utan klick.
// Allt är förallokerat – ljudtråden allokerar inget.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <krang/dsp/Grund.h>
#include <array>
#include <cmath>

namespace rv::dsp
{

class Vokodern
{
public:
    static constexpr int maxBand = 20;
    static constexpr int maxKanaler = 2;

    void forbered (double samplingsfrekvens)
    {
        fs = samplingsfrekvens;
        hfAnalys.satt (krang::dsp::Biquad::Typ::hogpass, fs, 4000.0, 0.707);
        lfAnalys.satt (krang::dsp::Biquad::Typ::lagpass, fs, 1000.0, 0.707);
        hfF.satt (fs, 0.002f, 0.040f);
        lfF.satt (fs, 0.002f, 0.040f);
        uK = krang::dsp::tidKoeff (0.004f, fs);
        for (auto& k : hfOrig) for (auto& b : k) b.satt (krang::dsp::Biquad::Typ::hogpass, fs, 4500.0, 0.707);
        for (auto& f : barvagF) f.satt (fs, 0.001f, 0.060f);
        for (auto& n : nivakomp) { n.satt (fs, 0.35f, 0.35f, 0.02f); n.minDb = -24.0f; n.maxDb = 36.0f; n.forvanta (4.0f); }
        rampSteg = 1.0f / (float) (0.012 * fs);
        antal = nyttAntal;
        rakna (true);
        nollstall();
    }

    void nollstall() noexcept
    {
        for (auto& b : analys) for (auto& f : b) f.reset();
        for (auto& k : syntes) for (auto& b : k) for (auto& f : b) f.reset();
        env.fill (0.0f);
        hfAnalys.reset(); lfAnalys.reset(); hfF.reset(); lfF.reset(); u = 0.0f;
        for (auto& k : hfOrig) for (auto& b : k) b.reset();
        for (auto& n : nivakomp) n.reset();
        for (auto& f : barvagF) f.reset();
        fade = 1.0f; vantar = false;
    }

    // antalBand 8/12/16/20, formant ±12 st, snabbhet/skärpa/begriplighet 0..1.
    void installning (int antalBand, float formantSt, float snabbhet, float skarpa, float begriplighet, bool frys) noexcept
    {
        if (antalBand != antal) { nyttAntal = antalBand; vantar = true; }
        const bool nyQ = std::abs (skarpa - skarpaNu) > 1.0e-4f;
        const bool nyF = std::abs (formantSt - formantNu) > 0.01f;
        skarpaNu = skarpa; formantNu = formantSt;
        if (nyQ || nyF) rakna (nyQ);
        // Snabbhet: 1 = attack 1 ms, släpp 12 ms (tal); 0 = 40 ms och 400 ms (mjuk yta).
        const float att = 0.001f * std::pow (40.0f, 1.0f - snabbhet);
        const float rel = 0.012f * std::pow (33.0f, 1.0f - snabbhet);
        attK = krang::dsp::tidKoeff (att, fs); relK = krang::dsp::tidKoeff (rel, fs);
        begripl = begriplighet;
        fryst = frys;
    }

    int antalBand() const noexcept { return antal; }
    float bandNiva (int i) const noexcept { return i < antal ? env[(size_t) i] : 0.0f; }

    // rost: mono, n sample. barvag/ut: per kanal. ut får vokoderns våta signal.
    void process (const float* rost, const float* const* barvag, float* const* ut, int kanaler, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            // Omkopplingen av bandantalet: tona ut, byt, tona in.
            if (vantar)
            {
                fade -= rampSteg;
                if (fade <= 0.0f) { fade = 0.0f; antal = nyttAntal; vantar = false; rakna (true); nollstallFilter(); }
            }
            else if (fade < 1.0f) fade = std::min (1.0f, fade + rampSteg);

            const float x = rost[i];
            // Analysen
            if (! fryst)
            {
                for (int b = 0; b < antal; ++b)
                {
                    const float y = std::abs (analys[(size_t) b][1].process (analys[(size_t) b][0].process (x)));
                    auto& e = env[(size_t) b];
                    e = y + (y > e ? attK : relK) * (e - y);
                }
            }
            // Tonlöst eller tonande?
            const float hf = hfF.process (hfAnalys.process (x)), lf = lfF.process (lfAnalys.process (x));
            const float kvot = hf / (hf + lf + 1.0e-6f);
            const float tonlos = std::clamp ((kvot - 0.25f) / 0.30f, 0.0f, 1.0f);
            u = tonlos + uK * (u - tonlos);
            const float konsonant = begripl * u * (fryst ? 0.0f : 1.0f);

            for (int k = 0; k < kanaler; ++k)
            {
                // Fryst: nivåkompensationen behåller röstens nivå från ögonblicket då den frös.
                if (! fryst) nivakomp[(size_t) k].matIn (x);
                const float c = barvag[k][i];
                // Konsonanterna släpps bara igenom när bärvågen klingar – utan ton är vokodern tyst.
                const float klingar = std::min (1.0f, barvagF[(size_t) k].process (c) * 20.0f);
                float summa = 0.0f;
                for (int b = 0; b < antal; ++b)
                {
                    auto& f = syntes[(size_t) k][(size_t) b];
                    summa += f[1].process (f[0].process (c)) * env[(size_t) b];
                }
                float v = nivakomp[(size_t) k].process (summa * bandVikt);
                const float hfx = hfOrig[(size_t) k][1].process (hfOrig[(size_t) k][0].process (x));
                const float kons = konsonant * klingar;
                // Omkopplingen tonas med en halv cosinus, så att varken nivån eller lutningen hoppar.
                const float f = 0.5f - 0.5f * std::cos (fade * 3.14159265f);
                ut[k][i] = (v * (1.0f - 0.5f * kons) + hfx * kons * 2.0f) * f;
            }
        }
    }

private:
    // Bandens mittfrekvenser och Q. allt = analysen också (vid nytt antal eller ny skärpa).
    void rakna (bool allt) noexcept
    {
        const double omfang = std::log2 (80.0);   // 100 Hz – 8 kHz
        const double steg = omfang / (double) (antal - 1);
        const double bw = steg * (1.5 - 0.95 * (double) skarpaNu);        // bandbredd i oktaver
        const double p = std::pow (2.0, bw);
        const double q = std::sqrt (p) / (p - 1.0);
        const double skift = std::pow (2.0, (double) formantNu / 12.0);
        for (int b = 0; b < antal; ++b)
        {
            const double f = 100.0 * std::pow (2.0, steg * (double) b);
            if (allt) for (auto& bq : analys[(size_t) b]) bq.satt (krang::dsp::Biquad::Typ::bandpass, fs, f, q);
            for (auto& k : syntes) for (auto& bq : k[(size_t) b]) bq.satt (krang::dsp::Biquad::Typ::bandpass, fs, std::min (f * skift, fs * 0.45), q);
        }
        // Färre och smalare band ger mindre energi; vikten ger ungefär samma nivå före nivåkompensationen.
        bandVikt = (float) (3.0 * std::sqrt (20.0 / (double) antal) * (1.0 + 1.5 * (double) skarpaNu));
    }

    void nollstallFilter() noexcept
    {
        for (auto& b : analys) for (auto& f : b) f.reset();
        for (auto& k : syntes) for (auto& b : k) for (auto& f : b) f.reset();
        env.fill (0.0f);
    }

    double fs = 48000.0;
    int antal = 20, nyttAntal = 20;
    bool vantar = false, fryst = false;
    float fade = 1.0f, rampSteg = 0.01f;
    float skarpaNu = 0.5f, formantNu = 0.0f, attK = 0.0f, relK = 0.0f, begripl = 0.5f, bandVikt = 1.0f;
    std::array<std::array<krang::dsp::Biquad, 2>, maxBand> analys {};
    std::array<std::array<std::array<krang::dsp::Biquad, 2>, maxBand>, maxKanaler> syntes {};
    std::array<float, maxBand> env {};
    krang::dsp::Biquad hfAnalys, lfAnalys;
    krang::dsp::Foljare hfF, lfF;
    float u = 0.0f, uK = 0.0f;
    std::array<std::array<krang::dsp::Biquad, 2>, maxKanaler> hfOrig {};
    std::array<krang::dsp::Nivakomp, maxKanaler> nivakomp {};
    std::array<krang::dsp::Foljare, maxKanaler> barvagF {};
};

} // namespace rv::dsp
