// Kören – dubblingen. En röst blir två (DUBBEL), fyra (KÖR) eller åtta (MASSA).
//
// Varje extra röst är en modulerad fördröjning: en egen grundtid (Spridning: 12–45 ms, fler röster
// sprids över längre tid), ett långsamt svaj i tiden som ger tonhöjdssvaj (Svaj: upp till ±12 ct) och
// en egen plats i stereobilden (Bredd). Rösterna är isärstämda med olika svajtakt, så att de inte
// följer varandra. Blandningen: torrt fullt upp till 50 %, kören fullt från 50 %.
//
// Utgår från tankarna i krang/dsp/Choruset.h, men med egna röster för tidsförskjutningen.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <krang/dsp/Grund.h>
#include <array>
#include <cmath>

namespace rv::dsp
{

class Koren
{
public:
    static constexpr int maxRoster = 7;   // extra röster utöver den torra
    enum Lage { dubbel = 0, kor = 1, massa = 2 };

    void forbered (double samplingsfrekvens)
    {
        fs = samplingsfrekvens;
        for (auto& l : linje) l.forbered ((int) (fs * 0.08));
        Slump s;
        for (int r = 0; r < maxRoster; ++r)
        {
            auto& rr = roster[(size_t) r];
            rr.takt = 0.11 + 0.37 * s.enhet();
            rr.fas = s.enhet();
            rr.bas = 0.25f + 0.75f * ((float) r + s.enhet() * 0.5f) / (float) maxRoster;
            rr.pan = (r % 2 == 0 ? -1.0f : 1.0f) * (0.3f + 0.7f * (float) (r / 2 + 1) / 4.0f);
            rr.niva = 0.0f;
        }
        nollstall();
    }

    void nollstall() noexcept
    {
        for (auto& l : linje) l.reset();
        for (auto& r : roster) r.niva = 0.0f;
    }

    void installning (int l, float spridning, float svaj, float bredd, float blandning) noexcept
    {
        lage = l; sprid = spridning; svajM = svaj; breddM = bredd; bland = blandning;
    }

    void process (float* const* ljud, int kanaler, int n) noexcept
    {
        const int aktiva = lage == dubbel ? 1 : (lage == kor ? 3 : 7);
        const float gTorr = std::min (1.0f, 2.0f * (1.0f - bland));
        const float gVat  = std::min (1.0f, 2.0f * bland);
        const float rostVikt = 1.0f / std::sqrt ((float) aktiva);
        const float tidMin = 0.012f * (float) fs, tidMax = (0.020f + 0.025f * (float) (aktiva > 1 ? 1 : 0)) * (float) fs;
        const float steg = 1.0f / (0.02f * (float) fs);   // röster tonas in och ut på 20 ms
        for (int i = 0; i < n; ++i)
        {
            float in[2] { ljud[0][i], kanaler > 1 ? ljud[1][i] : ljud[0][i] };
            for (int k = 0; k < kanaler; ++k) linje[(size_t) k].skriv (in[k]);
            float ut[2] { 0.0f, 0.0f };
            for (int r = 0; r < maxRoster; ++r)
            {
                auto& rr = roster[(size_t) r];
                const float mal = r < aktiva ? 1.0f : 0.0f;
                rr.niva += std::clamp (mal - rr.niva, -steg, steg);
                if (rr.niva <= 0.0f) continue;
                rr.fas += rr.takt / fs; if (rr.fas >= 1.0) rr.fas -= 1.0;
                // Svaj ±12 ct vid fullt: tidens derivata ger tonhöjden; djupet i sample räknas om från takten.
                const float djup = svajM * 0.0069f / (2.0f * 3.14159265f * (float) rr.takt) * (float) fs;
                const float tid = tidMin + (tidMax - tidMin) * rr.bas * (0.3f + 0.7f * sprid) + djup * (1.0f + (float) std::sin (2.0 * krang::dsp::pi * rr.fas));
                const float p = rr.pan * breddM;
                const float gl = std::sqrt (0.5f * (1.0f - p)), gr = std::sqrt (0.5f * (1.0f + p));
                if (kanaler > 1)
                {
                    const float vl = linje[0].las (tid), vr = linje[1].las (tid);
                    const float v = 0.5f * (vl + vr);
                    ut[0] += (v * gl * 1.41f * breddM + vl * (1.0f - breddM)) * rr.niva * rostVikt;
                    ut[1] += (v * gr * 1.41f * breddM + vr * (1.0f - breddM)) * rr.niva * rostVikt;
                }
                else ut[0] += linje[0].las (tid) * rr.niva * rostVikt;
            }
            for (int k = 0; k < kanaler; ++k) ljud[k][i] = in[k] * gTorr + ut[k] * gVat;
        }
    }

private:
    struct Slump { uint32_t s = 0x2545F491u; float enhet() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s >> 8) / 16777216.0f; } };
    struct Rost { double takt = 0.2, fas = 0.0; float bas = 0.5f, pan = 0.0f, niva = 0.0f; };
    double fs = 48000.0;
    std::array<krang::dsp::Fordrojning, 2> linje;
    std::array<Rost, maxRoster> roster {};
    int lage = dubbel;
    float sprid = 0.4f, svajM = 0.3f, breddM = 0.7f, bland = 0.5f;
};

} // namespace rv::dsp
