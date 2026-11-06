// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rv;

namespace
{
    // dB till gain, där värden nära 0 dB blir exakt 1 (parametrarnas normalisering ger annars 1,0000001
    // och förstör det bitexakta genomsläppet).
    float gainAv (float dB) { return std::abs (dB) < 0.01f ? 1.0f : krang::dsp::dbTillGain (dB); }
    bool vid (const krang::dsp::Ramp& r, float m) { return juce::exactlyEqual (r.v, m) && juce::exactlyEqual (r.mal, m); }   // stilla vid m
}

// Bussarna: stereo in och ut, och en sidokedja – utom i AU, där Pannrummet visade att sidokedjan ger
// fyra ingångsstift i Reaper och att ljudet inte gick igenom.
juce::AudioProcessor::BusesProperties RostverketProcessor::bussar()
{
    auto b = BusesProperties()
                 .withInput  ("In", juce::AudioChannelSet::stereo(), true)
                 .withOutput ("Ut", juce::AudioChannelSet::stereo(), true);
    if (juce::PluginHostType::getPluginLoadedAs() != juce::AudioProcessor::wrapperType_AudioUnit)
        b = b.withInput ("Sidokedja", juce::AudioChannelSet::stereo(), true);   // på från början: Reaper visar kanal 3/4
    return b;
}

RostverketProcessor::RostverketProcessor()
    : AudioProcessor (bussar()),
      apvts (*this, nullptr, "Rostverket", skapaLayout())
{
    brus[1].s = 0x9e3779b9u;
}

bool RostverketProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto ut = layouts.getMainOutputChannelSet();
    if (ut != juce::AudioChannelSet::mono() && ut != juce::AudioChannelSet::stereo()) return false;
    if (ut != layouts.getMainInputChannelSet()) return false;
    if (layouts.inputBuses.size() > 1)
    {
        const auto sido = layouts.getChannelSet (true, 1);
        if (! sido.isDisabled() && sido != juce::AudioChannelSet::mono() && sido != juce::AudioChannelSet::stereo()) return false;
    }
    return true;
}

void RostverketProcessor::prepareToPlay (double samplingsfrekvens, int)
{
    fs = samplingsfrekvens;
    kanalerForberedda = juce::jlimit (1, 2, getMainBusNumOutputChannels());
    rampLangd = (int) (0.010 * fs);
    porten.forbered (fs);
    orgeln.forbered (fs);
    vokodern.forbered (fs);
    strupen.forbered (fs);
    sandaren.forbered (fs);
    koren.forbered (fs);
    ekot.forbered (fs);
    limitern.forbered (fs);
    latens = dsp::Strupen::latensFor (fs);
    for (auto* f : { &torrFordr, &vagFordr })
        for (auto& d : *f) { d.forbered (latens); d.sattLangd (latens); }
    torr.setSize (2, bit); analys.setSize (2, bit); barvag.setSize (2, bit); vok.setSize (2, bit);
    modul.setSize (2, bit); strupTorr.setSize (2, bit);
    mono.assign ((size_t) bit, 0.0f);
    inGain.reset (fs, 0.02);  inGain.setCurrentAndTargetValue (gainAv (las (id::inNiva)));
    utGain.reset (fs, 0.02);  utGain.setCurrentAndTargetValue (gainAv (las (id::utNiva)));
    torrVat.reset (fs, 0.02); torrVat.setCurrentAndTargetValue (las (id::torrVat));
    vokBlandning.reset (fs, 0.02); vokBlandning.setCurrentAndTargetValue (las (id::vokBlandning));
    forbiR.satt (pa (id::forbi) || ! aktiv() ? 1.0f : 0.0f);
    portR.satt (pa (id::portPa) ? 1.0f : 0.0f);
    vokR.satt (pa (id::vokPa) ? 1.0f : 0.0f);
    strupR.satt (pa (id::strupPa) ? 1.0f : 0.0f);
    sandR.satt (pa (id::sandPa) ? 1.0f : 0.0f);
    korR.satt (pa (id::korPa) ? 1.0f : 0.0f);
    ekoR.satt (pa (id::ekoPa) ? 1.0f : 0.0f);
    strupenNollad = ekotNollat = true;
    midiVarPa = false; kedjanStilla = true;
    inTopp = utTopp = 0.0f;
    setLatencySamples (latens);
}

