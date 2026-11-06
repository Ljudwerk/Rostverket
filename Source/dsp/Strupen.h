// Strupen – tonhöjd och formant var för sig, med PSOLA (tonhöjdssynkron överlappning och addition).
//
// Tonhöjden mäts med YIN på en nedsamplad kopia (24 kHz, fönster 25 ms, var 5:e ms), 70–600 Hz.
// Analysmärkena sätts en period i taget och flyttas till periodens topp. Syntesen lägger ut
// Hann-fönstrade korn, två perioder långa, med avståndet period/tonhöjdskvot; kornen läses med
// formantkvoten, så att formanterna flyttas för sig. Fönstersumman delas bort, så att nivån står still.
// Tonlösa ljud (s, f, brus) får korn med fast längd och ingen tonhöjdsändring.
//
// Lägen: SKIFT (tonhöjd och formant), OKTAV (den förskjutna rösten blandas med den torra – parollrösten)
// och MONOTON (röstens tonhöjd dras mot en enda ton; Följsamhet = hur hårt).
//
// Latensen är fast: 25 ms. Den torra vägen fördröjs lika mycket, så att blandningen inte kamfiltrerar,
// och en avstängd Strupe är en ren fördröjning (så att latensen aldrig ändras).
// Allt är förallokerat – ljudtråden allokerar inget.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <krang/dsp/Grund.h>
#include <array>
#include <cmath>
#include <vector>

namespace rv::dsp
{

class Strupen
{
public:
    static constexpr int maxKanaler = 2;
    enum Lage { skift = 0, oktav = 1, monoton = 2 };

    static int latensFor (double fs) { return (int) std::lround (0.025 * fs); }

    void forbered (double samplingsfrekvens)
    {
        fs = samplingsfrekvens;
        latens = latensFor (fs);
        minPeriod = (int) (fs / 600.0); maxPeriod = (int) (fs / 70.0);
        for (auto& k : kan)
        {
            k.in.assign ((size_t) storlek, 0.0f);
            k.ut.assign ((size_t) storlek, 0.0f);
            k.torr.forbered (latens); k.torr.sattLangd (latens);
        }
        vikt.assign ((size_t) storlek, 0.0f);
        dec.assign ((size_t) decStorlek, 0.0f);
        yinD.assign ((size_t) (maxPeriod / 2 + 2), 0.0f);
        decLp.satt (fs, 5000.0);
        nollstall();
    }

    void nollstall() noexcept
    {
        for (auto& k : kan) { std::fill (k.in.begin(), k.in.end(), 0.0f); std::fill (k.ut.begin(), k.ut.end(), 0.0f); k.torr.reset(); }
        std::fill (vikt.begin(), vikt.end(), 0.0f);
        std::fill (dec.begin(), dec.end(), 0.0f);
        decLp.reset();
        w = 0; decW = 0; sedanYin = 0;
        period = (float) (0.005 * fs); tonande = false; f0 = 0.0f;
        nastaMarke = (double) latens; marken.fill ({ 0.0, (float) (0.005 * fs) }); markeW = 0; antalMarken = 0;
        nastaKorn = 0.0; kvotNu = 1.0f;
    }

    // tonhojdSt/formantSt ±12; foljsamhet 0..1; tonMidi = MONOTON-tonen.
    void installning (int l, float tonhojdSt, float formantSt, float foljsamhet, int tonMidi) noexcept
    {
        lage = l;
        kvotSkift = std::pow (2.0f, tonhojdSt / 12.0f);
        formantKvot = std::pow (2.0f, formantSt / 12.0f);
        folj = foljsamhet;
        malF0 = 440.0f * std::pow (2.0f, ((float) tonMidi - 69.0f) / 12.0f);
    }

    int latensSampel() const noexcept { return latens; }
    float uppmattF0() const noexcept { return tonande ? f0 : 0.0f; }

