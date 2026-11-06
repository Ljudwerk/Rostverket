// RVRender – kör ljud genom Röstverket utan DAW, och testar kedjan.
//
//   RVRender --test                         testfallen (se nedan; skriver OK/FEL och avslutar med felkod)
//   RVRender in.wav ut.wav [id=värde ...]   renderar en fil; värdena är parametrarnas riktiga värden
//                                           (dB, ms, 0..1, läge som index, 0/1)
//   RVRender --exempel röst.wav bas.wav utmapp
//                                           grind 1-exemplen: vokodern med Orgeln (ackord från MIDI),
//                                           med basen i sidokedjan, med brus, frusen, och en robot
//   RVRender --parametrar fil.txt           parametrarnas index, id, namn och intervall (för Reaper-skripten)
//   RVRender --program röst.wav utmapp      alla fabriksprogram på rösten (Orgeln på TON, ingen MIDI),
//                                           med nivån mot den torra rösten (för att nivåanpassa Utnivå)
//   RVRender --state fil.xml [id=värde ...] pluginets läge som XML (det som ligger i Reaper-projektet)
//   RVRender --projekt låt.json ut.wav      spelar röstspåret i en testlåt (ljudobjekt, MIDI och automation
//                                           ur json-filen, skriven av tools/reaper/gor_reaper_projekt.py)
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <krang/ui/ValueFormat.h>
#include <algorithm>
#include <functional>
#include <iostream>

namespace
{
using namespace rv;
using krang::dsp::Biquad;

int fel = 0;

juce::String u (const char* s) { return juce::String::fromUTF8 (s); }
juce::String sv (double v, int dec) { return krang::ui::swedishNumber (v, dec); }   // decimalkomma, riktigt minus

void kontroll (bool ok, const juce::String& vad)
{
    std::cout << (ok ? "  OK    " : "  FEL   ") << vad.toRawUTF8() << "\n";
    if (! ok) ++fel;
}
void kontroll (bool ok, const char* vad) { kontroll (ok, u (vad)); }

void satt (RostverketProcessor& p, const juce::String& pid, float varde)
{
    auto* par = p.apvts.getParameter (pid);
    if (par == nullptr) { std::cout << "  okänd parameter: " << pid << "\n"; return; }
    par->setValueNotifyingHost (par->convertTo0to1 (varde));
}

using Installning = std::vector<std::pair<juce::String, float>>;

// En spelhuvud-attrapp: transporten går i givet tempo från takt 1 (Ekot följer tempot).
struct Spelhuvud : juce::AudioPlayHead
{
    double bpm = 120.0, ppq = 0.0;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setBpm (bpm); p.setPpqPosition (ppq); p.setIsPlaying (true);
        return p;
    }
};
double korBpm = 120.0;
// Anropas före varje block: (processorn, blockets start, längd, MIDI att fylla).
using PerBlock = std::function<void (RostverketProcessor&, int, int, juce::MidiBuffer&)>;

// --------------------------------------------------------------------------------------------
// Signaler

// En provsignal: brus i skurar, en sågton med vibrato och ett par klick – med full upplösning i
// mantissan, så att varje förändring av ett sample syns i jämförelsen.
juce::AudioBuffer<float> provsignal (int kanaler, double fs, float sek, uint32_t fro = 1)
{
    juce::AudioBuffer<float> b (kanaler, (int) (fs * sek));
    juce::Random r ((juce::int64) fro);
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double t = i / fs;
        const double sag = std::fmod (t * (110.0 + 3.0 * std::sin (t * 30.0)), 1.0) * 2.0 - 1.0;
        const double skur = std::fmod (t, 0.5) < 0.12 ? r.nextFloat() * 2.0f - 1.0f : 0.0f;
        double v = 0.35 * sag + 0.25 * skur;
        if (i % 9973 == 0) v = 0.95;
        for (int k = 0; k < kanaler; ++k)
            b.setSample (k, i, (float) (v * (k == 0 ? 1.0 : 0.93) + 1.0e-7 * r.nextFloat()));
    }
    return b;
}

// En syntetisk röst: pulståg (120 Hz, lite svaj) genom tre formanter, vokaler som byts var 0,3 s,
// s-ljud (högpassat brus) och korta pauser. vokal = −1 ger alla vokaler; annars bara den vokalen.
juce::AudioBuffer<float> syntRost (int kanaler, double fs, float sek, bool medS = true, int vokal = -1)
{
    static const float form[4][3] { { 730, 1090, 2440 }, { 270, 2290, 3010 }, { 300, 870, 2240 }, { 530, 1840, 2480 } };
    juce::AudioBuffer<float> b (kanaler, (int) (fs * sek));
    std::array<Biquad, 3> f;
    Biquad sHp; sHp.satt (Biquad::Typ::hogpass, fs, 5000.0, 0.707);
    krang::dsp::Brus br;
    double fas = 0.0;
    int forraSeg = -1;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double t = i / fs;
        const int seg = (int) (t / 0.3);
        const double iSeg = std::fmod (t, 0.3);
        const bool paus = iSeg > 0.25;
        const bool s = medS && seg % 4 == 3;
        if (seg != forraSeg)
        {
            const int v = vokal >= 0 ? vokal : seg % 4;
            for (int j = 0; j < 3; ++j) f[(size_t) j].satt (Biquad::Typ::bandpass, fs, form[v][j], 6.0);
            forraSeg = seg;
        }
        float x = 0.0f;
        const double f0 = 120.0 * (1.0 + 0.02 * std::sin (t * 5.0));
        fas += f0 / fs;
        const float puls = fas >= 1.0 ? 1.0f : 0.0f;
        if (fas >= 1.0) fas -= 1.0;
        if (! paus)
        {
            if (s) x = 0.5f * sHp.process (br.vitt());
            else   x = 6.0f * (f[0].process (puls) + 0.7f * f[1].process (puls) + 0.4f * f[2].process (puls));
        }
        const float env = paus ? 0.0f : (float) std::min (1.0, std::min (iSeg / 0.01, (0.25 - iSeg) / 0.01));
        for (int k = 0; k < kanaler; ++k) b.setSample (k, i, x * env);
    }
    return b;
}

juce::AudioBuffer<float> sag (int kanaler, double fs, float sek, float frek, float niva = 0.4f)
{
    juce::AudioBuffer<float> b (kanaler, (int) (fs * sek));
    double fas = 0.0;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        fas += frek / fs; if (fas >= 1.0) fas -= 1.0;
        for (int k = 0; k < kanaler; ++k) b.setSample (k, i, niva * (float) (2.0 * fas - 1.0));
    }
    return b;
}

juce::AudioBuffer<float> sinus (int kanaler, double fs, float sek, float frek, float niva)
{
    juce::AudioBuffer<float> b (kanaler, (int) (fs * sek));
    for (int i = 0; i < b.getNumSamples(); ++i)
        for (int k = 0; k < kanaler; ++k) b.setSample (k, i, niva * (float) std::sin (2.0 * juce::MathConstants<double>::pi * frek * i / fs));
    return b;
}

// --------------------------------------------------------------------------------------------
// Körningen

