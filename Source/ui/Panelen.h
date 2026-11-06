// Panelen: sändarbordet ritat från processorns parametrar och mätare, och kopplat till dem.
//
// Rattar: dra upp/ned (Skift = fint), rulla, dubbelklick = grundvärdet. Tonskalan: klick = Orgelns ton. Vridomkopplare: klick = nästa,
// Skift-klick = förra, högerklick = lista, rulla. Vippor, PÅ LUFTEN, LIMITER-lampan och MIDI: klick.
// Programmen: ◀ ▶ bläddrar, klick på programfönstret ger listan per kategori, SPARA frågar efter ett namn,
// en stjärna visar att programmet är ändrat. MENY: storleken, mappen med egna program och – i appen –
// mikrofonen, ljudkortet och inspelaren.
//
// Alla sex moduler är inkopplade (fas 2). Bakgrunden cachas; reglagen ritas om bara
// när ett värde eller en mätare ändras (högst 30 bilder/s).
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <krang/ui/ValueFormat.h>
#include <functional>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../PluginProcessor.h"
#include "stil/Panelbild.h"

namespace rv::ui
{
using stil::panel::Del;
namespace pb = stil::panel;

class Panelen : public juce::Component, private juce::Timer
{
public:
    explicit Panelen (RostverketProcessor& p) : proc (p)
    {
        setOpaque (true);
        tillstand = bygg();
        startTimerHz (30);
    }
    ~Panelen() override { avslutaGest(); }

    std::function<void (float)> onStorlek;   // MENY: ny storlek (0,6–1,6)
    float storlek = 0.85f;

    void paint (juce::Graphics& g) override { cache.rita (g, tillstand); }

    // Bygger allt som syns från parametrarna och mätarna.
    pb::Tillstand bygg() const
    {
        pb::Tillstand t;
        auto& m = t.moduler;
        m[0] = { 1, "Porten", pa (id::portPa), false, { { "", {
            ratt (id::portTroskel, svt ("Tröskel")), ratt (id::portKompression, "Kompr."), ratt (id::portHogpass, svt ("Högpass")),
            ratt (id::portNarhet, svt ("Närhet")), vippa (id::portAvsibilant, "Avsib.") } } }, id::portPa };
        m[1] = { 2, "Vokodern", pa (id::vokPa), false, {
            { "", { valjare (id::vokBand, "Band"), valjare (id::vokBarvag, svt ("Bärvåg")), ratt (id::vokFormant, "Formant"),
                    ratt (id::vokSnabbhet, "Snabbhet"), ratt (id::vokSkarpa, svt ("Skärpa")), ratt (id::vokBegriplighet, "Begripl.") } },
            { "Orgeln", { valjare (id::orgVag, svt ("Våg")), valjare (id::orgOktav, "Oktav"), ratt (id::orgGlid, "Glid"), ratt (id::orgBrus, "Brus"),
                          vippa (id::vokFrys, "Frys"), ratt (id::vokBlandning, "Blandning") } } }, id::vokPa };
        m[2] = { 3, "Strupen", pa (id::strupPa), false, {
            { "", { valjare (id::strupLage, svt ("Läge")), ratt (id::strupTonhojd, svt ("Tonhöjd")), ratt (id::strupFormant, "Formant") } },
            { "", { ratt (id::strupFoljsamhet, svt ("Följsamhet")), ratt (id::strupBlandning, "Blandning"), ratt (id::strupTon, "Ton") } } }, id::strupPa };
        m[3] = { 4, svt ("Sändaren"), pa (id::sandPa), false, {
            { "", { valjare (id::sandLage, svt ("Läge")), ratt (id::sandStyrka, "Styrka"), ratt (id::sandFrekvens, "Frekvens") } },
            { "", { ratt (id::sandTon, "Ton"), ratt (id::sandBlandning, "Blandning"), vippa (id::sandMidi, "MIDI") } } }, id::sandPa };
        m[4] = { 5, svt ("Kören"), pa (id::korPa), false, { { "", {
            valjare (id::korLage, svt ("Läge")), ratt (id::korSpridning, "Spridning"), ratt (id::korSvaj, "Svaj"),
            ratt (id::korBredd, "Bredd"), ratt (id::korBlandning, "Blandning") } } }, id::korPa };
        m[5] = { 6, "Ekot", pa (id::ekoPa), false, {
            { "", { valjare (id::ekoTid, "Tid"), ratt (id::ekoAterkoppling, svt ("Återkoppl.")), ratt (id::ekoMorker, svt ("Mörker")) } },
            { "", { ratt (id::ekoRum, "Rum"), ratt (id::ekoBlandning, "Blandning"), vippa (id::ekoPingpong, "Ping-pong") } } }, id::ekoPa };

        const auto omv = ratt (id::omvandlingen, "");
        t.omvandlingen = omv.varde; t.omvandlingenText = omv.text;
        t.inDb = proc.inNivaDb.load(); t.utDb = proc.utNivaDb.load();
        t.paLuften = ! pa (id::forbi);
        t.bandAktiva = proc.bandAktiva.load();
        for (size_t b = 0; b < t.band.size(); ++b) t.band[b] = proc.bandNiva[b].load();
        t.fryst = pa (id::vokFrys) && pa (id::vokPa);
        t.ton = proc.orgelTon.load();
        t.tonRubrik = "ORGELN " + tonnamn (t.ton);
        {
            const auto& bank = proc.programbank;
            const auto& p = bank.program (bank.aktuellt());
            t.programNr = juce::String (bank.aktuellt() + 1).paddedLeft ('0', 2).getLastCharacters (2);
            t.programNamn = p.namn + (andratFran (p) ? " *" : "");
            t.kategori = p.kategori;
            if (iAppen())
            {
                t.kategori << svt ("  ·  MIKROFON ") << (mikrofonPa() ? svt ("PÅ") : "AV");
                if (proc.inspelaren.spelarIn())
                {
                    const int sek = (int) proc.inspelaren.sekunder();
                    t.kategori << svt ("  ·  ● INSPELNING ") << juce::String (sek / 60) << ":" << juce::String (sek % 60).paddedLeft ('0', 2);
                }
            }
        }
        t.utgang = { ratt (id::inNiva, svt ("In-nivå")), ratt (id::torrVat, svt ("Torr/våt")), ratt (id::utNiva, svt ("Utnivå")) };
        t.limiterPa = pa (id::limiter);
        t.limiterLampa = juce::jlimit (0.0f, 1.0f, -proc.limiterDb.load() / 6.0f);
        t.midiIn = pa (id::midiIn);
        return t;
    }

