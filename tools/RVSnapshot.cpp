// RVSnapshot – ritar Röstverkets panel och stilprov till PNG, så att gränssnittet kan granskas utan DAW.
//
//   RVSnapshot fil.png [skala] [id=värde ...]         den riktiga panelen med givna värden
//   RVSnapshot --stilprov fil.png [skala]             sändarbordets stilprov (fasta exempelvärden)
//   RVSnapshot --stilprov-detaljer fil.png [skala]    närbilder av reglagen och instrumenten
//   RVSnapshot --manual mapp                          manualens bilder: panelen och utsnitt i 200 %
//   RVSnapshot --tid                                  ritningstider för bakgrunden och reglagen
//   RVSnapshot --klicktest                            klickar, drar och rullar på varje reglage och
//                                                     kontrollerar att rätt parameter ändrades (OK/FEL)
//
// Skalan är fysisk (2 = Retina). Stilproven använder varken processorn eller parametrarna.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginEditor.h"
#include "ui/stil/Stilprov.h"
#include <iostream>

static int sparaPNG (juce::Component& c, const juce::File& fil, float skala, const char* vad)
{
    const auto bild = c.createComponentSnapshot (c.getLocalBounds(), true, skala);
    fil.deleteFile();
    fil.getParentDirectory().createDirectory();
    juce::FileOutputStream ut (fil);
    juce::PNGImageFormat png;
    if (! ut.openedOk() || ! png.writeImageToStream (bild, ut))
    {
        std::cout << "Kunde inte skriva " << fil.getFullPathName() << "\n";
        return 1;
    }
    std::cout << vad << " ritat till " << fil.getFullPathName() << "\n";
    return 0;
}

// Klicktestet: en mushändelse på panelen i designkoordinater.
static juce::MouseEvent hand (juce::Component& c, juce::Point<float> p, juce::ModifierKeys mods, int klick = 1, juce::Point<float> ner = {})
{
    auto kalla = juce::Desktop::getInstance().getMainMouseSource();
    const auto nu = juce::Time::getCurrentTime();
    return juce::MouseEvent (kalla, p, mods, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, &c, &c, nu, ner.isOrigin() ? p : ner, nu, klick, false);
}