// Kör en buffert genom en ny processor. sido = sidokedjans ljud (eller nullptr).
juce::AudioBuffer<float> kor (const juce::AudioBuffer<float>& in, double fs, const Installning& inst, int block,
                              const juce::AudioBuffer<float>* sido = nullptr, PerBlock perBlock = {}, int* latens = nullptr)
{
    RostverketProcessor p;
    const int kanaler = in.getNumChannels();
    auto layout = p.getBusesLayout();
    layout.inputBuses.getReference (0) = kanaler == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
    layout.outputBuses.getReference (0) = layout.inputBuses[0];
    p.setBusesLayout (layout);
    p.setRateAndBufferSizeDetails (fs, block);
    for (auto& [k, v] : inst) satt (p, k, v);
    Spelhuvud ph; ph.bpm = korBpm;
    p.setPlayHead (&ph);
    p.prepareToPlay (fs, block);
    if (latens) *latens = p.getLatencySamples();

    const int totalIn = p.getTotalNumInputChannels();
    const int n = in.getNumSamples();
    juce::AudioBuffer<float> ut (kanaler, n);
    juce::AudioBuffer<float> arbete (juce::jmax (totalIn, p.getTotalNumOutputChannels()), block);
    for (int s = 0; s < n; s += block)
    {
        const int len = std::min (block, n - s);
        arbete.clear();
        for (int k = 0; k < kanaler; ++k) arbete.copyFrom (k, 0, in, k, s, len);
        if (sido != nullptr)
            for (int k = kanaler; k < totalIn; ++k)
                if (s < sido->getNumSamples())
                    arbete.copyFrom (k, 0, *sido, (k - kanaler) % sido->getNumChannels(), s, std::min (len, sido->getNumSamples() - s));
        juce::AudioBuffer<float> bit (arbete.getArrayOfWritePointers(), arbete.getNumChannels(), 0, len);
        juce::MidiBuffer midi;
        if (perBlock) perBlock (p, s, len, midi);
        p.processBlock (bit, midi);
        ph.ppq += ph.bpm / 60.0 / fs * len;
        for (int k = 0; k < kanaler; ++k) ut.copyFrom (k, s, bit, k, 0, len);
    }
    return ut;
}

// Signalen fördröjd med 'd' sample (latensen), samma längd.
juce::AudioBuffer<float> fordrojd (const juce::AudioBuffer<float>& in, int d)
{
    juce::AudioBuffer<float> ut (in.getNumChannels(), in.getNumSamples());
    ut.clear();
    for (int k = 0; k < in.getNumChannels(); ++k) ut.copyFrom (k, d, in, k, 0, in.getNumSamples() - d);
    return ut;
}

bool bitexakt (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples()) return false;
    for (int k = 0; k < a.getNumChannels(); ++k)
        if (std::memcmp (a.getReadPointer (k), b.getReadPointer (k), sizeof (float) * (size_t) a.getNumSamples()) != 0) return false;
    return true;
}

bool andligt (const juce::AudioBuffer<float>& b)
{
    for (int k = 0; k < b.getNumChannels(); ++k)
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (! std::isfinite (b.getSample (k, i))) return false;
    return true;
}

float rmsDb (const juce::AudioBuffer<float>& b, double fs, float fran = 0.0f, float till = 1.0e9f)
{
    const int a = (int) juce::jlimit (0.0, (double) b.getNumSamples(), fran * fs), e = (int) juce::jlimit (0.0, (double) b.getNumSamples(), till * fs);
    double s = 0.0; int n = 0;
    for (int k = 0; k < b.getNumChannels(); ++k) for (int i = a; i < e; ++i) { s += (double) b.getSample (k, i) * b.getSample (k, i); ++n; }
    return n > 0 && s > 0.0 ? (float) (10.0 * std::log10 (s / n)) : -200.0f;
}

float toppDb (const juce::AudioBuffer<float>& b)
{
    float m = 0.0f;
    for (int k = 0; k < b.getNumChannels(); ++k) m = std::max (m, b.getMagnitude (k, 0, b.getNumSamples()));
    return krang::dsp::gainTillDb (m);
}

float dc (const juce::AudioBuffer<float>& b)
{
    float m = 0.0f;
    for (int k = 0; k < b.getNumChannels(); ++k)
    {
        double s = 0.0;
        for (int i = 0; i < b.getNumSamples(); ++i) s += b.getSample (k, i);
        m = std::max (m, (float) std::abs (s / b.getNumSamples()));
    }
    return m;
}

// Filtrerad kopia (för energi i ett band).
juce::AudioBuffer<float> filtrera (const juce::AudioBuffer<float>& b, double fs, Biquad::Typ typ, double f, double q = 0.707, int ordning = 2)
{
    juce::AudioBuffer<float> ut (b);
    for (int k = 0; k < ut.getNumChannels(); ++k)
        for (int o = 0; o < ordning; ++o)
        {
            Biquad bq; bq.satt (typ, fs, f, q);
            for (int i = 0; i < ut.getNumSamples(); ++i) ut.setSample (k, i, bq.process (ut.getSample (k, i)));
        }
    return ut;
}

// Största andra differensen – ett klick syns som en topp här.
float klick (const juce::AudioBuffer<float>& b)
{
    float m = 0.0f;
    for (int k = 0; k < b.getNumChannels(); ++k)
        for (int i = 2; i < b.getNumSamples(); ++i)
            m = std::max (m, std::abs (b.getSample (k, i) - 2.0f * b.getSample (k, i - 1) + b.getSample (k, i - 2)));
    return m;
}

// Grundtonen (median över 40 ms-fönster med tydlig periodicitet), 60–600 Hz. 0 om ingen hittas.
float f0Av (const juce::AudioBuffer<float>& b, double fs, float fran, float till)
{
    const int fon = (int) (0.04 * fs), lagMin = (int) (fs / 600.0), lagMax = (int) (fs / 55.0);
    std::vector<float> f;
    for (int s = (int) (fran * fs); s + fon + lagMax < (int) (till * fs) && s + fon + lagMax < b.getNumSamples(); s += fon / 2)
    {
        const float* x = b.getReadPointer (0) + s;
        double e0 = 0.0; for (int i = 0; i < fon; ++i) e0 += (double) x[i] * x[i];
        if (e0 < 1.0e-6 * fon) continue;
        int basta = -1; double bv = 0.0;
        std::vector<double> r ((size_t) lagMax + 2, 0.0);
        for (int lag = lagMin; lag <= lagMax + 1; ++lag)
        {
            double c = 0.0, e1 = 0.0;
            for (int i = 0; i < fon; ++i) { c += (double) x[i] * x[i + lag]; e1 += (double) x[i + lag] * x[i + lag]; }
            r[(size_t) lag] = c / std::sqrt (e0 * e1 + 1.0e-12);
        }
        // Första toppen över 0,8 av den största (undviker oktavfel uppåt).
        double maxR = 0.0; for (int lag = lagMin; lag <= lagMax; ++lag) maxR = std::max (maxR, r[(size_t) lag]);
        if (maxR < 0.6) continue;
        for (int lag = lagMin + 1; lag <= lagMax; ++lag)
            if (r[(size_t) lag] > 0.85 * maxR && r[(size_t) lag] >= r[(size_t) lag - 1] && r[(size_t) lag] >= r[(size_t) lag + 1]) { basta = lag; bv = r[(size_t) lag]; break; }
        if (basta > 0)
        {
            const double a = r[(size_t) basta - 1], c = r[(size_t) basta + 1];
            const double den = a - 2.0 * bv + c;
            const double t = basta + (std::abs (den) > 1.0e-12 ? 0.5 * (a - c) / den : 0.0);
            f.push_back ((float) (fs / t));
        }
    }
    if (f.empty()) return 0.0f;
    std::sort (f.begin(), f.end());
    return f[f.size() / 2];
}

float cent (float f, float ref) { return f > 0.0f ? 1200.0f * std::log2 (f / ref) : 9999.0f; }

