// Röstverket – processorn: Porten → Vokodern (med bärvåg från Orgeln, sidokedjan eller brus) →
// Strupen → Sändaren → Kören → Ekot → torr/våt → utnivå → limiter, och PÅ LUFTEN.
// Omvandlingen (makrospaken) drar de påslagna modulerna från människa mot maskin.
//
// Bussarna: stereo in och ut, och en sidokedja (bärvåg till Vokodern) i VST3 och CLAP. MIDI in spelar
// Orgeln när MIDI-brytaren är på; annars spelar Orgeln tonen på ratten TON.
//
// Init och förbikoppling är bitexakta (fördröjda med latensen): när ingen modul är på och nivåerna står
// på 0 dB passerar ljudet orört. Moduler och förbikoppling tonas in och ut (10 ms), och en modul som slås
// på börjar från noll.
//
// Latensen är fast, 25 ms (Strupens fönster), och rapporteras alltid – även när Strupen är av. Den torra
// vägen och förbikopplingen fördröjs lika mycket.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <krang/dsp/Grund.h>
#include <krang/dsp/Limiter.h>
#include <krang/app/Inspelaren.h>
#include <array>
#include <atomic>
#include <functional>
#include "Parameters.h"
#include "presets/Fabriksbandet.h"
#include "dsp/Porten.h"
#include "dsp/Orgeln.h"
#include "dsp/Vokodern.h"
#include "dsp/Strupen.h"
#include "dsp/Sandaren.h"
#include "dsp/Koren.h"
#include "dsp/Ekot.h"

class RostverketProcessor : public juce::AudioProcessor
{
public:
    RostverketProcessor();

    void prepareToPlay (double fs, int maxBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return juce::String::fromUTF8 ("Röstverket"); }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }   // Ekot och rummet
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter (rv::id::forbi); }

    // Värdens programlista är fabriksbandet; egna program nås från panelen.
    int getNumPrograms() override { return programbank.antalFabrik(); }
    int getCurrentProgram() override { return juce::jmin (programbank.aktuellt(), programbank.antalFabrik() - 1); }
    // Samma program igen gör ingenting: ett inläst projekt ska inte nollställas av värdens programbyte.
    void setCurrentProgram (int i) override { if (i != programbank.aktuellt()) programbank.ladda (i); }
    const juce::String getProgramName (int i) override { return programbank.program (i).namn; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    // Programmen: fabriksbandet och egna program. Förbikopplingen och MIDI in rörs aldrig av ett program.
    krang::presets::Programbank programbank { apvts, "Rostverket", "rvprog", rv::fabriksbandet(),
                                              { rv::id::forbi, rv::id::midiIn } };
    // Appen: ljudkortet (sätts av krang::app::FristaendeApp) och inspelaren av utgången.
    juce::AudioDeviceManager* enheter = nullptr;
    // Mikrofonen i appen (JUCE:s "tysta ingången"); sätts av Source/app/App.cpp, tomt i pluginet.
    struct App { std::function<bool()> mikrofonPa; std::function<void (bool)> sattMikrofon; };
    inline static App app;
    krang::app::Inspelaren inspelaren { juce::String::fromUTF8 ("Röstverket") };

    // Mätvärden till panelen (skrivs av ljudtråden, läses av panelen).
    std::atomic<float> inNivaDb { -120.0f }, utNivaDb { -120.0f }, limiterDb { 0.0f };
    std::array<std::atomic<float>, rv::dsp::Vokodern::maxBand> bandNiva {};   // 0..1 för bandmätaren
    std::atomic<int> bandAktiva { 20 }, orgelTon { 48 };

    static constexpr int formatVersion = 1;
    static constexpr int bit = 256;     // kedjan körs i bitar om högst så här många sample

private:
    static BusesProperties bussar();
    void processBit (float* const* ljud, int kanaler, int n, const float* const* sido, int sidoKanaler, double bpm);
    // Tonar mellan buffertens nuvarande innehåll (före) och modulens utgång (efter) med rampen.
    static void tona (krang::dsp::Ramp& r, float* const* ljud, float* const* efter, int kanaler, int n);
    void lasInstallning();
    bool aktiv() const;
    float las (const char* pid) const { return apvts.getRawParameterValue (pid)->load(); }
    bool  pa  (const char* pid) const { return las (pid) > 0.5f; }

    double fs = 48000.0;
    int kanalerForberedda = 2, rampLangd = 480;
    rv::dsp::Porten porten;
    rv::dsp::Orgeln orgeln;
    rv::dsp::Vokodern vokodern;
    rv::dsp::Strupen strupen;
    rv::dsp::Sandaren sandaren;
    rv::dsp::Koren koren;
    rv::dsp::Ekot ekot;
    std::array<krang::dsp::Fordrojning, 2> torrFordr, vagFordr;
    krang::dsp::Limiter limitern;
    std::array<krang::dsp::Brus, 2> brus;
    krang::dsp::Ramp forbiR, portR, vokR, strupR, sandR, korR, ekoR;
    bool strupenNollad = true, ekotNollat = true;
    int latens = 1200;
    juce::AudioBuffer<float> torr, analys, barvag, vok, modul, strupTorr;
    std::vector<float> mono;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inGain, utGain;
    juce::SmoothedValue<float> torrVat, vokBlandning;
    bool midiVarPa = false, kedjanStilla = true;
    float inTopp = 0.0f, utTopp = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RostverketProcessor)
};