    // Läser ljud[k][i], skriver tillbaka det förskjutna ljudet; torr[k][i] får den fördröjda torra rösten.
    void process (float* const* ljud, float* const* torr, int kanaler, int n) noexcept
    {
        const int mask = storlek - 1;
        for (int i = 0; i < n; ++i)
        {
            // Skriv in
            float mono = 0.0f;
            for (int k = 0; k < kanaler; ++k)
            {
                kan[(size_t) k].in[(size_t) (w & mask)] = ljud[k][i];
                mono += ljud[k][i];
            }
            mono /= (float) kanaler;
            // Nedsamplad kopia (24 kHz) för YIN
            const float lp = decLp.process (mono);
            if ((w & 1) == 0) { dec[(size_t) (decW & (decStorlek - 1))] = lp; ++decW; }
            if (++sedanYin >= hopp) { sedanYin = 0; matTonhojd(); }

            // Nya analysmärken när ett helt korn finns (märket + en period <= w). Märkena ligger aldrig
            // längre bak än ringbufferten räcker.
            if (nastaMarke < (double) w - storlek / 4) nastaMarke = (double) w - storlek / 4;
            while (nastaMarke + period < (double) w)
            {
                const double m = justera (nastaMarke, period);
                marken[(size_t) (markeW % antalMarkenMax)] = { m, period };
                ++markeW; antalMarken = std::min (antalMarken + 1, antalMarkenMax);
                nastaMarke = m + period;
            }

            // Läggs korn ut? Utgången ligger 'latens' bakom ingången; ett korn läggs när dess början når utgången.
            const double tUt = (double) (w - latens);
            while (antalMarken > 0 && nastaKorn - kornHalva() <= tUt + 1.0)
            {
                if (nastaKorn < tUt) nastaKorn = tUt;   // efter en paus: börja om vid utgången
                laggKorn (kanaler, nastaKorn);
            }

            // Läs ut
            const int r = (int) (tUt) & mask;
            const float v = vikt[(size_t) r];
            const float norm = v > 0.25f ? 1.0f / v : 16.0f * v;   // fönstersumman delas bort; nära noll tonas ned
            for (int k = 0; k < kanaler; ++k)
            {
                auto& kk = kan[(size_t) k];
                torr[k][i] = kk.torr.process (ljud[k][i]);
                ljud[k][i] = kk.ut[(size_t) r] * norm;
                kk.ut[(size_t) r] = 0.0f;
            }
            vikt[(size_t) r] = 0.0f;
            ++w;
        }
    }

private:
    struct Marke { double tid; float period; };
    static constexpr int storlek = 16384;                 // ringbuffertarna (2^n)
    static constexpr int decStorlek = 2048;
    static constexpr int hopp = 240;                      // YIN var 5:e ms vid 48 kHz
    static constexpr int antalMarkenMax = 64;

    // Halva nästa korns längd (för att lägga ut det när dess början når utgången).
    float kornHalva() const noexcept { return tonande ? period / std::clamp (formantKvot, 0.5f, 2.0f) : period; }

    // YIN på den nedsamplade kopian. Ger period (i sample vid fs) och tonande/tonlös.
    void matTonhojd() noexcept
    {
        const int fonster = (int) (0.025 * fs / 2.0);
        const int tauMin = std::max (2, minPeriod / 2), tauMax = std::min ((int) yinD.size() - 1, maxPeriod / 2);
        const int bas = decW - fonster - tauMax;
        if (bas < 0) return;
        const int dm = decStorlek - 1;
        float summa = 0.0f; int basta = -1; float bastaV = 1.0f;
        yinD[0] = 1.0f;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            float d = 0.0f;
            for (int j = 0; j < fonster; j += 2)
            {
                const float a = dec[(size_t) ((bas + tauMax + j) & dm)] - dec[(size_t) ((bas + tauMax + j - tau) & dm)];
                d += a * a;
            }
            summa += d;
            yinD[(size_t) tau] = summa > 0.0f ? d * (float) tau / summa : 1.0f;
            if (tau >= tauMin && basta < 0 && yinD[(size_t) tau] < 0.15f)
            {
                // Första dalen under tröskeln: gå till dess botten.
                int t = tau;
                while (t + 1 <= tauMax)
                {
                    float d2 = 0.0f;
                    for (int j = 0; j < fonster; j += 2)
                    {
                        const float a = dec[(size_t) ((bas + tauMax + j) & dm)] - dec[(size_t) ((bas + tauMax + j - t - 1) & dm)];
                        d2 += a * a;
                    }
                    const float s2 = summa + d2;
                    const float v2 = s2 > 0.0f ? d2 * (float) (t + 1) / s2 : 1.0f;
                    if (v2 >= yinD[(size_t) t]) break;
                    summa = s2; ++t; yinD[(size_t) t] = v2;
                }
                basta = t; bastaV = yinD[(size_t) t];
                break;
            }
        }
        // Energi: tystnad räknas som tonlös.
        float e = 0.0f;
        for (int j = 0; j < fonster; ++j) { const float x = dec[(size_t) ((bas + tauMax + j) & dm)]; e += x * x; }
        const bool rost = e / (float) fonster > 1.0e-7f;
        if (basta > 0 && rost && bastaV < 0.15f)
        {
            // Parabolisk interpolation runt dalen.
            float tauF = (float) basta;
            if (basta > 1 && basta < tauMax)
            {
                const float a = yinD[(size_t) basta - 1], b = yinD[(size_t) basta], c = yinD[(size_t) basta + 1];
                const float den = a - 2.0f * b + c;
                if (std::abs (den) > 1.0e-9f) tauF += 0.5f * (a - c) / den;
            }
            const float p = std::clamp (tauF * 2.0f, (float) minPeriod, (float) maxPeriod);
            period = tonande ? 0.6f * period + 0.4f * p : p;
            tonande = true;
            f0 = (float) fs / period;
        }
        else
        {
            tonande = false;
            period = (float) (0.006 * fs);
        }
    }