float korrelation (const juce::AudioBuffer<float>& a, int ka, const juce::AudioBuffer<float>& b, int kb, int fran, int till)
{
    double c = 0.0, ea = 0.0, eb = 0.0;
    for (int i = fran; i < till; ++i) { const double x = a.getSample (ka, i), y = b.getSample (kb, i); c += x * y; ea += x * x; eb += y * y; }
    return (float) (c / std::sqrt (ea * eb + 1.0e-12));
}

// Parametrarnas ordning, som den var när fas 0 byggdes. Ändras aldrig – nya parametrar läggs sist.
const char* const ordningen[] {
    "rv_port_pa", "rv_port_troskel", "rv_port_kompression", "rv_port_hogpass", "rv_port_narhet", "rv_port_avsibilant",
    "rv_vok_pa", "rv_vok_band", "rv_vok_barvag", "rv_vok_formant", "rv_vok_snabbhet", "rv_vok_skarpa", "rv_vok_begriplighet",
    "rv_vok_frys", "rv_vok_blandning",
    "rv_org_vag", "rv_org_oktav", "rv_org_glid", "rv_org_brus", "rv_org_ton",
    "rv_strup_pa", "rv_strup_lage", "rv_strup_tonhojd", "rv_strup_formant", "rv_strup_foljsamhet", "rv_strup_blandning", "rv_strup_ton",
    "rv_sand_pa", "rv_sand_lage", "rv_sand_styrka", "rv_sand_frekvens", "rv_sand_ton", "rv_sand_blandning", "rv_sand_midi",
    "rv_kor_pa", "rv_kor_lage", "rv_kor_spridning", "rv_kor_svaj", "rv_kor_bredd", "rv_kor_blandning",
    "rv_eko_pa", "rv_eko_tid", "rv_eko_aterkoppling", "rv_eko_morker", "rv_eko_rum", "rv_eko_blandning", "rv_eko_pingpong",
    "rv_omvandlingen", "rv_ut_in", "rv_ut_torrvat", "rv_ut_niva", "rv_ut_limiter", "rv_forbi", "rv_midi" };

// Vokodern på med Orgeln (TON, utan MIDI).
const Installning vok { { id::vokPa, 1.0f } };
Installning med (Installning a, const Installning& b) { a.insert (a.end(), b.begin(), b.end()); return a; }

// En hållen ton från början (MIDI på).
PerBlock hallTon (int ton)
{
    return [ton] (RostverketProcessor&, int s, int, juce::MidiBuffer& m) { if (s == 0) m.addEvent (juce::MidiMessage::noteOn (1, ton, (juce::uint8) 100), 0); };
}

// Byter en parameter vid tiden 'vid' (sekunder).
PerBlock vidTid (double fs, float vid, const char* pid, float varde, PerBlock forst = {})
{
    return [=] (RostverketProcessor& p, int s, int len, juce::MidiBuffer& m)
    {
        if (forst) forst (p, s, len, m);
        const int t = (int) (vid * fs);
        if (s <= t && t < s + len) satt (p, pid, varde);
    };
}

Installning programInst (int i);

