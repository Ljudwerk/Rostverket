// Fabriksbandet: Röstverkets fabriksprogram i fem kategorier – Robot, Vokoder, Kör, Paroll och Radio –
// och Init. Varje program anger bara det som skiljer sig från grundvärdena (alla moduler av).
// Värdena är parametrarnas riktiga värden: dB, ms, Hz, st, procent för Ekots återkoppling, 0–1 för
// rattar, index för vridomkopplare och 0/1 för vippor.
//
// MIDI in och förbikopplingen hör till uppkopplingen och sätts aldrig av ett program. Utan MIDI spelar
// Orgeln tonen på TON; programmen med vokoder anger en ton som passar.
//
// Utnivån är satt så att programmen ligger ungefär lika starkt som den torra rösten (mätt med
// RVRender --program), så att man kan bläddra utan stora nivåhopp.
//
// Lägg nya program sist; programnumren visas i programfönstret.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <krang/presets/Programbank.h>
#include "../Parameters.h"

namespace rv
{

inline std::vector<krang::presets::Program> fabriksbandet()
{
    using P = krang::presets::Program;
    using V = std::initializer_list<std::pair<const char*, float>>;
    auto prog = [] (const char* namn, const char* kategori, V v)
    {
        P p;
        p.namn = svt (namn);
        p.kategori = svt (kategori);
        // Porten är på i alla program utom Init: rösten in ska vara städad.
        if (juce::String (namn) != "Init") p.varden.push_back ({ id::portPa, 1.0f });
        for (auto& [pid, varde] : v) p.varden.push_back ({ pid, varde });
        return p;
    };

    return {
        prog ("Init", "Grund", {}),

        // --- Robot ---------------------------------------------------------------------------------
        prog ("Maskinmänniskan", "Robot", { { id::utNiva, -3.5f },
            { id::vokPa, 1 }, { id::vokBand, 3 }, { id::vokSnabbhet, 0.75f }, { id::vokFormant, -3 },
            { id::orgVag, 1 }, { id::orgTon, 41 } }),
        prog ("Monoton order", "Robot", { { id::utNiva, -2.0f },
            { id::strupPa, 1 }, { id::strupLage, 2 }, { id::strupTon, 45 }, { id::strupFoljsamhet, 1 }, { id::strupBlandning, 1 },
            { id::sandPa, 1 }, { id::sandLage, 1 }, { id::sandFrekvens, 150 }, { id::sandStyrka, 0.5f }, { id::sandBlandning, 0.4f } }),
        prog ("Talchippet", "Robot", { { id::utNiva, -3.0f },
            { id::strupPa, 1 }, { id::strupLage, 2 }, { id::strupTon, 52 }, { id::strupFoljsamhet, 0.9f }, { id::strupBlandning, 1 },
            { id::sandPa, 1 }, { id::sandLage, 0 }, { id::sandFrekvens, 6000 }, { id::sandStyrka, 0.9f }, { id::sandTon, 0.7f } }),
        prog ("Ringrösten", "Robot", { { id::utNiva, -5.5f },
            { id::sandPa, 1 }, { id::sandLage, 1 }, { id::sandFrekvens, 90 }, { id::sandStyrka, 1 }, { id::sandTon, 0.6f },
            { id::ekoPa, 1 }, { id::ekoTid, 2 }, { id::ekoAterkoppling, 30 }, { id::ekoBlandning, 0.2f } }),
        prog ("Robot i schaktet", "Robot", { { id::utNiva, -7.5f },
            { id::vokPa, 1 }, { id::orgVag, 1 }, { id::orgOktav, 1 }, { id::orgTon, 43 }, { id::vokSnabbhet, 0.7f },
            { id::ekoPa, 1 }, { id::ekoTid, 4 }, { id::ekoAterkoppling, 50 }, { id::ekoMorker, 0.7f }, { id::ekoRum, 0.5f }, { id::ekoBlandning, 0.3f } }),
        prog ("Kall kalkylator", "Robot", { { id::utNiva, -6.0f },
            { id::sandPa, 1 }, { id::sandLage, 0 }, { id::sandFrekvens, 4000 }, { id::sandStyrka, 1 }, { id::sandTon, 0.3f },
            { id::korPa, 1 }, { id::korLage, 0 }, { id::korBlandning, 0.4f } }),

        // --- Vokoder -------------------------------------------------------------------------------
        prog ("Sågtandskör", "Vokoder", { { id::utNiva, -6.5f },
            { id::vokPa, 1 }, { id::orgVag, 0 }, { id::orgTon, 48 }, { id::orgBrus, 0.08f },
            { id::korPa, 1 }, { id::korLage, 1 }, { id::korBlandning, 0.5f } }),
        prog ("Mässingsmaskin", "Vokoder", { { id::utNiva, -4.0f },
            { id::vokPa, 1 }, { id::orgVag, 2 }, { id::orgTon, 50 }, { id::vokSkarpa, 0.8f }, { id::vokSnabbhet, 0.8f } }),
        prog ("Viskningen", "Vokoder", { { id::utNiva, -5.5f },
            { id::vokPa, 1 }, { id::vokBarvag, 2 }, { id::vokSnabbhet, 0.85f }, { id::vokBegriplighet, 0.8f }, { id::vokSkarpa, 0.3f },
            { id::ekoPa, 1 }, { id::ekoTid, 4 }, { id::ekoRum, 0.4f }, { id::ekoBlandning, 0.15f } }),
        prog ("Basen talar", "Vokoder", { { id::utNiva, -4.0f },
            { id::vokPa, 1 }, { id::vokBarvag, 1 }, { id::vokBand, 2 }, { id::vokSnabbhet, 0.8f }, { id::vokBegriplighet, 0.6f } }),
        prog ("Åtta band", "Vokoder", { { id::utNiva, -4.0f },
            { id::vokPa, 1 }, { id::vokBand, 0 }, { id::vokSkarpa, 0.3f }, { id::orgVag, 1 }, { id::orgTon, 45 } }),
        prog ("Glidande orgel", "Vokoder", { { id::utNiva, -5.0f },
            { id::vokPa, 1 }, { id::vokSnabbhet, 0.4f }, { id::orgVag, 2 }, { id::orgGlid, 120 }, { id::orgTon, 48 },
            { id::ekoPa, 1 }, { id::ekoTid, 3 }, { id::ekoBlandning, 0.2f } }),
        prog ("Spöket i vokodern", "Vokoder", { { id::utNiva, -7.5f },
            { id::vokPa, 1 }, { id::vokFormant, 5 }, { id::orgBrus, 0.3f }, { id::orgTon, 55 },
            { id::ekoPa, 1 }, { id::ekoTid, 5 }, { id::ekoAterkoppling, 55 }, { id::ekoRum, 0.6f }, { id::ekoBlandning, 0.3f }, { id::ekoPingpong, 1 } }),

        // --- Kör -----------------------------------------------------------------------------------
        prog ("Dubbelröst", "Kör", { { id::utNiva, -7.0f },
            { id::korPa, 1 }, { id::korLage, 0 }, { id::korBlandning, 0.5f } }),
        prog ("Kören i hallen", "Kör", { { id::utNiva, -12.0f },
            { id::korPa, 1 }, { id::korLage, 1 }, { id::korBlandning, 0.6f },
            { id::ekoPa, 1 }, { id::ekoTid, 4 }, { id::ekoAterkoppling, 25 }, { id::ekoRum, 0.6f }, { id::ekoBlandning, 0.2f } }),
        prog ("Massan marscherar", "Kör", { { id::utNiva, -5.0f },
            { id::korPa, 1 }, { id::korLage, 2 }, { id::korBredd, 1 }, { id::korBlandning, 0.6f },
            { id::strupPa, 1 }, { id::strupLage, 1 }, { id::strupTonhojd, -12 }, { id::strupBlandning, 0.3f } }),
        prog ("Oktav under", "Kör", { { id::utNiva, -4.5f },
            { id::strupPa, 1 }, { id::strupLage, 1 }, { id::strupTonhojd, -12 }, { id::strupBlandning, 0.5f },
            { id::korPa, 1 }, { id::korLage, 0 }, { id::korBlandning, 0.4f } }),
        prog ("Oktav över", "Kör", { { id::utNiva, -6.0f },
            { id::strupPa, 1 }, { id::strupLage, 1 }, { id::strupTonhojd, 12 }, { id::strupBlandning, 0.35f },
            { id::korPa, 1 }, { id::korLage, 1 }, { id::korBlandning, 0.5f } }),
        prog ("Munkarna", "Kör", { { id::utNiva, -11.5f },
            { id::strupPa, 1 }, { id::strupLage, 0 }, { id::strupTonhojd, -5 }, { id::strupFormant, -4 }, { id::strupBlandning, 1 },
            { id::korPa, 1 }, { id::korLage, 2 }, { id::korBlandning, 0.6f },
            { id::ekoPa, 1 }, { id::ekoTid, 6 }, { id::ekoRum, 0.7f }, { id::ekoBlandning, 0.15f } }),

        // --- Paroll --------------------------------------------------------------------------------
        prog ("Parollen", "Paroll", { { id::utNiva, -6.5f },
            { id::portKompression, 0.6f }, { id::portNarhet, 0.6f },
            { id::strupPa, 1 }, { id::strupLage, 1 }, { id::strupTonhojd, -12 }, { id::strupBlandning, 0.4f },
            { id::ekoPa, 1 }, { id::ekoTid, 5 }, { id::ekoAterkoppling, 35 }, { id::ekoRum, 0.3f }, { id::ekoBlandning, 0.25f } }),
        prog ("Från talarstolen", "Paroll", { { id::utNiva, -10.5f },
            { id::portKompression, 0.5f },
            { id::ekoPa, 1 }, { id::ekoTid, 6 }, { id::ekoAterkoppling, 45 }, { id::ekoRum, 0.6f }, { id::ekoBlandning, 0.3f } }),
        prog ("Ledaren talar", "Paroll", { { id::utNiva, -5.5f },
            { id::portNarhet, 0.5f },
            { id::strupPa, 1 }, { id::strupLage, 0 }, { id::strupTonhojd, -3 }, { id::strupFormant, -2 }, { id::strupBlandning, 1 },
            { id::ekoPa, 1 }, { id::ekoTid, 4 }, { id::ekoBlandning, 0.2f } }),
        prog ("Massmötet", "Paroll", { { id::utNiva, -13.0f },
            { id::korPa, 1 }, { id::korLage, 2 }, { id::korBlandning, 0.55f },
            { id::ekoPa, 1 }, { id::ekoTid, 5 }, { id::ekoRum, 0.7f }, { id::ekoBlandning, 0.25f } }),
        prog ("Högtalaren på torget", "Paroll", { { id::utNiva, -9.0f },
            { id::sandPa, 1 }, { id::sandLage, 2 }, { id::sandTon, 0.7f }, { id::sandStyrka, 0.3f },
            { id::ekoPa, 1 }, { id::ekoTid, 3 }, { id::ekoAterkoppling, 40 }, { id::ekoRum, 0.5f }, { id::ekoBlandning, 0.3f } }),
        prog ("Marschorder", "Paroll", { { id::utNiva, -4.0f },
            { id::portKompression, 0.8f },
            { id::strupPa, 1 }, { id::strupLage, 1 }, { id::strupTonhojd, -12 }, { id::strupBlandning, 0.3f },
            { id::ekoPa, 1 }, { id::ekoTid, 2 }, { id::ekoAterkoppling, 40 }, { id::ekoBlandning, 0.25f }, { id::ekoPingpong, 1 } }),

        // --- Radio ---------------------------------------------------------------------------------
        prog ("Kortvåg från öst", "Radio", { { id::utNiva, -6.0f },
            { id::sandPa, 1 }, { id::sandLage, 2 }, { id::sandStyrka, 0.7f }, { id::sandTon, 0.4f },
            { id::ekoPa, 1 }, { id::ekoTid, 4 }, { id::ekoAterkoppling, 40 }, { id::ekoBlandning, 0.25f } }),
        prog ("Nattsändning", "Radio", { { id::utNiva, -7.5f },
            { id::sandPa, 1 }, { id::sandLage, 2 }, { id::sandStyrka, 0.9f }, { id::sandTon, 0.3f },
            { id::ekoPa, 1 }, { id::ekoTid, 6 }, { id::ekoRum, 0.4f }, { id::ekoBlandning, 0.2f } }),
        prog ("Störsändaren", "Radio", { { id::utNiva, -7.0f },
            { id::sandPa, 1 }, { id::sandLage, 2 }, { id::sandStyrka, 1 }, { id::sandFrekvens, 1200 }, { id::sandTon, 0.5f },
            { id::korPa, 1 }, { id::korLage, 0 }, { id::korSvaj, 1 }, { id::korBlandning, 0.5f } }),
        prog ("Sifferstationen", "Radio", { { id::utNiva, -8.5f },
            { id::sandPa, 1 }, { id::sandLage, 0 }, { id::sandFrekvens, 8000 }, { id::sandStyrka, 0.6f }, { id::sandTon, 0.5f },
            { id::ekoPa, 1 }, { id::ekoTid, 4 }, { id::ekoRum, 0.5f }, { id::ekoBlandning, 0.2f } }),
    };
}

} // namespace rv