bool RostverketProcessor::aktiv() const
{
    return pa (id::portPa) || pa (id::vokPa) || pa (id::strupPa) || pa (id::sandPa) || pa (id::korPa) || pa (id::ekoPa)
        || std::abs (las (id::inNiva)) >= 0.01f || std::abs (las (id::utNiva)) >= 0.01f;
}

// Omvandlingen (o = 0..1) drar varje modul mot maskin: mer vokoder och snabbare, hårdare följare; Strupen
// följer tonen hårdare; mer Sändare; en tätare kör. Modulernas egna nivåkompensationer håller nivån.
void RostverketProcessor::lasInstallning()
{
    const float o = las (id::omvandlingen);
    auto mot = [o] (float v, float mal) { return v + o * (mal - v); };
    porten.installning (las (id::portTroskel), las (id::portKompression), las (id::portHogpass), las (id::portNarhet), pa (id::portAvsibilant));
    vokodern.installning (lagen::bandAntal ((int) las (id::vokBand)), las (id::vokFormant), mot (las (id::vokSnabbhet), 1.0f),
                          mot (las (id::vokSkarpa), 0.9f), las (id::vokBegriplighet), pa (id::vokFrys));
    orgeln.installning ((int) las (id::orgVag), (int) las (id::orgOktav) - 2, las (id::orgGlid), las (id::orgBrus));
    strupen.installning ((int) las (id::strupLage), las (id::strupTonhojd), las (id::strupFormant), mot (las (id::strupFoljsamhet), 1.0f),
                         (int) las (id::strupTon));
    dsp::Sandaren::Param sp;
    sp.lage = (int) las (id::sandLage); sp.styrka = mot (las (id::sandStyrka), 1.0f); sp.frekvens = las (id::sandFrekvens); sp.ton = las (id::sandTon);
    sp.midiFrek = pa (id::sandMidi) && pa (id::midiIn) ? dsp::Orgeln::frekvens (orgeln.senasteTon()) : 0.0f;
    sandaren.installning (sp);
    koren.installning ((int) las (id::korLage), las (id::korSpridning) * (1.0f - 0.6f * o), las (id::korSvaj) * (1.0f - 0.5f * o),
                       las (id::korBredd), las (id::korBlandning));
    inGain.setTargetValue (gainAv (las (id::inNiva)));
    utGain.setTargetValue (gainAv (las (id::utNiva)));
    torrVat.setTargetValue (las (id::torrVat));
    vokBlandning.setTargetValue (mot (las (id::vokBlandning), 1.0f));
}

void RostverketProcessor::tona (krang::dsp::Ramp& r, float* const* ljud, float* const* efter, int kanaler, int n)
{
    for (int i = 0; i < n; ++i)
    {
        const float v = r.nasta();
        for (int k = 0; k < kanaler; ++k) ljud[k][i] += v * (efter[k][i] - ljud[k][i]);
    }
}

void RostverketProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals ingaDenormaler;
    lasInstallning();

    auto main = getBusBuffer (buffer, true, 0);
    const int kanaler = juce::jmin (kanalerForberedda, main.getNumChannels());
    const int n = main.getNumSamples();

    // Mätaren IN läses före allt annat.
    float inT = 0.0f;
    for (int k = 0; k < kanaler; ++k) inT = juce::jmax (inT, main.getMagnitude (k, 0, n));

    const float* sido[2] { nullptr, nullptr };
    int sidoKanaler = 0;
    if (getBusCount (true) > 1 && getBus (true, 1)->isEnabled())
    {
        auto s = getBusBuffer (buffer, true, 1);
        sidoKanaler = juce::jmin (2, s.getNumChannels());
        for (int k = 0; k < sidoKanaler; ++k) sido[k] = s.getReadPointer (k);
    }
    for (int k = getTotalNumInputChannels(); k < getTotalNumOutputChannels(); ++k)
        buffer.clear (k, 0, n);

    // MIDI: på = tonerna spelar Orgeln på sitt sample; av = Orgeln håller tonen på ratten TON.
    const bool midiPa = pa (id::midiIn);
    if (midiPa != midiVarPa) { orgeln.allaAv(); midiVarPa = midiPa; }
    if (! midiPa) orgeln.halltTon ((int) las (id::orgTon));

    // Tempot till Ekot.
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto position = ph->getPosition())
            if (auto b = position->getBpm()) bpm = *b;

    int pos = 0;
    auto korTill = [&] (int slut)
    {
        while (pos < slut)
        {
            const int len = juce::jmin (bit, slut - pos);
            float* ljud[2] { main.getWritePointer (0) + pos, kanaler > 1 ? main.getWritePointer (1) + pos : nullptr };
            const float* s[2] { sido[0] ? sido[0] + pos : nullptr, sido[1] ? sido[1] + pos : nullptr };
            processBit (ljud, kanaler, len, sidoKanaler > 0 ? s : nullptr, sidoKanaler, bpm);
            pos += len;
        }
    };
    if (midiPa)
    {
        for (const auto m : midi)
        {
            korTill (juce::jlimit (0, n, m.samplePosition));
            const auto msg = m.getMessage();
            if (msg.isNoteOn())                         orgeln.tonPa (msg.getNoteNumber());
            else if (msg.isNoteOff())                   orgeln.tonAv (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff()) orgeln.allaAv();
        }
    }
    korTill (n);

    // Appens inspelare (gör ingenting när den inte spelar in).
    if (inspelaren.spelarIn())
    {
        const float* ut[2] { main.getReadPointer (0), main.getReadPointer (kanaler > 1 ? 1 : 0) };
        inspelaren.skriv (ut, n, midi);
    }

    // Mätarna: toppvärde med fall på ungefär 20 dB/s, utan lås.
    float utT = 0.0f;
    for (int k = 0; k < kanaler; ++k) utT = juce::jmax (utT, main.getMagnitude (k, 0, n));
    const float fall = std::pow (10.0f, -1.0f * (float) n / (float) fs);
    inTopp = juce::jmax (inT, inTopp * fall);
    utTopp = juce::jmax (utT, utTopp * fall);
    inNivaDb.store (krang::dsp::gainTillDb (inTopp));
    utNivaDb.store (krang::dsp::gainTillDb (utTopp));
    limiterDb.store (limitern.hamtaDampning());
    bandAktiva.store (vokodern.antalBand());
    orgelTon.store (midiPa ? orgeln.senasteTon() : (int) las (id::orgTon));
    for (int b = 0; b < dsp::Vokodern::maxBand; ++b)
    {
        const float e = pa (id::vokPa) && ! kedjanStilla ? vokodern.bandNiva (b) : 0.0f;
        bandNiva[(size_t) b].store (juce::jlimit (0.0f, 1.0f, (krang::dsp::gainTillDb (e * 4.0f) + 54.0f) / 54.0f));
    }
}