int test()
{
    std::cout << "RVRender --test (fas 2)\n";
    const double fs = 48000.0;
    const auto stereo = provsignal (2, fs, 3.0f);
    const auto mono = provsignal (1, fs, 2.0f, 7);
    const auto sido = provsignal (2, fs, 3.0f, 99);

    const int L = dsp::Strupen::latensFor (fs);
    const auto stereoD = fordrojd (stereo, L), monoD = fordrojd (mono, L);
    std::cout << "Grunden\n";
    kontroll (bitexakt (kor (stereo, fs, {}, 512), stereoD), "Init: stereo passerar bitexakt, fördröjt med latensen (block 512)");
    kontroll (bitexakt (kor (stereo, fs, {}, 37), stereoD), "Init: stereo passerar bitexakt (block 37)");
    kontroll (bitexakt (kor (stereo, fs, med (vok, { { id::portPa, 1.0f }, { id::forbi, 1.0f } }), 256), stereoD),
              "Förbikoppling (PÅ LUFTEN släckt) är bitexakt, även med modulerna på");
    kontroll (bitexakt (kor (mono, fs, {}, 512), monoD), "Mono in passerar bitexakt i Init");
    kontroll (bitexakt (kor (stereo, fs, {}, 512, &sido), stereoD), "Sidokedjan påverkar inte Init");
    kontroll (bitexakt (kor (stereo, fs, { { id::midiIn, 1.0f } }, 512, nullptr, hallTon (60)), stereoD), "MIDI påverkar inte Init");

    // Alla parametrar i topp och i botten: inget NaN, inget över taket.
    {
        Installning topp, botten;
        RostverketProcessor p;
        for (auto* par : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (par))
            {
                const juce::String pid = r->getParameterID();
                if (pid == id::forbi) continue;
                const auto ri = r->getNormalisableRange();
                topp.push_back ({ pid, ri.end });
                botten.push_back ({ pid, pid.endsWith ("_pa") ? 1.0f : ri.start });
            }
        const auto rost = syntRost (2, fs, 2.0f);
        const auto a = kor (rost, fs, topp, 512, &sido), b = kor (rost, fs, botten, 512, &sido);
        kontroll (andligt (a) && andligt (b) && toppDb (a) <= -0.29f, "Alla reglage i topp och botten: inget NaN, topp under −0,3 dBFS");
    }
    {
        int a = -1, b = -1;
        kor (stereo, fs, {}, 512, nullptr, {}, &a);
        kor (stereo, fs, { { id::strupPa, 1.0f }, { id::vokPa, 1.0f }, { id::ekoPa, 1.0f } }, 512, nullptr, {}, &b);
        kontroll (a == L && b == L && L == 1200, u ("Latensen är fast: 25 ms (") + juce::String (L) + " sample vid 48 kHz), med och utan moduler");
    }
    {
        RostverketProcessor a, b;
        satt (a, id::vokBand, 1.0f); satt (a, id::portTroskel, -42.0f); satt (a, id::orgTon, 55.0f); satt (a, id::ekoPingpong, 1.0f);
        juce::MemoryBlock mb; a.getStateInformation (mb);
        b.setStateInformation (mb.getData(), (int) mb.getSize());
        bool lika = true;
        for (int i = 0; i < a.getParameters().size(); ++i)
            lika = lika && std::abs (a.getParameters()[i]->getValue() - b.getParameters()[i]->getValue()) < 1.0e-6f;
        kontroll (lika && b.apvts.state.getProperty ("rvFormat").toString() == "1", "Tillståndet sparas och läses in (formatversion 1)");
    }
    {
        RostverketProcessor p;
        const auto& ps = p.getParameters();
        bool ok = ps.size() == antalParametrar && ps.size() == (int) std::size (ordningen);
        for (int i = 0; ok && i < ps.size(); ++i)
            ok = dynamic_cast<juce::AudioProcessorParameterWithID*> (ps[i])->paramID == ordningen[i];
        kontroll (ok, "Parametrarnas antal (54) och ordning ligger fast");
    }

    std::cout << "Orgeln\n";
    {
        auto frekvensAv = [fs] (int ton, int oktav)
        {
            dsp::Orgeln o; o.forbered (fs); o.installning (0, oktav, 0.0f, 0.0f); o.tonPa (ton);
            // Sågtanden går uppåt genom noll en gång per period: räkna nollgenomgångarna under 2 s.
            float forra = o.nasta(); int fall = 0, forst = -1, sist = -1;
            for (int i = 1; i < (int) (2.0 * fs); ++i)
            {
                const float v = o.nasta();
                if (forra < 0.0f && v >= 0.0f && i > 2000) { if (forst < 0) forst = i; sist = i; ++fall; }
                forra = v;
            }
            return fall > 1 ? (float) ((fall - 1) * fs / (sist - forst)) : 0.0f;
        };
        const float f220 = frekvensAv (57, 0), f110 = frekvensAv (57, -1);
        const float cent = 1200.0f * std::log2 (f220 / 220.0f), cent2 = 1200.0f * std::log2 (f110 / 110.0f);
        kontroll (std::abs (cent) < 3.0f && std::abs (cent2) < 3.0f,
                  u ("Stämningen: A3 = 220 Hz och oktav −1 = 110 Hz (") + sv (cent, 1) + " och " + sv (cent2, 1) + " ct)");
        dsp::Orgeln o; o.forbered (fs); o.installning (2, 0, 50.0f, 0.3f);
        bool ok = true; float m = 0.0f;
        for (int t = 0; t < 20; ++t) o.tonPa (36 + t * 3);   // fler toner än stämmor: stöld
        for (int i = 0; i < (int) fs; ++i) { const float v = o.nasta(); ok = ok && std::isfinite (v); m = std::max (m, std::abs (v)); }
        kontroll (ok && m < 4.0f, "Åtta stämmor och stöld: inget NaN, rimlig nivå");
    }

    std::cout << "Vokodern\n";
    const auto rost = syntRost (2, fs, 3.0f);
    {
        const auto ut = kor (rost, fs, vok, 512);
        kontroll (andligt (ut) && rmsDb (ut, fs) > -40.0f && toppDb (ut) <= -0.29f && dc (ut) < 1.0e-3f,
                  "Med Orgeln: ljud ut (" + sv (rmsDb (ut, fs), 1) + u (" dB RMS), topp under taket, ingen likspänning"));
    }
    {
        juce::AudioBuffer<float> in (2, (int) (fs * 3.5));
        in.clear();
        for (int k = 0; k < 2; ++k) in.copyFrom (k, 0, rost, k, 0, (int) (fs * 2.0));
        const auto ut = kor (in, fs, vok, 512);
        kontroll (rmsDb (ut, fs, 3.0f, 3.5f) < -80.0f, u ("Tystnad efter tystnad: rösten tystnar, Orgeln hörs inte (") + sv (rmsDb (ut, fs, 3.0f, 3.5f), 1) + " dB)");
    }
    {
        const auto bas = sag (2, fs, 3.0f, 55.0f);
        const auto medSido = kor (rost, fs, med (vok, { { id::vokBarvag, 1.0f } }), 512, &bas);
        const auto utanSido = kor (rost, fs, med (vok, { { id::vokBarvag, 1.0f } }), 512);
        kontroll (rmsDb (medSido, fs) > -40.0f && rmsDb (utanSido, fs) < -90.0f, "Sidokedjan som bärvåg: ljud med, tyst utan");
        const auto brus = kor (rost, fs, med (vok, { { id::vokBarvag, 2.0f } }), 512);
        kontroll (rmsDb (brus, fs) > -40.0f && andligt (brus), "Brus som bärvåg");
    }
    {
        const auto s = syntRost (2, fs, 2.0f, true, 1);   // s-ljud var fjärde segment
        auto energi = [&] (float begr)
        {
            const auto ut = kor (s, fs, med (vok, { { id::vokBegriplighet, begr }, { id::orgBrus, 0.0f } }), 512);
            return rmsDb (filtrera (filtrera (ut, fs, Biquad::Typ::hogpass, 4000.0), fs, Biquad::Typ::lagpass, 8000.0), fs);
        };
        const float noll = energi (0.0f), ett = energi (1.0f);
        kontroll (ett - noll > 6.0f, u ("Begripligheten: s-ljuden hörs (+") + sv (ett - noll, 1) + u (" dB i 4–8 kHz)"));
    }
    {
        auto andelHog = [&] (float formant)
        {
            const auto ut = kor (rost, fs, med (vok, { { id::vokFormant, formant }, { id::vokBegriplighet, 0.0f } }), 512);
            return rmsDb (filtrera (ut, fs, Biquad::Typ::hogpass, 2000.0), fs) - rmsDb (ut, fs);
        };
        const float a = andelHog (0.0f), b = andelHog (12.0f);
        kontroll (b - a > 3.0f, u ("Formant +12 flyttar klangen uppåt (+") + sv (b - a, 1) + u (" dB över 2 kHz)"));
    }
    {
        juce::AudioBuffer<float> in (2, (int) (fs * 2.0));
        in.clear();
        for (int k = 0; k < 2; ++k) in.copyFrom (k, 0, syntRost (2, fs, 1.0f, false, 0), k, 0, (int) (fs * 0.95));
        const auto ut = kor (in, fs, vok, 512, nullptr, vidTid (fs, 0.2f, id::vokFrys, 1.0f));
        const float fore = rmsDb (ut, fs, 0.05f, 0.2f), efter = rmsDb (ut, fs, 1.5f, 2.0f);
        kontroll (efter > fore - 6.0f, u ("FRYS håller stavelsen när rösten tystnar (") + sv (efter - fore, 1) + u (" dB mot före)"));
    }
    {
        const auto vokal = syntRost (2, fs, 2.0f, false, 0);
        const auto stilla = kor (vokal, fs, vok, 512);
        PerBlock byt = [fs] (RostverketProcessor& p, int s, int, juce::MidiBuffer&)
        {
            satt (p, id::vokBand, (float) ((int) (s / (fs * 0.25)) % 4));
        };
        const auto bytt = kor (vokal, fs, vok, 512, nullptr, byt);
        kontroll (klick (bytt) < 2.0f * klick (stilla) && andligt (bytt), "Byte av bandantal ger inga klick (" + sv (klick (bytt) / klick (stilla), 2) + u ("×)"));
        PerBlock vaxla = [fs] (RostverketProcessor& p, int s, int, juce::MidiBuffer&)
        {
            const int steg = (int) (s / (fs * 0.2));
            satt (p, id::forbi, (float) (steg % 2)); satt (p, id::vokPa, (float) ((steg / 2) % 2 == 0 ? 1 : 0)); satt (p, id::portPa, (float) ((steg / 3) % 2));
        };
        const auto vaxlat = kor (vokal, fs, vok, 512, nullptr, vaxla);
        const float ref = std::max (klick (stilla), klick (vokal));
        kontroll (klick (vaxlat) < 2.0f * ref, u ("PÅ LUFTEN, Vokodern och Porten av och på ger inga klick (") + sv (klick (vaxlat) / ref, 2) + u ("×)"));
    }
    {
        const auto tyst = kor (rost, fs, med (vok, { { id::midiIn, 1.0f } }), 512);
        const auto spelad = kor (rost, fs, med (vok, { { id::midiIn, 1.0f } }), 512, nullptr, hallTon (48));
        kontroll (rmsDb (tyst, fs) < -90.0f && rmsDb (spelad, fs) > -40.0f, u ("MIDI på: Orgeln tyst utan toner (") + sv (rmsDb (tyst, fs), 1) + u (" dB), spelar med (") + sv (rmsDb (spelad, fs), 1) + " dB)");
    }
    {
        const auto m = syntRost (1, fs, 2.0f);
        const auto ut = kor (m, fs, med (vok, { { id::portPa, 1.0f } }), 512);
        kontroll (andligt (ut) && rmsDb (ut, fs) > -40.0f, "Mono in med Porten och Vokodern");
    }

    std::cout << "Porten\n";
    {
        const auto brum = sinus (2, fs, 1.0f, 50.0f, 0.3f);
        const auto ut = kor (brum, fs, { { id::portPa, 1.0f }, { id::portHogpass, 300.0f }, { id::portKompression, 0.0f }, { id::portNarhet, 0.0f } }, 512);
        kontroll (rmsDb (brum, fs) - rmsDb (ut, fs, 0.2f) > 20.0f, u ("Högpass 300 Hz tar bort 50 Hz-brum (−") + sv (rmsDb (brum, fs) - rmsDb (ut, fs, 0.2f), 1) + " dB)");
        const auto sus = sinus (2, fs, 1.0f, 1000.0f, 0.0003f);   // −70 dBFS
        const auto g = kor (sus, fs, { { id::portPa, 1.0f }, { id::portTroskel, -40.0f }, { id::portKompression, 0.0f } }, 512);
        kontroll (rmsDb (sus, fs) - rmsDb (g, fs, 0.3f) > 40.0f, "Grinden stänger under tröskeln");
        const auto ut2 = kor (rost, fs, { { id::portPa, 1.0f }, { id::portKompression, 1.0f }, { id::portAvsibilant, 1.0f }, { id::portNarhet, 1.0f } }, 512);
        kontroll (andligt (ut2) && toppDb (ut2) <= -0.29f && dc (ut2) < 1.0e-3f, "Kompression, närhet och avsibilant: inget NaN, under taket");
    }

    std::cout << "Strupen\n";
    {
        const auto vokal = syntRost (2, fs, 3.0f, false, 0);
        const float in0 = f0Av (vokal, fs, 0.0f, 3.0f);
        const Installning strup { { id::strupPa, 1.0f }, { id::strupBlandning, 1.0f }, { id::strupFormant, 0.0f } };
        const auto upp = kor (vokal, fs, med (strup, { { id::strupLage, 0.0f }, { id::strupTonhojd, 7.0f } }), 512);
        const float c1 = cent (f0Av (upp, fs, 0.2f, 3.0f), in0 * 1.4983f);
        kontroll (std::abs (c1) < 25.0f, u ("SKIFT +7 st: grundtonen en kvint upp (") + sv (c1, 1) + " ct)");
        const auto ned = kor (vokal, fs, med (strup, { { id::strupLage, 1.0f }, { id::strupTonhojd, -12.0f } }), 512);
        const float c2 = cent (f0Av (ned, fs, 0.2f, 3.0f), in0 * 0.5f);
        kontroll (std::abs (c2) < 30.0f, u ("OKTAV −12 st: en oktav ned (") + sv (c2, 1) + " ct)");
        const auto monot = kor (vokal, fs, med (strup, { { id::strupLage, 2.0f }, { id::strupTon, 57.0f }, { id::strupFoljsamhet, 1.0f } }), 512);
        const float c3 = cent (f0Av (monot, fs, 0.2f, 3.0f), 220.0f);
        kontroll (std::abs (c3) < 35.0f, u ("MONOTON: rösten dras till A3, 220 Hz (") + sv (c3, 1) + " ct)");
        const auto noll = kor (vokal, fs, med (strup, { { id::strupTonhojd, 0.0f } }), 512);
        const auto ref = fordrojd (vokal, L);
        const float kr = korrelation (noll, 0, ref, 0, (int) (0.2 * fs), (int) (2.9 * fs));
        kontroll (kr > 0.85f, u ("0 st: rösten ligger i fas med den torra (korrelation ") + sv (kr, 2) + ")");
        auto andelHog = [&] (const juce::AudioBuffer<float>& b) { return rmsDb (filtrera (b, fs, Biquad::Typ::hogpass, 1500.0), fs, 0.2f) - rmsDb (b, fs, 0.2f); };
        const auto form = kor (vokal, fs, med (strup, { { id::strupTonhojd, 0.0f }, { id::strupFormant, 12.0f } }), 512);
        const float dh = andelHog (form) - andelHog (noll);
        const float c4 = cent (f0Av (form, fs, 0.2f, 3.0f), in0);
        kontroll (dh > 2.0f && std::abs (c4) < 30.0f, u ("Formant +12 st flyttar klangen (+") + sv (dh, 1) + u (" dB över 1,5 kHz) men inte tonen (") + sv (c4, 1) + " ct)");
    }

    std::cout << u ("Sändaren, Kören och Ekot\n");
    {
        const Installning sand { { id::sandPa, 1.0f }, { id::sandBlandning, 1.0f } };
        bool ok = true;
        juce::String niv;
        for (int l = 0; l < 3; ++l)
        {
            const auto ut = kor (rost, fs, med (sand, { { id::sandLage, (float) l }, { id::sandStyrka, 0.8f }, { id::sandFrekvens, l == 1 ? 300.0f : 4000.0f } }), 512);
            const float d = rmsDb (ut, fs) - rmsDb (rost, fs);
            niv << (l > 0 ? ", " : "") << (d >= 0.0f ? "+" : "") << sv (d, 1);
            ok = ok && andligt (ut) && std::abs (d) < 8.0f && ! bitexakt (ut, fordrojd (rost, L)) && toppDb (ut) <= -0.29f;
        }
        kontroll (ok, u ("TALCHIP, RING och KORTVÅG: inget NaN, nivån inom ±8 dB (") + niv + u (" dB), under taket"));
        const auto kv = kor (rost, fs, med (sand, { { id::sandLage, 2.0f }, { id::sandStyrka, 0.8f }, { id::sandTon, 0.5f } }), 512);
        const float hog = rmsDb (filtrera (kv, fs, Biquad::Typ::hogpass, 6000.0, 0.707, 3), fs) - rmsDb (kv, fs);
        kontroll (hog < -25.0f, u ("KORTVÅG är smal: ") + sv (hog, 1) + u (" dB över 6 kHz"));
    }
    {
        const auto r = syntRost (2, fs, 3.0f);   // samma i båda kanalerna
        const auto ut = kor (r, fs, { { id::korPa, 1.0f }, { id::korLage, 1.0f }, { id::korBredd, 1.0f }, { id::korBlandning, 0.7f } }, 512);
        const float kr = korrelation (ut, 0, ut, 1, (int) (0.2 * fs), (int) (2.9 * fs));
        const auto db = kor (r, fs, { { id::korPa, 1.0f }, { id::korLage, 0.0f } }, 512);
        kontroll (kr < 0.95f && std::abs (rmsDb (db, fs) - rmsDb (r, fs)) < 3.0f && andligt (ut),
                  u ("Kören breddar (korrelation V/H ") + sv (kr, 2) + u ("), DUBBEL håller nivån"));
    }
    {
        juce::AudioBuffer<float> imp (2, (int) (fs * 1.5)); imp.clear();
        const int t0 = (int) (0.1 * fs);
        imp.setSample (0, t0, 0.8f); imp.setSample (1, t0, 0.8f);
        const auto ut = kor (imp, fs, { { id::ekoPa, 1.0f }, { id::ekoTid, 3.0f }, { id::ekoAterkoppling, 0.0f }, { id::ekoRum, 0.0f },
                                        { id::ekoBlandning, 0.5f }, { id::ekoPingpong, 0.0f } }, 512);
        int bast = 0; float bv = 0.0f;
        for (int i = t0 + L + (int) (0.2 * fs); i < t0 + L + (int) (0.6 * fs); ++i) if (std::abs (ut.getSample (0, i)) > bv) { bv = std::abs (ut.getSample (0, i)); bast = i; }
        const float ms = (float) (bast - t0 - L) / (float) fs * 1000.0f;
        kontroll (std::abs (ms - 375.0f) < 3.0f, u ("Ekot 1/8. i 120 BPM kommer efter ") + sv (ms, 1) + " ms (375 ms)");
    }

    std::cout << "Hela bordet\n";
    const Installning allt { { id::portPa, 1.0f }, { id::vokPa, 1.0f }, { id::strupPa, 1.0f }, { id::sandPa, 1.0f }, { id::korPa, 1.0f },
                             { id::ekoPa, 1.0f }, { id::ekoRum, 0.5f }, { id::ekoAterkoppling, 60.0f } };
    {
        juce::AudioBuffer<float> in (2, (int) (fs * 12.0)); in.clear();
        for (int k = 0; k < 2; ++k) in.copyFrom (k, 0, rost, k, 0, (int) (fs * 2.0));
        const auto ut = kor (in, fs, allt, 512);
        kontroll (rmsDb (ut, fs, 11.0f, 12.0f) < -80.0f && andligt (ut), u ("Tystnad efter tystnad med alla moduler (") + sv (rmsDb (ut, fs, 11.0f, 12.0f), 1) + " dB efter 9 s)");
    }
    {
        const auto vokal = syntRost (2, fs, 3.0f, false, 0);
        const auto stilla = kor (vokal, fs, allt, 512);
        PerBlock vaxla = [fs] (RostverketProcessor& p, int s, int, juce::MidiBuffer&)
        {
            const int steg = (int) (s / (fs * 0.2));
            satt (p, id::strupPa, (float) (steg % 2)); satt (p, id::sandPa, (float) ((steg / 2) % 2));
            satt (p, id::korPa, (float) ((steg / 3) % 2)); satt (p, id::ekoPa, (float) ((steg / 4) % 2 == 0 ? 1 : 0));
        };
        const auto vaxlat = kor (vokal, fs, allt, 512, nullptr, vaxla);
        const float ref = std::max (klick (stilla), klick (vokal));
        kontroll (klick (vaxlat) < 2.0f * ref, u ("Strupen, Sändaren, Kören och Ekot av och på ger inga klick (") + sv (klick (vaxlat) / ref, 2) + u ("×)"));
    }
    {
        const Installning omv { { id::vokPa, 1.0f }, { id::vokBlandning, 0.4f }, { id::strupPa, 1.0f }, { id::strupLage, 2.0f }, { id::strupFoljsamhet, 0.3f },
                                { id::sandPa, 1.0f }, { id::sandBlandning, 0.3f } };
        const auto a = kor (rost, fs, omv, 512), b = kor (rost, fs, med (omv, { { id::omvandlingen, 1.0f } }), 512);
        const float d = rmsDb (b, fs) - rmsDb (a, fs);
        kontroll (std::abs (d) < 3.0f && ! bitexakt (a, b), u ("Omvandlingen ändrar klangen men inte nivån (") + sv (d, 1) + " dB)");
    }
    {
        const auto m = syntRost (1, fs, 2.0f);
        const auto ut = kor (m, fs, allt, 512);
        kontroll (andligt (ut) && rmsDb (ut, fs) > -40.0f, "Mono in med alla moduler");
    }

    std::cout << "Fabriksbandet\n";
    {
        const auto band = fabriksbandet();
        const auto r = syntRost (2, fs, 3.0f);
        const auto bas = sag (2, fs, 3.0f, 55.0f);
        const float torr = rmsDb (r, fs);
        bool ok = true;
        std::vector<float> niv;
        for (int i = 1; i < (int) band.size(); ++i)
        {
            const auto ut = kor (r, fs, programInst (i), 512, &bas);
            niv.push_back (rmsDb (ut, fs) - torr);
            ok = ok && andligt (ut) && toppDb (ut) <= -0.29f && dc (ut) < 1.0e-3f;
        }
        kontroll (ok, juce::String (band.size()) + u (" program: inget NaN, under taket, ingen likspänning"));
        // Nivåerna är satta på talsyntesen (RVRender --program); här räcker det att de ligger samlat.
        auto sorterad = niv; std::sort (sorterad.begin(), sorterad.end());
        const float median = sorterad[sorterad.size() / 2];
        int utanfor = 0; for (float d : niv) if (std::abs (d - median) > 4.0f) ++utanfor;
        kontroll (utanfor == 0, u ("Programmen ligger inom ±4 dB från varandras median (") + sv (sorterad.front() - median, 1) + u (" … +")
                                + sv (sorterad.back() - median, 1) + " dB)");
        RostverketProcessor p;
        satt (p, id::forbi, 1.0f); satt (p, id::midiIn, 1.0f);
        p.programbank.ladda (5);
        kontroll (p.apvts.getParameter (id::forbi)->getValue() > 0.5f && p.apvts.getParameter (id::midiIn)->getValue() > 0.5f
                  && p.getProgramName (5) == band[5].namn, u ("Ett program rör inte förbikopplingen eller MIDI in"));
        juce::MemoryBlock mb; p.getStateInformation (mb);
        RostverketProcessor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
        kontroll (q.getCurrentProgram() == 5, u ("Programmets namn följer med tillståndet"));
    }

    std::cout << "Prestanda\n";
    {
        const auto lang = syntRost (2, fs, 10.0f);
        const auto start = juce::Time::getMillisecondCounterHiRes();
        kor (lang, fs, med (vok, { { id::portPa, 1.0f }, { id::portAvsibilant, 1.0f } }), 512);
        const double andel = (juce::Time::getMillisecondCounterHiRes() - start) / 10000.0 * 100.0;
        kontroll (andel < 3.0, "CPU: Porten och Vokodern med 20 band, stereo, 48 kHz = " + sv (andel, 2) + u (" % av en kärna"));
    }
    {
        const auto lang = syntRost (2, fs, 10.0f);
        const auto start = juce::Time::getMillisecondCounterHiRes();
        kor (lang, fs, med (allt, { { id::korLage, 2.0f } }), 512);
        const double andel = (juce::Time::getMillisecondCounterHiRes() - start) / 10000.0 * 100.0;
        kontroll (andel < 8.0, u ("CPU: hela bordet (Kören MASSA), stereo, 48 kHz = ") + sv (andel, 2) + u (" % av en kärna"));
    }

    std::cout << (fel == 0 ? "Alla kontroller gick igenom.\n" : "Kontroller som inte gick igenom: ");
    if (fel) std::cout << fel << "\n";
    return fel == 0 ? 0 : 1;
}