static int klicktest()
{
    RostverketProcessor p;
    p.setRateAndBufferSizeDetails (48000.0, 512);
    p.prepareToPlay (48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    auto& panel = dynamic_cast<RostverketEditor&> (*ed).panel;
    panel.setTransform ({});
    int fel = 0, antal = 0;
    auto kontroll = [&] (bool ok, const juce::String& vad) { ++antal; if (! ok) { ++fel; std::cout << "  FEL   " << vad.toRawUTF8() << "\n"; } };
    const auto vanster = juce::ModifierKeys (juce::ModifierKeys::leftButtonModifier);
    juce::StringArray provade;
    for (const auto& m : panel.allaMal())
    {
        auto* par = p.apvts.getParameter (m.pid);
        if (par == nullptr) { kontroll (false, "okänd parameter " + m.pid); continue; }
        provade.addIfNotAlreadyThere (m.pid);
        const float fore = par->getValue();
        switch (m.typ)
        {
            case rv::stil::panel::Del::Typ::ratt:
            {
                par->setValueNotifyingHost (0.3f);
                panel.mouseDown (hand (panel, m.c, vanster));
                panel.mouseDrag (hand (panel, m.c.translated (0.0f, -40.0f), vanster, 1, m.c));
                panel.mouseUp (hand (panel, m.c.translated (0.0f, -40.0f), {}, 1, m.c));
                kontroll (par->getValue() > 0.45f, m.pid + juce::String::fromUTF8 (": dra uppåt höjer"));
                panel.mouseDoubleClick (hand (panel, m.c, vanster, 2));
                kontroll (std::abs (par->getValue() - par->getDefaultValue()) < 1.0e-4f, m.pid + juce::String::fromUTF8 (": dubbelklick ger grundvärdet"));
                juce::MouseWheelDetails w; w.deltaX = 0.0f; w.deltaY = 0.2f; w.isReversed = false; w.isSmooth = false; w.isInertial = false;
                par->setValueNotifyingHost (0.5f);
                panel.mouseWheelMove (hand (panel, m.c, {}), w);
                kontroll (par->getValue() > 0.5f, m.pid + ": rullning");
                break;
            }
            case rv::stil::panel::Del::Typ::valjare:
            {
                panel.mouseDown (hand (panel, m.c, vanster));
                panel.mouseUp (hand (panel, m.c, {}));
                kontroll (std::abs (par->getValue() - fore) > 1.0e-4f, m.pid + juce::String::fromUTF8 (": klick byter läge"));
                const float efter = par->getValue();
                panel.mouseDown (hand (panel, m.c, vanster.withFlags (juce::ModifierKeys::shiftModifier)));
                panel.mouseUp (hand (panel, m.c, {}));
                kontroll (std::abs (par->getValue() - fore) < 1.0e-4f && std::abs (efter - fore) > 1.0e-4f, m.pid + juce::String::fromUTF8 (": Skift-klick går tillbaka"));
                break;
            }
            case rv::stil::panel::Del::Typ::vippa:
            {
                panel.mouseDown (hand (panel, m.c, vanster));
                panel.mouseUp (hand (panel, m.c, {}));
                kontroll ((par->getValue() > 0.5f) != (fore > 0.5f), m.pid + ": klick växlar");
                par->setValueNotifyingHost (fore);
                break;
            }
        }
        par->setValueNotifyingHost (par->getDefaultValue());
    }
    // Programknapparna ◀ ▶ bläddrar i fabriksbandet.
    {
        p.programbank.ladda (3);
        const auto& kn = rv::stil::panel::geo::knappar;
        panel.mouseDown (hand (panel, kn[1].getCentre(), vanster)); panel.mouseUp (hand (panel, kn[1].getCentre(), {}));
        kontroll (p.programbank.aktuellt() == 4, juce::String::fromUTF8 ("▶ går till nästa program"));
        panel.mouseDown (hand (panel, kn[0].getCentre(), vanster)); panel.mouseUp (hand (panel, kn[0].getCentre(), {}));
        panel.mouseDown (hand (panel, kn[0].getCentre(), vanster)); panel.mouseUp (hand (panel, kn[0].getCentre(), {}));
        kontroll (p.programbank.aktuellt() == 2, juce::String::fromUTF8 ("◀ går till förra programmet"));
        const auto t = panel.bygg();
        kontroll (t.programNr == "03" && t.programNamn == p.programbank.program (2).namn, juce::String::fromUTF8 ("Programfönstret visar numret och namnet"));
        p.apvts.getParameter (rv::id::portTroskel)->setValueNotifyingHost (0.9f);
        kontroll (panel.bygg().programNamn.endsWith ("*"), juce::String::fromUTF8 ("En ändring visas med en stjärna"));
        p.programbank.ladda (0);
    }
    // Alla parametrar ska nås från panelen.
    for (auto* par : p.getParameters())
        if (auto* r = dynamic_cast<juce::AudioProcessorParameterWithID*> (par))
            kontroll (provade.contains (r->paramID), r->paramID + juce::String::fromUTF8 (": nås från panelen"));
    std::cout << "RVSnapshot --klicktest: " << antal << " kontroller, " << fel << " fel\n";
    return fel == 0 ? 0 : 1;
}

// Manualens bilder: hela panelen med programmet Parollen, och utsnitt i 200 %.
static int manual (const juce::File& mapp)
{
    RostverketProcessor p;
    p.setRateAndBufferSizeDetails (48000.0, 512);
    p.programbank.ladda (20);   // Parollen
    p.apvts.getParameter (rv::id::omvandlingen)->setValueNotifyingHost (0.35f);
    p.prepareToPlay (48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    juce::AudioBuffer<float> b (4, 512);
    juce::MidiBuffer m;
    // Lite "röst" (vokal med övertoner) genom bordet, så att mätarna står.
    for (int k = 0; k < 30; ++k)
    {
        b.clear();
        for (int i = 0; i < 512; ++i)
        {
            const float t = (float) (k * 512 + i) / 48000.0f;
            float v = 0.0f;
            for (int h = 1; h < 25; ++h) v += std::sin (2.0f * 3.14159f * 120.0f * (float) h * t) / (float) h * (h == 6 || h == 12 ? 3.0f : 1.0f);
            b.setSample (0, i, 0.12f * v); b.setSample (1, i, 0.12f * v);
        }
        p.processBlock (b, m);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (35);
    }
    auto& panel = dynamic_cast<RostverketEditor&> (*ed).panel;
    panel.setTransform ({});
    mapp.createDirectory();
    if (sparaPNG (panel, mapp.getChildFile ("panelen.png"), 2.0f, "Panelen") != 0) return 1;
    const auto hel = panel.createComponentSnapshot (panel.getLocalBounds(), true, 2.0f);
    namespace g = rv::stil::panel::geo;
    const std::vector<std::pair<const char*, juce::Rectangle<float>>> utsnitt {
        { "manual_toppen", { 24.0f, 22.0f, 1152.0f, 118.0f } },
        { "manual_banden", g::band.getUnion (g::tonskala).expanded (6.0f) },
        { "manual_porten", g::modul[0].expanded (4.0f) }, { "manual_vokodern", g::modul[1].expanded (4.0f) },
        { "manual_strupen", g::modul[2].expanded (4.0f) }, { "manual_sandaren", g::modul[3].expanded (4.0f) },
        { "manual_koren", g::modul[4].expanded (4.0f) }, { "manual_ekot", g::modul[5].expanded (4.0f) },
        { "manual_nederkanten", { 30.0f, 672.0f, 1140.0f, 64.0f } } };
    for (auto& [namn, r] : utsnitt)
    {
        const auto bild = hel.getClippedImage ((r * 2.0f).toNearestInt());
        const auto fil = mapp.getChildFile (juce::String (namn) + ".png");
        fil.deleteFile();
        juce::FileOutputStream ut (fil);
        juce::PNGImageFormat png;
        if (! ut.openedOk() || ! png.writeImageToStream (bild, ut)) { std::cout << "Kunde inte skriva " << fil.getFullPathName() << "\n"; return 1; }
    }
    std::cout << utsnitt.size() << " utsnitt i " << mapp.getFullPathName() << "\n";
    return 0;
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juce;
    if (argc < 2)
    {
        std::cout << "Användning: RVSnapshot fil.png [skala] [id=värde ...] | --stilprov | --stilprov-detaljer fil.png [skala] | --tid\n";
        return 1;
    }
    const juce::String lage (argv[1]);
    const auto cwd = juce::File::getCurrentWorkingDirectory();

    if (lage == "--klicktest") return klicktest();
    if (lage == "--manual" && argc >= 3) return manual (cwd.getChildFile (argv[2]));
    if (lage == "--tid")
    {
        // Bakgrunden (byggs en gång per skala) och reglagen (varje omritning) i 100 % och 200 %.
        for (float skala : { 1.0f, 2.0f })
        {
            juce::Image bild (juce::Image::RGB, (int) (rv::stil::panel::bredd * skala), (int) (rv::stil::panel::hojd * skala), true);
            juce::Graphics g (bild);
            g.addTransform (juce::AffineTransform::scale (skala));
            const auto t = rv::stil::prov::exempel();
            auto start = juce::Time::getMillisecondCounterHiRes();
            rv::stil::panel::bakgrund (g, t, skala);
            const auto bak = juce::Time::getMillisecondCounterHiRes() - start;
            start = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < 10; ++i) rv::stil::panel::reglage (g, t);
            const auto reg = (juce::Time::getMillisecondCounterHiRes() - start) / 10.0;
            std::cout << "Skala " << skala << ": bakgrund " << juce::String (bak, 1) << " ms (en gång), reglage "
                      << juce::String (reg, 1) << " ms per omritning\n";
        }
        return 0;
    }

    if (! lage.startsWith ("--"))
    {
        // Den riktiga panelen: processorn med givna värden och lite ljud genom den, så att mätarna rör sig.
        RostverketProcessor p;
        p.setRateAndBufferSizeDetails (48000.0, 512);
        for (int i = 3; i < argc; ++i)
        {
            const juce::String a (argv[i]);
            if (auto* par = p.apvts.getParameter (a.upToFirstOccurrenceOf ("=", false, false)))
                par->setValueNotifyingHost (par->convertTo0to1 (a.fromFirstOccurrenceOf ("=", false, false).getFloatValue()));
        }
        p.prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> b (4, 512);
        juce::MidiBuffer m;
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        for (int k = 0; k < 20; ++k)
        {
            b.clear();
            for (int i = 0; i < 512; ++i) { const float v = 0.4f * std::sin ((float) (k * 512 + i) * 0.03f); b.setSample (0, i, v); b.setSample (1, i, v); }
            p.processBlock (b, m);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (40);
        }
        auto& panel = dynamic_cast<RostverketEditor&> (*ed).panel;
        panel.setTransform ({});
        const float skala = argc > 2 ? (float) juce::String (argv[2]).getDoubleValue() : 1.0f;
        return sparaPNG (panel, cwd.getChildFile (argv[1]), skala, "Panelen");
    }

    if (argc < 3) return 1;
    const juce::File fil = cwd.getChildFile (argv[2]);
    const float skala = argc > 3 ? (float) juce::String (argv[3]).getDoubleValue() : 1.0f;

    if (lage == "--stilprov")
    {
        rv::stil::prov::Panel panel;
        panel.setBounds (0, 0, rv::stil::prov::Panel::bredd, rv::stil::prov::Panel::hojd);
        return sparaPNG (panel, fil, skala, "Stilprovet");
    }
    if (lage == "--stilprov-detaljer")
    {
        rv::stil::prov::Detaljer d;
        d.setBounds (0, 0, rv::stil::prov::Detaljer::bredd, rv::stil::prov::Detaljer::hojd);
        return sparaPNG (d, fil, skala, "Detaljarket");
    }
    std::cout << "Okänt läge: " << lage << "\n";
    return 1;
}
