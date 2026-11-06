// Orgeln – vokoderns inbyggda bärvåg: åtta stämmor från MIDI, eller en hållen ton (TON) utan MIDI.
//
// Vågformerna är polyBLEP-såg och -puls (som Kolvbasens oscillatorer i Kroppsmaskinen), utan filter –
// vokodern gör filtreringen. Brus kan blandas in, för att tonlösa ljud ska få något att färga.
// Glid gäller varje stämma: en ny ton glider från den förra tonen i samma stämma.
//
// Ljudtråden allokerar inget: stämmorna är en fast array.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <krang/dsp/Grund.h>
#include <array>
#include <algorithm>
#include <cmath>

namespace rv::dsp
{

class Orgeln
{
public:
    static constexpr int antalStammor = 8;
    enum Vag { sag = 0, puls = 1, sagPuls = 2 };

    void forbered (double samplingsfrekvens)
    {
        fs = samplingsfrekvens;
        attK = krang::dsp::tidKoeff (0.003f, fs);
        relK = krang::dsp::tidKoeff (0.040f, fs);
        allaAv (true);
    }

    // vag 0..2, oktav −2..+1, glid i ms, brus 0..1.
    void installning (int v, int okt, float glidMs, float brusMangd) noexcept
    {
        vag = v;
        oktavFaktor = std::pow (2.0f, (float) okt);
        glidK = glidMs <= 0.5f ? 0.0f : krang::dsp::tidKoeff (glidMs * 0.001f, fs);
        brus = brusMangd;
    }

    void tonPa (int ton) noexcept
    {
        // Samma ton igen: återanvänd stämman. Annars en ledig, annars den äldsta (stöld utan klick: den
        // stulna stämman glider bara över till den nya tonen och behåller sin nivå).
        int val = -1;
        for (int i = 0; i < antalStammor; ++i) if (st[(size_t) i].ton == ton) val = i;
        if (val < 0) for (int i = 0; i < antalStammor; ++i) if (! st[(size_t) i].haller && st[(size_t) i].niva < 1.0e-4f) { val = i; break; }
        if (val < 0) { int aldst = 0; for (int i = 1; i < antalStammor; ++i) if (st[(size_t) i].alder > st[(size_t) aldst].alder) aldst = i; val = aldst; }
        auto& s = st[(size_t) val];
        const float f = frekvens (ton);
        if (s.niva < 1.0e-4f || glidK <= 0.0f) s.frek = f;
        s.malFrek = f; s.ton = ton; s.haller = true; s.alder = 0;
        for (auto& a : st) ++a.alder;
        senaste = ton;
    }

    void tonAv (int ton) noexcept
    {
        for (auto& s : st) if (s.ton == ton) s.haller = false;
    }

    // Hållen ton utan MIDI: stämma 0 spelar 'ton' och alla andra släpps.
    void halltTon (int ton) noexcept
    {
        for (int i = 1; i < antalStammor; ++i) st[(size_t) i].haller = false;
        auto& s = st[0];
        if (! s.haller || s.ton != ton)
        {
            const float f = frekvens (ton);
            if (s.niva < 1.0e-4f || glidK <= 0.0f) s.frek = f;
            s.malFrek = f; s.ton = ton; s.haller = true;
        }
        senaste = ton;
    }

    void allaAv (bool direkt = false) noexcept
    {
        for (auto& s : st) { s.haller = false; if (direkt) { s.niva = 0.0f; s.ton = -1; } }
    }

    // Ett sample, mono.
    float nasta() noexcept
    {
        float summa = 0.0f, storst = 0.0f;
        for (auto& s : st)
        {
            const float mal = s.haller ? 1.0f : 0.0f;
            s.niva = mal + (mal > s.niva ? attK : relK) * (s.niva - mal);
            if (s.niva < 1.0e-5f && ! s.haller) { s.niva = 0.0f; continue; }
            s.frek = s.malFrek + glidK * (s.frek - s.malFrek);
            const float dt = std::clamp (s.frek * oktavFaktor / (float) fs, 0.0f, 0.45f);
            float v = 0.0f;
            if (vag != puls)   v += sagtand (s.fas, dt);
            if (vag != sag)    v += fyrkant (s.fas, dt);
            if (vag == sagPuls) v *= 0.6f;
            summa += v * s.niva;
            storst = std::max (storst, s.niva);
            s.fas += dt; if (s.fas >= 1.0f) s.fas -= 1.0f;
        }
        // Bruset följer stämmorna: utan ton är Orgeln tyst.
        return summa * 0.35f + brus * 0.5f * storst * br.vitt();
    }

    int senasteTon() const noexcept { return senaste; }
    bool klingar() const noexcept { for (auto& s : st) if (s.haller || s.niva > 1.0e-4f) return true; return false; }

    static float frekvens (int ton) noexcept { return 440.0f * std::pow (2.0f, ((float) ton - 69.0f) / 12.0f); }

private:
    struct Stamma { int ton = -1; float fas = 0.0f, frek = 110.0f, malFrek = 110.0f, niva = 0.0f; bool haller = false; int alder = 0; };

    static float blep (float t, float dt) noexcept
    {
        if (t < dt)        { t /= dt; return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }
    static float sagtand (float t, float dt) noexcept { return 2.0f * t - 1.0f - blep (t, dt); }
    static float fyrkant (float t, float dt) noexcept
    {
        float v = t < 0.5f ? 1.0f : -1.0f;
        v += blep (t, dt);
        float t2 = t + 0.5f; if (t2 >= 1.0f) t2 -= 1.0f;
        v -= blep (t2, dt);
        return v;
    }

    double fs = 48000.0;
    std::array<Stamma, antalStammor> st {};
    int vag = sag, senaste = 48;
    float oktavFaktor = 1.0f, glidK = 0.0f, brus = 0.0f, attK = 0.0f, relK = 0.0f;
    krang::dsp::Brus br;
};

} // namespace rv::dsp
