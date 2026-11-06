// Röstverkets parametrar. ID:na har formen rv_<modul>_<vad> och får ALDRIG döpas om när de väl finns i
// ett bygge som använts. Nya parametrar läggs sist i listan, så att DAW-automation och Reaper-skripten
// står kvar.
//
// Alla modulers parametrar finns från fas 0, även för moduler vars ljud kommer i fas 1 och 2, så att
// ID:n och ordning ligger fast från början.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace rv
{
inline juce::String svt (const char* utf8) { return juce::String::fromUTF8 (utf8); }

namespace id
{
    // 1 Porten
    inline constexpr const char* portPa          = "rv_port_pa";
    inline constexpr const char* portTroskel     = "rv_port_troskel";
    inline constexpr const char* portKompression = "rv_port_kompression";
    inline constexpr const char* portHogpass     = "rv_port_hogpass";
    inline constexpr const char* portNarhet      = "rv_port_narhet";
    inline constexpr const char* portAvsibilant  = "rv_port_avsibilant";
    // 2 Vokodern
    inline constexpr const char* vokPa           = "rv_vok_pa";
    inline constexpr const char* vokBand         = "rv_vok_band";
    inline constexpr const char* vokBarvag       = "rv_vok_barvag";
    inline constexpr const char* vokFormant      = "rv_vok_formant";
    inline constexpr const char* vokSnabbhet     = "rv_vok_snabbhet";
    inline constexpr const char* vokSkarpa       = "rv_vok_skarpa";
    inline constexpr const char* vokBegriplighet = "rv_vok_begriplighet";
    inline constexpr const char* vokFrys         = "rv_vok_frys";
    inline constexpr const char* vokBlandning    = "rv_vok_blandning";
    // Orgeln – vokoderns inbyggda bärvåg
    inline constexpr const char* orgVag          = "rv_org_vag";
    inline constexpr const char* orgOktav        = "rv_org_oktav";
    inline constexpr const char* orgGlid         = "rv_org_glid";
    inline constexpr const char* orgBrus         = "rv_org_brus";
    inline constexpr const char* orgTon          = "rv_org_ton";
    // 3 Strupen
    inline constexpr const char* strupPa         = "rv_strup_pa";
    inline constexpr const char* strupLage       = "rv_strup_lage";
    inline constexpr const char* strupTonhojd    = "rv_strup_tonhojd";
    inline constexpr const char* strupFormant    = "rv_strup_formant";
    inline constexpr const char* strupFoljsamhet = "rv_strup_foljsamhet";
    inline constexpr const char* strupBlandning  = "rv_strup_blandning";
    inline constexpr const char* strupTon        = "rv_strup_ton";
    // 4 Sändaren
    inline constexpr const char* sandPa          = "rv_sand_pa";
    inline constexpr const char* sandLage        = "rv_sand_lage";
    inline constexpr const char* sandStyrka      = "rv_sand_styrka";
    inline constexpr const char* sandFrekvens    = "rv_sand_frekvens";
    inline constexpr const char* sandTon         = "rv_sand_ton";
    inline constexpr const char* sandBlandning   = "rv_sand_blandning";
    inline constexpr const char* sandMidi        = "rv_sand_midi";
    // 5 Kören
    inline constexpr const char* korPa           = "rv_kor_pa";
    inline constexpr const char* korLage         = "rv_kor_lage";
    inline constexpr const char* korSpridning    = "rv_kor_spridning";
    inline constexpr const char* korSvaj         = "rv_kor_svaj";
    inline constexpr const char* korBredd        = "rv_kor_bredd";
    inline constexpr const char* korBlandning    = "rv_kor_blandning";
    // 6 Ekot
    inline constexpr const char* ekoPa           = "rv_eko_pa";
    inline constexpr const char* ekoTid          = "rv_eko_tid";
    inline constexpr const char* ekoAterkoppling = "rv_eko_aterkoppling";
    inline constexpr const char* ekoMorker       = "rv_eko_morker";
    inline constexpr const char* ekoRum          = "rv_eko_rum";
    inline constexpr const char* ekoBlandning    = "rv_eko_blandning";
    inline constexpr const char* ekoPingpong     = "rv_eko_pingpong";
    // Omvandlingen och utgången
    inline constexpr const char* omvandlingen    = "rv_omvandlingen";
    inline constexpr const char* inNiva          = "rv_ut_in";
    inline constexpr const char* torrVat         = "rv_ut_torrvat";
    inline constexpr const char* utNiva          = "rv_ut_niva";
    inline constexpr const char* limiter         = "rv_ut_limiter";
    inline constexpr const char* forbi           = "rv_forbi";       // PÅ LUFTEN släckt = förbikopplat
    inline constexpr const char* midiIn          = "rv_midi";        // MIDI in för hela Röstverket
}

namespace lagen
{
    inline juce::StringArray band()    { return { "8", "12", "16", "20" }; }
    inline juce::StringArray barvag()  { return { "Orgeln", "Sidokedja", "Brus" }; }
    inline juce::StringArray vag()     { return { svt ("Såg"), "Puls", svt ("Såg+puls") }; }
    inline juce::StringArray oktav()   { return { svt ("−2"), svt ("−1"), "0", "+1" }; }
    inline juce::StringArray strupe()  { return { "Skift", "Oktav", "Monoton" }; }
    inline juce::StringArray sandare() { return { "Talchip", "Ring", svt ("Kortvåg") }; }
    inline juce::StringArray kor()     { return { "Dubbel", svt ("Kör"), "Massa" }; }
    inline juce::StringArray tid()     { return { "1/16", "1/8T", "1/8", "1/8.", "1/4", "1/4.", "1/2" }; }
    // Ekots tider i fjärdedelar (samma som Pannrummets Schakt).
    inline float tidFjardedelar (int i)
    {
        static const float t[] { 0.25f, 1.0f / 3.0f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };
        return t[juce::jlimit (0, 6, i)];
    }
    inline int bandAntal (int i)       { static const int n[] { 8, 12, 16, 20 }; return n[juce::jlimit (0, 3, i)]; }
}

// Svenska tonnamn: H, inte B.
inline juce::String tonnamn (int ton)
{
    static const char* n[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "H" };
    return juce::String (n[((ton % 12) + 12) % 12]) + juce::String (ton / 12 - 1);
}

// Antalet parametrar. Testet i RVRender kontrollerar att det och ordningen står fast.
inline constexpr int antalParametrar = 54;

inline juce::AudioProcessorValueTreeState::ParameterLayout skapaLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    const int version = 1;
    auto pid  = [&] (const juce::String& s) { return ParameterID (s, version); };
    auto flyt = [&] (const juce::String& s, const char* namn, float lo, float hi, float std, const char* enhet = "", float mitt = -1.0f)
    {
        NormalisableRange<float> r (lo, hi);
        if (mitt > lo && mitt < hi) r.setSkewForCentre (mitt);
        l.add (std::make_unique<AudioParameterFloat> (pid (s), svt (namn), r, std, AudioParameterFloatAttributes().withLabel (svt (enhet))));
    };
    auto val  = [&] (const juce::String& s, const char* namn, const StringArray& lista, int std)
    {
        l.add (std::make_unique<AudioParameterChoice> (pid (s), svt (namn), lista, std));
    };
    auto bry  = [&] (const juce::String& s, const char* namn, bool std)
    {
        l.add (std::make_unique<AudioParameterBool> (pid (s), svt (namn), std));
    };
    auto ton  = [&] (const juce::String& s, const char* namn, int std)
    {
        l.add (std::make_unique<AudioParameterInt> (pid (s), svt (namn), 24, 84, std, AudioParameterIntAttributes()
                  .withStringFromValueFunction ([] (int v, int) { return tonnamn (v); })));
    };

    // 1 Porten
    bry  (id::portPa, "Porten på", false);   // av i Init: Init är bitexakt
    flyt (id::portTroskel, "Porten tröskel", -80.0f, 0.0f, -60.0f, "dB");
    flyt (id::portKompression, "Porten kompression", 0.0f, 1.0f, 0.3f);
    flyt (id::portHogpass, "Porten högpass", 40.0f, 300.0f, 80.0f, "Hz", 100.0f);
    flyt (id::portNarhet, "Porten närhet", 0.0f, 1.0f, 0.3f);
    bry  (id::portAvsibilant, "Porten avsibilant", false);
    // 2 Vokodern
    bry  (id::vokPa, "Vokodern på", false);
    val  (id::vokBand, "Vokodern band", lagen::band(), 3);
    val  (id::vokBarvag, "Vokodern bärvåg", lagen::barvag(), 0);
    flyt (id::vokFormant, "Vokodern formant", -12.0f, 12.0f, 0.0f, "st");
    flyt (id::vokSnabbhet, "Vokodern snabbhet", 0.0f, 1.0f, 0.6f);
    flyt (id::vokSkarpa, "Vokodern skärpa", 0.0f, 1.0f, 0.5f);
    flyt (id::vokBegriplighet, "Vokodern begriplighet", 0.0f, 1.0f, 0.5f);
    bry  (id::vokFrys, "Vokodern frys", false);
    flyt (id::vokBlandning, "Vokodern blandning", 0.0f, 1.0f, 1.0f);
    // Orgeln
    val  (id::orgVag, "Orgeln vågform", lagen::vag(), 0);
    val  (id::orgOktav, "Orgeln oktav", lagen::oktav(), 2);
    flyt (id::orgGlid, "Orgeln glid", 0.0f, 1000.0f, 0.0f, "ms", 100.0f);
    flyt (id::orgBrus, "Orgeln brus", 0.0f, 1.0f, 0.1f);
    ton  (id::orgTon, "Orgeln ton", 48);
    // 3 Strupen
    bry  (id::strupPa, "Strupen på", false);
    val  (id::strupLage, "Strupen läge", lagen::strupe(), 1);
    flyt (id::strupTonhojd, "Strupen tonhöjd", -12.0f, 12.0f, -12.0f, "st");
    flyt (id::strupFormant, "Strupen formant", -12.0f, 12.0f, 0.0f, "st");
    flyt (id::strupFoljsamhet, "Strupen följsamhet", 0.0f, 1.0f, 0.8f);
    flyt (id::strupBlandning, "Strupen blandning", 0.0f, 1.0f, 0.5f);
    ton  (id::strupTon, "Strupen ton", 48);
    // 4 Sändaren
    bry  (id::sandPa, "Sändaren på", false);
    val  (id::sandLage, "Sändaren läge", lagen::sandare(), 0);
    flyt (id::sandStyrka, "Sändaren styrka", 0.0f, 1.0f, 0.5f);
    flyt (id::sandFrekvens, "Sändaren frekvens", 20.0f, 16000.0f, 8000.0f, "Hz", 800.0f);
    flyt (id::sandTon, "Sändaren ton", 0.0f, 1.0f, 0.5f);
    flyt (id::sandBlandning, "Sändaren blandning", 0.0f, 1.0f, 1.0f);
    bry  (id::sandMidi, "Sändaren följer MIDI", false);
    // 5 Kören
    bry  (id::korPa, "Kören på", false);
    val  (id::korLage, "Kören läge", lagen::kor(), 0);
    flyt (id::korSpridning, "Kören spridning", 0.0f, 1.0f, 0.4f);
    flyt (id::korSvaj, "Kören svaj", 0.0f, 1.0f, 0.3f);
    flyt (id::korBredd, "Kören bredd", 0.0f, 1.0f, 0.7f);
    flyt (id::korBlandning, "Kören blandning", 0.0f, 1.0f, 0.5f);
    // 6 Ekot
    bry  (id::ekoPa, "Ekot på", false);
    val  (id::ekoTid, "Ekot tid", lagen::tid(), 3);
    flyt (id::ekoAterkoppling, "Ekot återkoppling", 0.0f, 95.0f, 35.0f, "%");
    flyt (id::ekoMorker, "Ekot mörker", 0.0f, 1.0f, 0.5f);
    flyt (id::ekoRum, "Ekot rum", 0.0f, 1.0f, 0.2f);
    flyt (id::ekoBlandning, "Ekot blandning", 0.0f, 1.0f, 0.25f);
    bry  (id::ekoPingpong, "Ekot ping-pong", false);
    // Omvandlingen och utgången
    flyt (id::omvandlingen, "Omvandlingen", 0.0f, 1.0f, 0.0f);
    flyt (id::inNiva, "In-nivå", -24.0f, 24.0f, 0.0f, "dB");
    flyt (id::torrVat, "Torr/våt", 0.0f, 1.0f, 1.0f);
    flyt (id::utNiva, "Utnivå", -24.0f, 12.0f, 0.0f, "dB");
    bry  (id::limiter, "Limiter", true);
    bry  (id::forbi, "Förbikoppling", false);
    bry  (id::midiIn, "MIDI in", false);
    return l;
}

} // namespace rv