    // ------------------------------------------------------------------------------------------
    // Musen

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto p = e.position;
        const auto mal = traff (p);
        if (mal.pid.isEmpty())
        {
            if (geo().knappar[0].contains (p)) { proc.programbank.stega (-1); uppdatera(); }
            else if (geo().knappar[1].contains (p)) { proc.programbank.stega (1); uppdatera(); }
            else if (geo().knappar[2].contains (p)) sparaDialog();
            else if (geo().knappar[3].contains (p)) visaMeny();
            else if (pb::geo::program.contains (p)) programLista();
            return;
        }
        auto* par = proc.apvts.getParameter (mal.pid);
        if (par == nullptr) return;
        // Tonskalan: klick sätter Orgelns ton där man klickar, och dragning fortsätter som en ratt.
        if (mal.pid == id::orgTon && pb::geo::tonskala.contains (p))
        {
            const auto skalan = pb::geo::tonskala.reduced (14.0f, 0.0f);
            par->beginChangeGesture();
            par->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, (p.x - skalan.getX()) / skalan.getWidth()));
            dras = par; startVarde = par->getValue(); startY = p.y;
            uppdatera();
            return;
        }
        switch (mal.typ)
        {
            case Del::Typ::ratt:
                dras = par; startVarde = par->getValue(); startY = p.y;
                par->beginChangeGesture();
                break;
            case Del::Typ::valjare:
            {
                const int antal = juce::jmax (2, par->getNumSteps());
                const int nu = juce::roundToInt (par->getValue() * (float) (antal - 1));
                if (e.mods.isPopupMenu())
                {
                    juce::PopupMenu meny;
                    const auto lista = dynamic_cast<juce::AudioParameterChoice*> (par)->choices;
                    for (int i = 0; i < lista.size(); ++i) meny.addItem (i + 1, lista[i], true, i == nu);
                    meny.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (juce::Rectangle<int> (e.getScreenX(), e.getScreenY(), 1, 1)),
                                        [par, antal] (int r) { if (r > 0) satt (par, (float) (r - 1) / (float) (antal - 1)); });
                }
                else
                {
                    const int ny = (nu + (e.mods.isShiftDown() ? antal - 1 : 1)) % antal;
                    satt (par, (float) ny / (float) (antal - 1));
                }
                break;
            }
            case Del::Typ::vippa:
                satt (par, par->getValue() > 0.5f ? 0.0f : 1.0f);
                break;
        }
        uppdatera();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dras == nullptr) return;
        const float kanslighet = e.mods.isShiftDown() ? 1000.0f : 200.0f;
        dras->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, startVarde + (startY - e.position.y) / kanslighet));
        uppdatera();
    }

    void mouseUp (const juce::MouseEvent&) override { avslutaGest(); }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        const auto mal = traff (e.position);
        if (mal.typ != Del::Typ::ratt || mal.pid.isEmpty()) return;
        if (auto* par = proc.apvts.getParameter (mal.pid)) satt (par, par->getDefaultValue());
        uppdatera();
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        const auto mal = traff (e.position);
        auto* par = mal.pid.isEmpty() ? nullptr : proc.apvts.getParameter (mal.pid);
        if (par == nullptr) return;
        const float d = std::abs (w.deltaY) > 0.0f ? w.deltaY : w.deltaX;
        if (mal.typ == Del::Typ::ratt) satt (par, juce::jlimit (0.0f, 1.0f, par->getValue() + d * 0.1f));
        else if (mal.typ == Del::Typ::valjare)
        {
            const int antal = juce::jmax (2, par->getNumSteps());
            const int nu = juce::roundToInt (par->getValue() * (float) (antal - 1));
            satt (par, (float) juce::jlimit (0, antal - 1, nu + (d > 0.0f ? 1 : -1)) / (float) (antal - 1));
        }
        uppdatera();
    }

    // Klicktestet: var reglaget för en parameter sitter (tom punkt om det inte syns).
    struct Mal { juce::String pid; Del::Typ typ = Del::Typ::ratt; juce::Point<float> c; float r = 0.0f; };
    std::vector<Mal> allaMal() const
    {
        std::vector<Mal> ut;
        pb::forVarjeDel (tillstand, [&] (int, const Del& d, juce::Rectangle<float> cell, float r)
        {
            if (d.pid.isNotEmpty()) ut.push_back ({ d.pid, d.typ, pb::centrum (d, cell), pb::radie (d, r) });
        });
        for (int i = 0; i < 6; ++i) ut.push_back ({ tillstand.moduler[(size_t) i].paPid, Del::Typ::vippa, pb::modulVippa (i), 9.0f });
        ut.push_back ({ id::omvandlingen, Del::Typ::ratt, geo().spaken, 22.0f });
        const char* utg[] { id::inNiva, id::torrVat, id::utNiva };
        for (int i = 0; i < 3; ++i) ut.push_back ({ utg[i], Del::Typ::ratt, geo().utgang[(size_t) i], 13.0f });
        ut.push_back ({ id::orgTon, Del::Typ::ratt, pb::geo::tonskala.getCentre(), 40.0f });
        ut.push_back ({ id::forbi, Del::Typ::vippa, geo().paLuften.getCentre(), 30.0f });
        ut.push_back ({ id::limiter, Del::Typ::vippa, geo().limiter, 9.0f });
        ut.push_back ({ id::midiIn, Del::Typ::vippa, geo().midi, 9.0f });
        return ut;
    }