// --------------------------------------------------------------------------------------------
// Filer

bool lasWav (const juce::File& f, juce::AudioBuffer<float>& buf, double& fs)
{
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
    if (! r) { std::cout << "Kunde inte läsa " << f.getFullPathName() << "\n"; return false; }
    buf.setSize (2, (int) r->lengthInSamples);
    r->read (&buf, 0, (int) r->lengthInSamples, 0, true, true);
    if (r->numChannels == 1) buf.copyFrom (1, 0, buf, 0, 0, buf.getNumSamples());
    fs = r->sampleRate;
    return true;
}

bool skrivWav (const juce::File& f, const juce::AudioBuffer<float>& buf, double fs)
{
    f.deleteFile();
    f.getParentDirectory().createDirectory();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> st (f.createOutputStream());
    std::unique_ptr<juce::AudioFormatWriter> w (st ? wav.createWriterFor (st.get(), fs, (unsigned) buf.getNumChannels(), 24, {}, 0) : nullptr);
    if (! w) { std::cout << "Kunde inte skriva " << f.getFullPathName() << "\n"; return false; }
    st.release();
    w->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
    std::cout << "  " << f.getFileName() << "\n";
    return true;
}

// Rösten upprepad till 'sek' sekunder.
juce::AudioBuffer<float> upprepa (const juce::AudioBuffer<float>& in, double fs, float sek)
{
    juce::AudioBuffer<float> ut (2, (int) (fs * sek));
    ut.clear();
    for (int s = 0; s < ut.getNumSamples(); s += in.getNumSamples())
        for (int k = 0; k < 2; ++k) ut.copyFrom (k, s, in, k, 0, std::min (in.getNumSamples(), ut.getNumSamples() - s));
    return ut;
}

