// Porten – rösten in: högpass, grind, kompression, närhet och avsibilant.
//
// Ordningen: högpass → grind → kompressor → närhet. Grinden och kompressorn styrs av en gemensam
// detektor (den starkaste kanalen), så att stereobilden står still. Avsibilanten tar ner s-ljuden i
// den torra vägen; vokodern får rösten före avsibilanten, eftersom s-ljuden gör orden begripliga.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <krang/dsp/Grund.h>
#include <array>
#include <cmath>

namespace rv::dsp
{

class Porten
{
public:
    static constexpr int maxKanaler = 2;

    void forbered (double samplingsfrekvens)
    {
        fs = samplingsfrekvens;
        grindF.satt (fs, 0.001f, 0.060f);
        kompF.satt (fs, 0.005f, 0.120f);
        sibF.satt (fs, 0.001f, 0.050f);
        heltF.satt (fs, 0.001f, 0.050f);
        oppnaK = krang::dsp::tidKoeff (0.001f, fs);
        stangK = krang::dsp::tidKoeff (0.060f, fs);
        gK = krang::dsp::tidKoeff (0.003f, fs);
        for (auto& b : sibDet) b.satt (krang::dsp::Biquad::Typ::bandpass, fs, 6500.0, 1.0);
        for (auto& b : sibDel) b.satt (krang::dsp::Biquad::Typ::hogpass, fs, 4500.0, 0.707);
        hogpassNu = -1.0f; narhetNu = -1.0f;
        installning (-60.0f, 0.3f, 80.0f, 0.3f, false);
        nollstall();
    }

    void nollstall() noexcept
    {
        for (auto& b : hp) b.reset();
        for (auto& b : narhet) b.reset();
        for (auto& b : sibDel) b.reset();
        for (auto& b : sibDet) b.reset();
        grindF.reset(); kompF.reset(); sibF.reset(); heltF.reset();
        grindG = 0.0f; kompG = 1.0f; sibG = 1.0f;
    }

    void installning (float troskelDb, float kompression, float hogpassHz, float narhetMangd, bool avsibilant) noexcept
    {
        troskel = krang::dsp::dbTillGain (troskelDb);
        // En ratt: tröskel −12 … −30 dB, förhållande 1:1 … 6:1, och lite påfyllnad.
        kompTroskelDb = -12.0f - 18.0f * kompression;
        kompKvot = 1.0f + 5.0f * kompression;
        pafyllnad = krang::dsp::dbTillGain (-kompTroskelDb * (1.0f - 1.0f / kompKvot) * 0.45f);
        kompPa = kompression > 0.001f;
        if (std::abs (hogpassHz - hogpassNu) > 0.5f)
        {
            hogpassNu = hogpassHz;
            for (auto& b : hp) b.satt (krang::dsp::Biquad::Typ::hogpass, fs, hogpassHz, 0.707);
        }
        if (std::abs (narhetMangd - narhetNu) > 0.001f)
        {
            narhetNu = narhetMangd;
            for (auto& b : narhet) b.satt (krang::dsp::Biquad::Typ::topp, fs, 3200.0, 0.8, 8.0 * narhetMangd);
        }
        avsib = avsibilant;
    }

    // ljud: in och ut (torra vägen, med avsibilant). analys: ut, rösten före avsibilanten (per kanal).
    void process (float* const* ljud, float* const* analys, int kanaler, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            float x[maxKanaler] {};
            float topp = 0.0f;
            for (int k = 0; k < kanaler; ++k)
            {
                x[k] = hp[(size_t) k].process (ljud[k][i]);
                topp = std::max (topp, std::abs (x[k]));
            }
            // Grinden: öppnar på 1 ms, stänger på 60 ms, ner till −60 dB.
            const float e = grindF.process (topp);
            const float malG = e > troskel ? 1.0f : 0.001f;
            grindG = malG + (malG > grindG ? oppnaK : stangK) * (grindG - malG);
            // Kompressorn
            float g = grindG;
            if (kompPa)
            {
                const float ke = kompF.process (topp * grindG);
                const float db = krang::dsp::gainTillDb (ke);
                const float over = db - kompTroskelDb;
                const float mal = over > 0.0f ? krang::dsp::dbTillGain (-over * (1.0f - 1.0f / kompKvot)) : 1.0f;
                kompG = mal + gK * (kompG - mal);
                g *= kompG * pafyllnad;
            }
            // Avsibilanten: s-bandets andel av helheten.
            float sibMal = 1.0f;
            if (avsib)
            {
                float s = 0.0f, h = 0.0f;
                for (int k = 0; k < kanaler; ++k) { s = std::max (s, std::abs (sibDet[(size_t) k].process (x[k]))); h = std::max (h, std::abs (x[k])); }
                const float se = sibF.process (s), he = heltF.process (h);
                const float andel = se / (he + 1.0e-6f);
                sibMal = andel > 0.35f ? std::max (0.3f, 0.35f / andel) : 1.0f;
            }
            sibG = sibMal + gK * (sibG - sibMal);
            for (int k = 0; k < kanaler; ++k)
            {
                const float y = narhet[(size_t) k].process (x[k] * g);
                analys[k][i] = y;
                const float s = sibDel[(size_t) k].process (y);
                ljud[k][i] = y - s * (1.0f - sibG);
            }
        }
    }

    float grindOppning() const noexcept { return grindG; }

private:
    double fs = 48000.0;
    std::array<krang::dsp::Biquad, maxKanaler> hp {}, narhet {}, sibDel {}, sibDet {};
    krang::dsp::Foljare grindF, kompF, sibF, heltF;
    float troskel = 0.001f, kompTroskelDb = -18.0f, kompKvot = 2.0f, pafyllnad = 1.0f;
    float grindG = 0.0f, kompG = 1.0f, sibG = 1.0f, oppnaK = 0.0f, stangK = 0.0f, gK = 0.0f;
    float hogpassNu = -1.0f, narhetNu = -1.0f;
    bool kompPa = true, avsib = false;
};

} // namespace rv::dsp