    // Flyttar märket till den största toppen inom ±period/4.
    double justera (double m, float p) const noexcept
    {
        if (! tonande) return m;
        const int mask = storlek - 1;
        const int c = (int) m, r = (int) (p * 0.25f);
        int basta = c; float bv = -1.0f;
        for (int j = c - r; j <= c + r; ++j)
        {
            float v = 0.0f;
            for (int k = 0; k < 2; ++k) v += kan[(size_t) k].in[(size_t) (j & mask)];
            if (v > bv) { bv = v; basta = j; }
        }
        return (double) basta;
    }

    // Ett korn vid syntestiden t: närmaste analysmärke, Hann-fönster två perioder, läst med formantkvoten.
    void laggKorn (int kanaler, double t) noexcept   // t kan flyttas till märket
    {
        const int mask = storlek - 1;
        // Närmaste märke till t (bland de senaste)
        int basta = -1; double bd = 1.0e18;
        for (int j = 0; j < antalMarken; ++j)
        {
            const auto& m = marken[(size_t) ((markeW - 1 - j) % antalMarkenMax)];
            const double d = std::abs (m.tid - t);
            if (d < bd) { bd = d; basta = (markeW - 1 - j) % antalMarkenMax; }
        }
        const auto mk = marken[(size_t) basta];
        const float pa = mk.period;
        // Kvoten för det här kornet
        float kvot = 1.0f;
        if (tonande)
        {
            if (lage == monoton)
            {
                const float mal = std::clamp (malF0 / std::max (40.0f, f0), 0.25f, 4.0f);
                kvot = std::pow (mal, folj);
            }
            else kvot = kvotSkift;
        }
        kvotNu = kvot;
        // Utan tonhöjdsändring läggs kornet exakt på sitt märke, så att rösten ligger i fas med den torra.
        const bool last = std::abs (kvot - 1.0f) < 0.003f;
        if (last && mk.tid >= t - 2.0 && mk.tid <= t + pa) t = mk.tid;
        const float fk = tonande ? std::clamp (formantKvot, 0.5f, 2.0f) : 1.0f;
        const int L = std::max (8, (int) (2.0f * pa / fk));
        const double start = t - (double) L * 0.5;
        const float steg = fk;
        for (int j = 0; j < L; ++j)
        {
            const float fon = 0.5f - 0.5f * std::cos (2.0f * 3.14159265f * ((float) j + 0.5f) / (float) L);
            const double las = mk.tid + ((double) j - (double) L * 0.5) * steg;
            const int a = (int) std::floor (las);
            const float fr = (float) (las - a);
            const int o = (int) (start + j) & mask;
            for (int k = 0; k < kanaler; ++k)
            {
                const auto& in = kan[(size_t) k].in;
                const float x = in[(size_t) (a & mask)] + fr * (in[(size_t) ((a + 1) & mask)] - in[(size_t) (a & mask)]);
                kan[(size_t) k].ut[(size_t) o] += x * fon;
            }
            vikt[(size_t) o] += fon;
        }
        // Nästa korn: perioden delad med tonhöjdskvoten (tonlöst: fast avstånd, halva kornet).
        nastaKorn = t + std::max (16.0, (double) (tonande ? pa / kvot : pa));
        if (last)
        {
            // Nästa märke, om det redan finns: då följer kornen märkena exakt.
            for (int j = 0; j < antalMarken; ++j)
            {
                const auto& m = marken[(size_t) ((markeW - 1 - j) % antalMarkenMax)];
                if (m.tid > t + 1.0 && m.tid < nastaKorn + pa * 0.5) nastaKorn = std::min (nastaKorn, m.tid);
            }
        }
    }

    struct Kanal { std::vector<float> in, ut; krang::dsp::Fordrojning torr; };
    double fs = 48000.0;
    int latens = 1200, minPeriod = 80, maxPeriod = 685;
    std::array<Kanal, maxKanaler> kan;
    std::vector<float> vikt, dec, yinD;
    krang::dsp::EnPol decLp;
    int w = 0, decW = 0, sedanYin = 0;
    float period = 240.0f, f0 = 0.0f; bool tonande = false;
    std::array<Marke, antalMarkenMax> marken {};
    int markeW = 0, antalMarken = 0;
    double nastaMarke = 0.0, nastaKorn = 0.0;
    int lage = skift;
    float kvotSkift = 1.0f, formantKvot = 1.0f, folj = 0.8f, malF0 = 130.8f, kvotNu = 1.0f;
};

} // namespace rv::dsp