int exempel (const juce::File& rostFil, const juce::File& basFil, const juce::File& ut)
{
    juce::AudioBuffer<float> rost, bas;
    double fs = 48000.0, fsBas = 48000.0;
    if (! lasWav (rostFil, rost, fs) || ! lasWav (basFil, bas, fsBas)) return 1;
    if (std::abs (fs - fsBas) > 1.0) { std::cout << "Rösten och basen måste ha samma samplingsfrekvens\n"; return 1; }
    const float sek = (float) rost.getNumSamples() / (float) fs;
    std::cout << "Grind 1-exemplen till " << ut.getFullPathName() << "\n";
    skrivWav (ut.getChildFile ("0_rost_torr.wav"), rost, fs);

    const Installning grund { { id::portPa, 1.0f }, { id::portTroskel, -50.0f }, { id::vokPa, 1.0f } };

    // 1a: ackord i c-moll från MIDI, två sekunder var: Cm – Ab – Bb – G.
    {
        static const int ackord[4][4] { { 48, 55, 60, 63 }, { 44, 51, 56, 60 }, { 46, 53, 58, 62 }, { 43, 50, 55, 59 } };
        PerBlock spela = [fs] (RostverketProcessor&, int s, int len, juce::MidiBuffer& m)
        {
            const int per = (int) (2.0 * fs);
            for (int i = s; i < s + len; ++i)
                if (i % per == 0)
                {
                    const int a = (i / per) % 4, f = (i / per + 3) % 4;
                    if (i > 0) for (int t : ackord[f]) m.addEvent (juce::MidiMessage::noteOff (1, t), i - s);
                    for (int t : ackord[a]) m.addEvent (juce::MidiMessage::noteOn (1, t, (juce::uint8) 100), i - s);
                }
        };
        skrivWav (ut.getChildFile ("1a_sagackord_midi.wav"),
                  kor (upprepa (rost, fs, 8.0f), fs, med (grund, { { id::midiIn, 1.0f }, { id::orgVag, 0.0f }, { id::orgBrus, 0.08f } }), 512, nullptr, spela), fs);
    }
    // 1b: Kroppsmaskinens bas i sidokedjan.
    {
        const auto r = upprepa (rost, fs, std::min (16.0f, (float) bas.getNumSamples() / (float) fs));
        skrivWav (ut.getChildFile ("1b_sidokedja_kolvbas.wav"),
                  kor (r, fs, med (grund, { { id::vokBarvag, 1.0f }, { id::vokSnabbhet, 0.8f }, { id::vokBegriplighet, 0.6f } }), 512, &bas), fs);
    }
    // 1c: viskning – brus som bärvåg, snabb och begriplig.
    skrivWav (ut.getChildFile ("1c_viskning_brus.wav"),
              kor (rost, fs, med (grund, { { id::vokBarvag, 2.0f }, { id::vokSnabbhet, 0.85f }, { id::vokBegriplighet, 0.8f }, { id::vokSkarpa, 0.3f } }), 512), fs);
    // 1d: frusen stavelse spelad som orgel. Frys där rösten är starkast under första sekunden.
    {
        int starkast = 0; float m = 0.0f;
        krang::dsp::Foljare f; f.satt (fs, 0.005f, 0.05f);
        for (int i = 0; i < std::min (rost.getNumSamples(), (int) (1.5 * fs)); ++i)
        {
            const float v = f.process (rost.getSample (0, i));
            if (v > m && i > (int) (0.3 * fs)) { m = v; starkast = i; }
        }
        const float frysVid = (float) starkast / (float) fs;
        static const int melodi[8] { 60, 63, 67, 72, 70, 67, 63, 65 };
        PerBlock spela = [fs, frysVid] (RostverketProcessor& p, int s, int len, juce::MidiBuffer& mb)
        {
            const int t = (int) (frysVid * fs);
            if (s <= t && t < s + len) satt (p, id::vokFrys, 1.0f);
            const int start = (int) ((frysVid + 0.3f) * fs), per = (int) (0.5 * fs);
            for (int i = s; i < s + len; ++i)
                if (i >= start && (i - start) % per == 0)
                {
                    const int nr = (i - start) / per;
                    if (nr > 0) mb.addEvent (juce::MidiMessage::noteOff (1, melodi[(nr - 1) % 8]), i - s);
                    if (nr < 12) mb.addEvent (juce::MidiMessage::noteOn (1, melodi[nr % 8], (juce::uint8) 100), i - s);
                }
            if (s == 0) mb.addEvent (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100), 0);
            if (s <= start && start < s + len) mb.addEvent (juce::MidiMessage::noteOff (1, 48), start - s);
        };
        juce::AudioBuffer<float> in (2, (int) ((frysVid + 7.0f) * fs));
        in.clear();
        for (int k = 0; k < 2; ++k) in.copyFrom (k, 0, rost, k, 0, std::min (rost.getNumSamples(), in.getNumSamples()));
        skrivWav (ut.getChildFile ("1d_frusen_stavelse_orgel.wav"),
                  kor (in, fs, med (grund, { { id::midiIn, 1.0f }, { id::orgVag, 2.0f }, { id::vokSnabbhet, 0.5f }, { id::orgGlid, 60.0f } }), 512, nullptr, spela), fs);
    }
    // 1e: roboten – en hållen ton (utan MIDI), puls en oktav ned, formanten nedåt.
    skrivWav (ut.getChildFile ("1e_robot_puls_formant.wav"),
              kor (rost, fs, med (grund, { { id::orgVag, 1.0f }, { id::orgOktav, 1.0f }, { id::orgTon, 50.0f }, { id::vokFormant, -4.0f }, { id::vokBand, 1.0f } }), 512), fs);
    juce::ignoreUnused (sek);
    return 0;
}

