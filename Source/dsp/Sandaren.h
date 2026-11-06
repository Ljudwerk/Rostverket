// Sändaren – talchippet, ringen och kortvågen.
//
// TALCHIP: sample-and-hold ned till Frekvens (4–16 kHz) utan filter före, och 8 till 4 bitar (Styrka).
//          Spegelfrekvenserna är meningen: det är datorrösten från ett talchip.
// RING:    ringmodulator på Frekvens (20–2000 Hz), eller på senaste MIDI-ton när MIDI-brytaren är på.
//          Styrka blandar från amplitudmodulering (en del av rösten kvar) till ren ring.
// KORTVÅG: ett smalt band (Ton flyttar det, Styrka gör det smalare), långsam fädning, en heterodyn-
//          vissling på Frekvens (300–4000 Hz) och brus – rösten från andra sidan.
//
// Ton är ett lågpass (2–16 kHz) efter TALCHIP och RING. Nivån hålls med nivåkompensation.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <krang/dsp/Grund.h>
#include <array>
#include <cmath>

namespace rv::dsp
{

class Sandaren
{
public:
    static constexpr int maxKanaler = 2;
    enum Lage { talchip = 0, ring = 1, kortvag = 2 };

    void forbered (double samplingsfrekvens)
    {
        fs = samplingsfrekvens;
        fadK = krang::dsp::tidKoeff (0.35f, fs);
        for (auto& n : nivakomp) { n.satt (fs, 0.3f, 0.3f, 0.01f); n.minDb = -12.0f; n.maxDb = 18.0f; }
        nu = {}; nollstall();
    }

    void nollstall() noexcept
    {
        for (auto& k : kan) { k.hall = 0.0f; k.fas = 0.0f; for (auto& b : k.band) b.reset(); for (auto& b : k.radio) b.reset(); k.lp.reset(); }
        for (auto& n : nivakomp) n.reset();
        ringFas = 0.0; visselFas = 0.0; fad = 1.0f; fadMal = 1.0f; fadRakn = 0;
    }

    struct Param { int lage = 0; float styrka = 0.5f, frekvens = 8000.0f, ton = 0.5f; float midiFrek = 0.0f; };

    void installning (const Param& p) noexcept
    {
        const bool nyttFilter = p.lage != nu.lage || std::abs (p.ton - nu.ton) > 1.0e-3f || std::abs (p.styrka - nu.styrka) > 1.0e-3f;
        nu = p;
        if (nyttFilter || ! forst) rakna();
        forst = true;
    }

    void process (float* const* ljud, int kanaler, int n) noexcept
    {
        const float sh = (float) std::clamp ((double) nu.frekvens, 4000.0, 16000.0) / (float) fs;   // TALCHIP: steg per sample
        const float bitar = 8.0f - 4.0f * nu.styrka;
        const float niv = std::pow (2.0f, bitar - 1.0f);
        const double ringF = nu.midiFrek > 0.0f ? (double) nu.midiFrek : std::clamp ((double) nu.frekvens, 20.0, 2000.0);
        const double visselF = std::clamp ((double) nu.frekvens, 300.0, 4000.0);
        for (int i = 0; i < n; ++i)
        {
            // Gemensamma oscillatorer
            ringFas += ringF / fs; if (ringFas >= 1.0) ringFas -= 1.0;
            visselFas += visselF / fs; if (visselFas >= 1.0) visselFas -= 1.0;
            const float ringV = (float) std::sin (2.0 * krang::dsp::pi * ringFas);
            if (nu.lage == kortvag)
            {
                // Fädningen: ett nytt slumpmål var 0,4–1,2 s, mjukt.
                if (--fadRakn <= 0) { fadMal = 0.35f + 0.65f * br.enhet(); fadRakn = (int) ((0.4 + 0.8 * br.enhet()) * fs); }
                fad = fadMal + fadK * (fad - fadMal);
            }
            for (int k = 0; k < kanaler; ++k)
            {
                auto& kk = kan[(size_t) k];
                const float x = ljud[k][i];
                nivakomp[(size_t) k].matIn (x);
                float y = x;
                switch (nu.lage)
                {
                    case talchip:
                    {
                        kk.fas += sh;
                        if (kk.fas >= 1.0f) { kk.fas -= 1.0f; kk.hall = std::round (x * niv) / niv; }
                        y = kk.lp.process (kk.hall);
                        break;
                    }
                    case ring:
                    {
                        const float am = 0.5f + 0.5f * ringV;
                        y = kk.lp.process (x * ((1.0f - nu.styrka) * am + nu.styrka * ringV));
                        break;
                    }
                    default:
                    {
                        y = kk.band[1].process (kk.band[0].process (x)) * fad;
                        y += 0.02f * nu.styrka * (float) std::sin (2.0 * krang::dsp::pi * visselFas);
                        y += 0.008f * br.vitt();
                        y = std::tanh (y * 3.0f) * 0.7f;
                        y = kk.radio[1].process (kk.radio[0].process (y));   // mottagarens bandbredd
                        break;
                    }
                }
                ljud[k][i] = nivakomp[(size_t) k].process (y);
            }
        }
    }

private:
    void rakna() noexcept
    {
        const double lpHz = 2000.0 * std::pow (8.0, (double) nu.ton);
        for (auto& k : kan)
        {
            k.lp.satt (krang::dsp::Biquad::Typ::lagpass, fs, std::min (lpHz, fs * 0.45), 0.707);
            const double mitt = 700.0 * std::pow (3.0, (double) nu.ton);
            const double q = 1.2 + 4.0 * (double) nu.styrka;
            for (auto& b : k.band) b.satt (krang::dsp::Biquad::Typ::bandpass, fs, mitt, q);
            for (auto& b : k.radio) b.satt (krang::dsp::Biquad::Typ::lagpass, fs, 3000.0, 0.707);
        }
    }

    struct Kanal { float hall = 0.0f, fas = 0.0f; std::array<krang::dsp::Biquad, 2> band {}, radio {}; krang::dsp::Biquad lp; };
    double fs = 48000.0, ringFas = 0.0, visselFas = 0.0;
    std::array<Kanal, maxKanaler> kan;
    std::array<krang::dsp::Nivakomp, maxKanaler> nivakomp {};
    Param nu;
    bool forst = false;
    float fad = 1.0f, fadMal = 1.0f, fadK = 0.0f;
    int fadRakn = 0;
    krang::dsp::Brus br;
};

} // namespace rv::dsp