void RostverketProcessor::processBit (float* const* ljud, int kanaler, int n, const float* const* sido, int sidoKanaler, double bpm)
{
    // Den torra vägen: ingången fördröjd med latensen.
    for (int k = 0; k < kanaler; ++k)
    {
        auto* t = torr.getWritePointer (k);
        for (int i = 0; i < n; ++i) t[i] = torrFordr[(size_t) k].process (ljud[k][i]);
    }

    // Förbikopplad (PÅ LUFTEN släckt, eller inget att göra): det fördröjda ljudet orört.
    forbiR.mot (pa (id::forbi) || ! aktiv() ? 1.0f : 0.0f, rampLangd);
    if (vid (forbiR, 1.0f))
    {
        if (! kedjanStilla)
        {
            porten.nollstall(); vokodern.nollstall(); sandaren.nollstall(); koren.nollstall(); limitern.reset();
            for (auto& d : vagFordr) d.reset();
            if (! strupenNollad) { strupen.nollstall(); strupenNollad = true; }
            if (! ekotNollat) { ekot.nollstall(); ekotNollat = true; }
            kedjanStilla = true;
        }
        for (int k = 0; k < kanaler; ++k) juce::FloatVectorOperations::copy (ljud[k], torr.getReadPointer (k), n);
        // Övertoningarna i nivåerna följer med, så att de inte hoppar senare.
        inGain.skip (n); utGain.skip (n); torrVat.skip (n); vokBlandning.skip (n);
        return;
    }
    kedjanStilla = false;
    float* md[2] { modul.getWritePointer (0), modul.getWritePointer (1) };
    auto tillModul = [&] { for (int k = 0; k < kanaler; ++k) modul.copyFrom (k, 0, ljud[k], n); };

    // In-nivå
    for (int i = 0; i < n; ++i)
    {
        const float g = inGain.getNextValue();
        for (int k = 0; k < kanaler; ++k) ljud[k][i] *= g;
    }

    // 1 Porten (avstängd = analysen får ljudet som det är)
    portR.mot (pa (id::portPa) ? 1.0f : 0.0f, rampLangd);
    float* an[2] { analys.getWritePointer (0), analys.getWritePointer (1) };
    for (int k = 0; k < kanaler; ++k) analys.copyFrom (k, 0, ljud[k], n);
    if (vid (portR, 0.0f)) porten.nollstall();
    else
    {
        tillModul();
        porten.process (md, an, kanaler, n);
        for (int i = 0; i < n; ++i)
        {
            const float r = portR.nasta();
            for (int k = 0; k < kanaler; ++k)
            {
                const float orort = ljud[k][i];
                ljud[k][i] = orort + r * (md[k][i] - orort);
                an[k][i] = orort + r * (an[k][i] - orort);
            }
        }
    }

    // 2 Vokodern
    vokR.mot (pa (id::vokPa) ? 1.0f : 0.0f, rampLangd);
    if (vid (vokR, 0.0f)) { vokodern.nollstall(); vokBlandning.skip (n); }
    else
    {
        for (int i = 0; i < n; ++i)
        {
            float s = 0.0f;
            for (int k = 0; k < kanaler; ++k) s += an[k][i];
            mono[(size_t) i] = s / (float) kanaler;
        }
        const int barvagLage = (int) las (id::vokBarvag);
        float* bv[2] { barvag.getWritePointer (0), barvag.getWritePointer (1) };
        for (int i = 0; i < n; ++i)
        {
            switch (barvagLage)
            {
                case 0: { const float o = orgeln.nasta(); for (int k = 0; k < kanaler; ++k) bv[k][i] = o; break; }
                case 1:
                    for (int k = 0; k < kanaler; ++k)
                        bv[k][i] = sido != nullptr && sidoKanaler > 0 ? sido[juce::jmin (k, sidoKanaler - 1)][i] : 0.0f;
                    break;
                default: for (int k = 0; k < kanaler; ++k) bv[k][i] = 0.5f * brus[(size_t) k].vitt(); break;
            }
        }
        float* vu[2] { vok.getWritePointer (0), vok.getWritePointer (1) };
        vokodern.process (mono.data(), bv, vu, kanaler, n);
        for (int i = 0; i < n; ++i)
        {
            const float b = vokR.nasta() * vokBlandning.getNextValue();
            for (int k = 0; k < kanaler; ++k) ljud[k][i] += b * (vu[k][i] - ljud[k][i]);
        }
    }

    // 3 Strupen – latensen. Avstängd är den en ren fördröjning (vagFordr), så att latensen aldrig ändras.
    strupR.mot (pa (id::strupPa) ? 1.0f : 0.0f, rampLangd);
    if (vid (strupR, 0.0f))
    {
        if (! strupenNollad) { strupen.nollstall(); strupenNollad = true; }
        for (int k = 0; k < kanaler; ++k)
            for (int i = 0; i < n; ++i) ljud[k][i] = vagFordr[(size_t) k].process (ljud[k][i]);
    }
    else
    {
        strupenNollad = false;
        tillModul();
        float* st[2] { strupTorr.getWritePointer (0), strupTorr.getWritePointer (1) };
        strupen.process (md, st, kanaler, n);
        const float bl = las (id::strupBlandning);
        for (int i = 0; i < n; ++i)
        {
            const float r = strupR.nasta();
            for (int k = 0; k < kanaler; ++k)
            {
                const float d = vagFordr[(size_t) k].process (ljud[k][i]);
                const float v = d + bl * (md[k][i] - d);
                ljud[k][i] = d + r * (v - d);
            }
        }
    }

    // 4 Sändaren
    sandR.mot (pa (id::sandPa) ? 1.0f : 0.0f, rampLangd);
    if (vid (sandR, 0.0f)) sandaren.nollstall();
    else
    {
        tillModul();
        sandaren.process (md, kanaler, n);
        const float bl = juce::jlimit (0.0f, 1.0f, las (id::sandBlandning) + 0.5f * las (id::omvandlingen));
        for (int k = 0; k < kanaler; ++k)
            for (int i = 0; i < n; ++i) md[k][i] = ljud[k][i] + bl * (md[k][i] - ljud[k][i]);
        tona (sandR, ljud, md, kanaler, n);
    }

    // 5 Kören
    korR.mot (pa (id::korPa) ? 1.0f : 0.0f, rampLangd);
    if (vid (korR, 0.0f)) koren.nollstall();
    else { tillModul(); koren.process (md, kanaler, n); tona (korR, ljud, md, kanaler, n); }

    // 6 Ekot
    ekoR.mot (pa (id::ekoPa) ? 1.0f : 0.0f, rampLangd);
    if (vid (ekoR, 0.0f)) { if (! ekotNollat) { ekot.nollstall(); ekotNollat = true; } }
    else
    {
        ekotNollat = false;
        dsp::Ekot::Param ep;
        ep.fjardedelar = lagen::tidFjardedelar ((int) las (id::ekoTid)); ep.bpm = bpm;
        ep.aterkoppling = las (id::ekoAterkoppling) / 100.0f; ep.morker = las (id::ekoMorker); ep.rum = las (id::ekoRum);
        ep.blandning = las (id::ekoBlandning); ep.pingpong = pa (id::ekoPingpong);
        tillModul();
        ekot.process (md, kanaler, n, ep);
        tona (ekoR, ljud, md, kanaler, n);
    }

    // Torr/våt och utnivå
    for (int i = 0; i < n; ++i)
    {
        const float tv = torrVat.getNextValue(), g = utGain.getNextValue();
        for (int k = 0; k < kanaler; ++k)
        {
            const float t = torr.getSample (k, i);
            ljud[k][i] = (t + tv * (ljud[k][i] - t)) * g;
        }
    }
    if (pa (id::limiter)) limitern.process (ljud, kanaler, n);

    // Förbikopplingens övertoning
    if (! vid (forbiR, 0.0f))
        for (int i = 0; i < n; ++i)
        {
            const float f = forbiR.nasta();
            for (int k = 0; k < kanaler; ++k) ljud[k][i] += f * (torr.getSample (k, i) - ljud[k][i]);
        }
}

void RostverketProcessor::getStateInformation (juce::MemoryBlock& data)
{
    auto tillstand = apvts.copyState();
    tillstand.setProperty ("rvFormat", formatVersion, nullptr);
    if (auto xml = tillstand.createXml()) copyXmlToBinary (*xml, data);
}

void RostverketProcessor::setStateInformation (const void* data, int storlek)
{
    if (auto xml = getXmlFromBinary (data, storlek))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            programbank.aterstall();
        }
}

juce::AudioProcessorEditor* RostverketProcessor::createEditor() { return new RostverketEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RostverketProcessor(); }