// Ett programs inställning (värdena utöver grundvärdena, som Programbank::ladda sätter dem).
Installning programInst (int i)
{
    Installning inst;
    const auto band = fabriksbandet();   // hålls kvar: varden pekar in i vektorn
    for (auto& [pid, v] : band[(size_t) i].varden) inst.push_back ({ pid, v });
    return inst;
}

int program (const juce::File& rostFil, const juce::File& utMapp)
{
    juce::AudioBuffer<float> rost; double fs = 48000.0;
    if (! lasWav (rostFil, rost, fs)) return 1;
    // Sidokedjan till "Basen talar": en sågtandsbas i åttondelar (A1–C2) under rösten.
    juce::AudioBuffer<float> bas (2, rost.getNumSamples());
    double fas = 0.0;
    for (int i = 0; i < bas.getNumSamples(); ++i)
    {
        const int steg = (int) (i / (0.24 * fs));
        fas += (steg % 4 == 3 ? 65.4 : 55.0) / fs; if (fas >= 1.0) fas -= 1.0;
        for (int k = 0; k < 2; ++k) bas.setSample (k, i, 0.4f * (float) (2.0 * fas - 1.0));
    }
    const float torr = rmsDb (rost, fs);
    const auto band = fabriksbandet();
    std::cout << "Fabriksbandet på " << rostFil.getFileName() << " (torr " << sv (torr, 1) << " dB RMS)\n";
    for (int i = 0; i < (int) band.size(); ++i)
    {
        int lat = 0;
        const auto ut = kor (rost, fs, programInst (i), 512, &bas, {}, &lat);
        juce::AudioBuffer<float> rak (2, ut.getNumSamples() - lat);
        for (int k = 0; k < 2; ++k) rak.copyFrom (k, 0, ut, k, lat, rak.getNumSamples());
        const float d = rmsDb (rak, fs) - torr;
        std::cout << "  " << juce::String (i + 1).paddedLeft ('0', 2) << "  " << (band[(size_t) i].kategori + " / " + band[(size_t) i].namn).paddedRight (' ', 34)
                  << (d >= 0.0f ? " +" : " ") << sv (d, 1) << " dB" << (toppDb (rak) > -0.31f ? "  (limitern tar)" : "") << "\n";
        const auto namn = juce::String (i + 1).paddedLeft ('0', 2) + "_" + juce::File::createLegalFileName (band[(size_t) i].namn).replaceCharacter (' ', '_');
        if (utMapp != juce::File()) skrivWav (utMapp.getChildFile (namn + ".wav"), rak, fs);
    }
    return 0;
}

