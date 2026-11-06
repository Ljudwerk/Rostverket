// Ekot – det synkade ekot och ett litet rum.
//
// Ekot är Schaktet (krang/dsp/Schaktet.h, flyttat från Pannrummet): tid i DAW:ens tempo, återkoppling
// med mörker och mättnad, och ping-pong. Svajet står fast på 20 %. Rummet är krang/dsp/Rummet.h i
// lokalen SALONG, nära: ett litet studiorum som Rum blandar in.
//
// Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <krang/dsp/Schaktet.h>
#include <krang/dsp/Rummet.h>

namespace rv::dsp
{

class Ekot
{
public:
    void forbered (double fs) { schaktet.forbered (fs); rummet.forbered (fs); nollstall(); }
    void nollstall() noexcept { schaktet.reset(); rummet.reset(); }

    struct Param { float fjardedelar = 0.75f; double bpm = 120.0; float aterkoppling = 0.35f, morker = 0.5f, rum = 0.2f, blandning = 0.25f; bool pingpong = false; };

    void process (float* const* ljud, int kanaler, int n, const Param& p)
    {
        krang::dsp::SchaktParams s;
        s.fjardedelar = p.fjardedelar; s.bpm = p.bpm; s.aterkoppling = p.aterkoppling; s.morker = p.morker;
        s.svaj = 0.2f; s.blandning = p.blandning; s.pingpong = p.pingpong;
        schaktet.process (ljud, kanaler, n, s);
        rummet.satt (krang::dsp::Rummet::Lokal::salong, 0.0f, p.rum * 0.7f);
        if (kanaler > 1) rummet.process (ljud[0], ljud[1], n);
        else rummet.process (ljud[0], ljud[0], n);
    }

private:
    krang::dsp::Schaktet schaktet;
    krang::dsp::Rummet rummet;
};

} // namespace rv::dsp