private:
    struct Geo
    {
        juce::Point<float> spaken = pb::geo::spaken, limiter = pb::geo::limiter, midi = pb::geo::midi;
        juce::Rectangle<float> paLuften = pb::geo::paLuften;
        std::array<juce::Point<float>, 3> utgang = pb::geo::utgang;
        std::array<juce::Rectangle<float>, 4> knappar = pb::geo::knappar;
    };
    static const Geo& geo() { static const Geo g; return g; }

    Mal traff (juce::Point<float> p) const
    {
        if (geo().paLuften.contains (p)) return { id::forbi, Del::Typ::vippa, {}, 0.0f };
        if (pb::geo::tonskala.contains (p)) return { id::orgTon, Del::Typ::ratt, {}, 0.0f };
        Mal basta; float bast = 1.0e9f;
        for (const auto& m : allaMal())
        {
            if (m.pid == id::forbi) continue;
            const float d = m.c.getDistanceFrom (p);
            const float grans = m.typ == Del::Typ::vippa ? juce::jmax (12.0f, m.r * 1.4f) : m.r * 1.25f;
            if (d < grans && d < bast) { bast = d; basta = m; }
        }
        return basta;
    }

    // I appen sätter krang::app::FristaendeApp ljudkortet, och App.cpp mikrofonens funktioner.
    bool iAppen() const { return proc.enheter != nullptr; }
    static bool mikrofonPa() { return RostverketProcessor::app.mikrofonPa ? RostverketProcessor::app.mikrofonPa() : false; }

    void visaMeny()
    {
        juce::PopupMenu meny, storlekar;
        const int val[] { 60, 70, 85, 100, 120, 140, 160 };
        for (int i = 0; i < 7; ++i)
            storlekar.addItem (100 + i, juce::String (val[i]) + " %", true, std::abs (storlek * 100.0f - (float) val[i]) < 1.0f);
        meny.addSectionHeader (svt ("Röstverket RV-83"));
        meny.addSubMenu ("Storlek", storlekar);
        meny.addItem (2, svt ("Visa mappen med egna program"));
        if (iAppen())
        {
            meny.addSeparator();
            meny.addItem (10, svt ("Mikrofonen på (använd hörlurar)"), true, mikrofonPa());
            meny.addItem (11, svt ("Ljud och MIDI …"));
            meny.addItem (12, proc.inspelaren.spelarIn() ? svt ("Stoppa inspelningen") : svt ("Spela in utgången"));
            meny.addItem (13, svt ("Visa inspelningarna"));
        }
        meny.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this, val] (int r)
        {
            if (r >= 100 && r < 107 && onStorlek) onStorlek ((float) val[r - 100] / 100.0f);
            if (r == 2) { proc.programbank.egenMapp().createDirectory(); proc.programbank.egenMapp().revealToUser(); }
            if (! iAppen()) return;
            if (r == 10 && RostverketProcessor::app.sattMikrofon) RostverketProcessor::app.sattMikrofon (! mikrofonPa());
            if (r == 11) visaEnheter (*proc.enheter);
            if (r == 12)
            {
                if (proc.inspelaren.spelarIn()) proc.inspelaren.stoppa();
                else if (! proc.inspelaren.starta (proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 48000.0))
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, svt ("Röstverket"),
                                                            svt ("Kunde inte börja spela in i ") + proc.inspelaren.mapp().getFullPathName());
            }
            if (r == 13) { proc.inspelaren.mapp().createDirectory(); proc.inspelaren.mapp().revealToUser(); }
            uppdatera();
        });
    }

    // Ljud och MIDI: JUCE:s val av ljudkort, in- och utgångar och MIDI-ingångar.
    void visaEnheter (juce::AudioDeviceManager& enh)
    {
        auto* val = new juce::AudioDeviceSelectorComponent (enh, 0, 2, 0, 2, true, false, true, false);
        val->setSize (520, 420);
        juce::DialogWindow::LaunchOptions o;
        o.content.setOwned (val);
        o.dialogTitle = svt ("Ljud och MIDI");
        o.dialogBackgroundColour = stil::farg::kitt;
        o.escapeKeyTriggersCloseButton = true;
        o.useNativeTitleBar = true;
        o.resizable = false;
        o.launchAsync();
    }

    // Programlistan, en undermeny per kategori.
    void programLista()
    {
        auto& bank = proc.programbank;
        bank.lasEgna();
        juce::PopupMenu meny;
        for (const auto& kat : bank.kategorier())
        {
            juce::PopupMenu under;
            for (int i = 0; i < bank.antal(); ++i)
                if (bank.program (i).kategori == kat)
                    under.addItem (i + 1, juce::String (i + 1).paddedLeft ('0', 2) + "  " + bank.program (i).namn, true, i == bank.aktuellt());
            meny.addSubMenu (kat, under);
        }
        meny.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int val)
        {
            if (val > 0) { proc.programbank.ladda (val - 1); uppdatera(); }
        });
    }

    // SPARA: namnet frågas efter; programmet hamnar under "Egna". Ett namn som ett fabriksprogram får
    // tillägget "(eget)".
    void sparaDialog()
    {
        auto* ruta = new juce::AlertWindow (svt ("Spara program"), svt ("Namn på programmet (sparas under Egna):"), juce::MessageBoxIconType::NoIcon, this);
        const auto namn = proc.programbank.aktuelltNamn();
        ruta->addTextEditor ("namn", proc.programbank.program (proc.programbank.aktuellt()).fabrik() ? namn + " 2" : namn);
        ruta->addButton ("Spara", 1, juce::KeyPress (juce::KeyPress::returnKey));
        ruta->addButton ("Avbryt", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        ruta->enterModalState (true, juce::ModalCallbackFunction::create ([this, ruta] (int r)
        {
            if (r != 1) return;
            auto n = ruta->getTextEditorContents ("namn").trim();
            for (int i = 0; i < proc.programbank.antalFabrik(); ++i)
                if (proc.programbank.program (i).namn.equalsIgnoreCase (n)) { n << " (eget)"; break; }
            if (! proc.programbank.spara (n))
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, svt ("Röstverket"),
                                                        svt ("Kunde inte spara programmet i ") + proc.programbank.egenMapp().getFullPathName());
            uppdatera();
        }), true);
    }

    // Har någon parameter ändrats från programmet? (Undantagen – förbikoppling och MIDI – räknas inte.)
    bool andratFran (const krang::presets::Program& p) const
    {
        for (auto* par : proc.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (par))
            {
                const auto pid = r->getParameterID();
                if (pid == id::forbi || pid == id::midiIn) continue;
                float norm = r->getDefaultValue();
                for (auto& [vid, v] : p.varden) if (vid == pid) { norm = r->convertTo0to1 (v); break; }
                if (std::abs (r->getValue() - norm) > 1.0e-3f) return true;
            }
        return false;
    }

    static void satt (juce::RangedAudioParameter* par, float v)
    {
        par->beginChangeGesture();
        par->setValueNotifyingHost (v);
        par->endChangeGesture();
    }
    void avslutaGest() { if (dras != nullptr) { dras->endChangeGesture(); dras = nullptr; } }

    void uppdatera() { tillstand = bygg(); repaint(); }

    void timerCallback() override
    {
        auto ny = bygg();
        if (! lika (ny, tillstand)) { tillstand = std::move (ny); repaint(); }
    }

    // Jämför det som kan ändras utan att modulernas namn ändras (då behöver bakgrunden inte byggas om).
    static bool lika (const pb::Tillstand& a, const pb::Tillstand& b)
    {
        auto nara = [] (float x, float y, float tol) { return std::abs (x - y) < tol; };
        if (! nara (a.inDb, b.inDb, 0.2f) || ! nara (a.utDb, b.utDb, 0.2f) || a.paLuften != b.paLuften || a.fryst != b.fryst
            || a.ton != b.ton || a.bandAktiva != b.bandAktiva || a.limiterPa != b.limiterPa || a.midiIn != b.midiIn
            || ! nara (a.omvandlingen, b.omvandlingen, 0.002f) || ! nara (a.limiterLampa, b.limiterLampa, 0.05f)
            || a.programNr != b.programNr || a.programNamn != b.programNamn || a.kategori != b.kategori)
            return false;
        for (size_t i = 0; i < a.band.size(); ++i) if (! nara (a.band[i], b.band[i], 0.01f)) return false;
        for (size_t i = 0; i < a.moduler.size(); ++i)
        {
            if (a.moduler[i].pa != b.moduler[i].pa) return false;
            for (size_t r = 0; r < a.moduler[i].rader.size(); ++r)
                for (size_t k = 0; k < a.moduler[i].rader[r].delar.size(); ++k)
                {
                    const auto& x = a.moduler[i].rader[r].delar[k]; const auto& y = b.moduler[i].rader[r].delar[k];
                    if (x.text != y.text || x.lage != y.lage || x.pa != y.pa || ! nara (x.varde, y.varde, 0.002f)) return false;
                }
        }
        for (size_t i = 0; i < 3; ++i) if (a.utgang[i].text != b.utgang[i].text || ! nara (a.utgang[i].varde, b.utgang[i].varde, 0.002f)) return false;
        return true;
    }

    juce::RangedAudioParameter& par (const char* pid) const { return *proc.apvts.getParameter (pid); }
    float las (const char* pid) const { return proc.apvts.getRawParameterValue (pid)->load(); }
    bool pa (const char* pid) const { return las (pid) > 0.5f; }

    Del ratt (const char* pid, const juce::String& namn) const
    {
        auto& p = par (pid);
        const float v = las (pid);
        auto d = Del::ratt (namn, p.convertTo0to1 (v), krang::ui::formatValue (p, v)); d.pid = pid; return d;
    }
    Del valjare (const char* pid, const juce::String& namn) const
    {
        auto* c = dynamic_cast<juce::AudioParameterChoice*> (&par (pid));
        auto d = Del::valjare (namn, c != nullptr ? c->choices : juce::StringArray(), c != nullptr ? c->getIndex() : 0); d.pid = pid; return d;
    }
    Del vippa (const char* pid, const juce::String& namn) const { auto d = Del::vippa (namn, pa (pid)); d.pid = pid; return d; }

    RostverketProcessor& proc;
    pb::Tillstand tillstand;
    pb::Cache cache;
    juce::RangedAudioParameter* dras = nullptr;
    float startVarde = 0.0f, startY = 0.0f;
};

} // namespace rv::ui