int state (const juce::File& fil, const Installning& inst)
{
    RostverketProcessor p;
    for (auto& [k, v] : inst) satt (p, k, v);
    auto tillstand = p.apvts.copyState();
    tillstand.setProperty ("rvFormat", RostverketProcessor::formatVersion, nullptr);
    auto xml = tillstand.createXml();
    return fil.replaceWithText (xml->toString (juce::XmlElement::TextFormat().singleLine().withoutHeader())) ? 0 : 1;
}

// Röstspåret i en testlåt: ljudobjekten läggs ut, MIDI-tonerna och automationen (stegvis, i riktiga
// enheter) spelas i rätt sample, och latensen tas bort så att utgången ligger i fas med låten.
int projekt (const juce::File& jsonFil, const juce::File& ut)
{
    const auto j = juce::JSON::parse (jsonFil);
    if (! j.isObject()) { std::cout << "Kunde inte läsa " << jsonFil.getFullPathName() << "\n"; return 1; }
    const double fs = 48000.0;
    korBpm = (double) j["tempo"];
    const int n = (int) ((double) j["langd"] * fs);
    juce::AudioBuffer<float> in (2, n); in.clear();
    if (auto* obj = j["objekt"].getArray())
        for (auto& o : *obj)
        {
            juce::AudioBuffer<float> b; double f = fs;
            if (! lasWav (jsonFil.getParentDirectory().getChildFile (o["fil"].toString()), b, f)) return 1;
            const int start = (int) ((double) o["start"] * fs);
            const float g = o.hasProperty ("gain") ? (float) (double) o["gain"] : 1.0f;
            for (int k = 0; k < 2; ++k)
                if (start < n) in.addFrom (k, start, b, k, 0, std::min (b.getNumSamples(), n - start), g);
        }
    Installning lage;
    if (auto* d = j["lage"].getDynamicObject())
        for (auto& e : d->getProperties()) lage.push_back ({ e.name.toString(), (float) (double) e.value });
    struct Steg { int t; juce::String pid; float v; };
    std::vector<Steg> steg;
    if (auto* d = j["automation"].getDynamicObject())
        for (auto& e : d->getProperties())
            if (auto* pts = e.value.getArray())
                for (auto& pt : *pts) steg.push_back ({ (int) ((double) pt[0] * fs), e.name.toString(), (float) (double) pt[1] });
    struct Ton { int t; bool pa; int ton; int vel; };
    std::vector<Ton> toner;
    if (auto* nl = j["noter"].getArray())
        for (auto& nt : *nl)
        {
            const int t0 = (int) ((double) nt[0] * fs), t1 = (int) (((double) nt[0] + (double) nt[1]) * fs);
            toner.push_back ({ t0, true, (int) nt[2], (int) ((double) nt[3] * 127.0) });
            toner.push_back ({ t1, false, (int) nt[2], 0 });
        }
    PerBlock pb = [&] (RostverketProcessor& p, int s, int len, juce::MidiBuffer& m)
    {
        for (auto& st : steg) if (st.t >= s && st.t < s + len) satt (p, st.pid, st.v);
        for (auto& t : toner)
            if (t.t >= s && t.t < s + len)
                m.addEvent (t.pa ? juce::MidiMessage::noteOn (1, t.ton, (juce::uint8) juce::jlimit (1, 127, t.vel)) : juce::MidiMessage::noteOff (1, t.ton), t.t - s);
    };
    int lat = 0;
    const auto r = kor (in, fs, lage, 512, nullptr, pb, &lat);
    juce::AudioBuffer<float> rak (2, n - lat);
    for (int k = 0; k < 2; ++k) rak.copyFrom (k, 0, r, k, lat, n - lat);
    std::cout << "Röstspåret (" << sv ((double) n / fs, 1) << " s):\n";
    return skrivWav (ut, rak, fs) ? 0 : 1;
}

int parametrar (const juce::File& fil)
{
    RostverketProcessor p;
    juce::String ut;
    int i = 0;
    for (auto* par : p.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (par))
        {
            const auto ri = r->getNormalisableRange();
            ut << i++ << "\t" << r->getParameterID() << "\t" << r->getName (64) << "\t" << ri.start << "\t" << ri.end
               << "\t" << r->convertFrom0to1 (r->getDefaultValue()) << "\n";
        }
    if (! fil.replaceWithText (ut)) { std::cout << "Kunde inte skriva " << fil.getFullPathName() << "\n"; return 1; }
    std::cout << i << " parametrar skrivna till " << fil.getFullPathName() << "\n";
    return 0;
}

int rendera (const juce::File& in, const juce::File& ut, const Installning& inst)
{
    juce::AudioBuffer<float> buf; double fs = 48000.0;
    if (! lasWav (in, buf, fs)) return 1;
    return skrivWav (ut, kor (buf, fs, inst, 512), fs) ? 0 : 1;
}

} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juce;
    const auto cwd = juce::File::getCurrentWorkingDirectory();
    const juce::String a1 = argc > 1 ? juce::String (argv[1]) : juce::String();
    if (a1 == "--test") return test();
    if (a1 == "--parametrar" && argc >= 3) return parametrar (cwd.getChildFile (argv[2]));
    if (a1 == "--program" && argc >= 3) return program (cwd.getChildFile (argv[2]), argc >= 4 ? cwd.getChildFile (argv[3]) : juce::File());
    if (a1 == "--projekt" && argc >= 4) return projekt (cwd.getChildFile (argv[2]), cwd.getChildFile (argv[3]));
    if (a1 == "--state" && argc >= 3)
    {
        Installning inst;
        for (int i = 3; i < argc; ++i)
        {
            const juce::String a (argv[i]);
            inst.push_back ({ a.upToFirstOccurrenceOf ("=", false, false), a.fromFirstOccurrenceOf ("=", false, false).getFloatValue() });
        }
        return state (cwd.getChildFile (argv[2]), inst);
    }
    if (a1 == "--exempel" && argc >= 5) return exempel (cwd.getChildFile (argv[2]), cwd.getChildFile (argv[3]), cwd.getChildFile (argv[4]));
    if (argc >= 3 && ! a1.startsWith ("--"))
    {
        Installning inst;
        for (int i = 3; i < argc; ++i)
        {
            const juce::String a (argv[i]);
            inst.push_back ({ a.upToFirstOccurrenceOf ("=", false, false), a.fromFirstOccurrenceOf ("=", false, false).getFloatValue() });
        }
        return rendera (cwd.getChildFile (argv[1]), cwd.getChildFile (argv[2]), inst);
    }
    std::cout << "Användning: RVRender --test | --parametrar fil.txt | --state fil.xml [id=värde ...] | --projekt låt.json ut.wav\n"
                 "                | --exempel röst.wav bas.wav utmapp | in.wav ut.wav [id=värde ...]\n";
    return 1;
}
