# Röstverket – plan

*Modell RV-83. Sändarbordet i en radiostation, 1950–70-tal: rösten går in genom mikrofonen och kommer ut som maskin, kör eller paroll.*

**Status:** Fas 0 klar och grind 0 godkänd 6 november 2026. Fas 1 klar och grind 1 godkänd 6 november 2026. Fas 2 klar och grind 2 godkänd 6 november 2026. Fas 3 byggd 6 november 2026: fabriksbandet (30 program), programknapparna, appen och manualen. Grind 3 väntar, och vem som godkänner den är inte bestämt.

Effektplugin (VST3, AU, CLAP) med MIDI-ingång och sidokedja, och fristående app, för macOS. Bygger på samma stack som Pannrummet och Kroppsmaskinen. Allt i gränssnittet är på svenska.

## 1. Mål

**Röstverket ska göra en vanlig röst till den röst som EBM, syntpop och industri använder: vokoderns maskin, robotens monotoni, körens massa och radions paroll – på en insert.**

Måttstockar: rösterna hos Front 242, Laibach, Frontline Assembly, Depeche Mode, Gary Numan, Nitzer Ebb och Bigod 20. Dit hör vokodern som spelas av en synt, den djupa oktavdubblingen, den monotona robotrösten, den dubbelspelade kören och kortvågsrösten. Vi kopierar inget, utan lyssnar på dem för att veta när vi är framme.

Det här måste finnas:

- **En vokoder som går att förstå.** 8–20 band och en bärvåg från MIDI, sidokedjan eller brus. Sibilanterna ska gå igenom, så att orden hörs.
- **En inbyggd bärvåg.** Orgeln spelas från MIDI, så att man inte behöver en synt på ett annat spår.
- **Tonhöjd och formant var för sig.** Oktav under för den djupa parollrösten. Monotonläget plattar röstens tonhöjd till en enda ton, så att den låter som en robot.
- **Talchippet, ringen och kortvågen.** Datorrösten, metallrösten och rösten från en annan sida av järnridån.
- **Dubbling och kör.** En röst blir två eller åtta.
- **Ett synkat eko och ett litet rum.** Det räcker inte att bara ha Pannrummets hall efter.
- **Konstant, rapporterad latens.** Bitexakt förbikoppling.

### Inte i 1.0

- Talsyntes från text (fonem, LPC) – kandidat i fas 5
- Tonhöjdskorrigering mot skala ("autotune")
- MIDI ut från röstens tonhöjd
- Fri ordning i kedjan
- Windows och Linux

## 2. Aggregatet

```
 RÖST IN ─▶ 1 PORTEN ─▶ 2 VOKODERN ─▶ 3 STRUPEN ─▶ 4 SÄNDAREN ─▶ 5 KÖREN ─▶ 6 EKOT ─▶ TORR/VÅT ─▶ UT
                            ▲
            BÄRVÅGEN: ORGELN (MIDI) · SIDOKEDJAN (kanal 3/4) · BRUS
                ▲────────── OMVANDLINGEN (makrospak: människa → maskin) ──────────▲
 PÅ LUFTEN = förbikoppling av hela bordet (mjuk övertoning, bitexakt)
```

Varje modul har en vippa (PÅ/AV) och en lampa. En avstängd modul räknar inte.

### 1 Porten – rösten in

Det här gör en inspelad röst redo för maskinen.

- **Reglage:** tröskel (grind mot rummets brus), kompression (en ratt, fast knä), högpass (40–300 Hz) och närhet (en lätt presence-topp).
- **Avsibilanten:** en växel som tar ner s-ljuden före Sändaren och Ekot. Vokodern får fortfarande dem orörda, eftersom det är de som gör orden begripliga.

Utgår från Grinden i Pannrummet och `krang/dsp/Grund.h`.

### 2 Vokodern – kroppen

Analysen sker med bandpassfilter (4:e ordningen) och följare. Syntesen gör samma bandindelning på bärvågen.

- **Band** (vridomkopplare): **8**, **12**, **16** och **20** band, logaritmiskt mellan 100 Hz och 8 kHz.
- **Bärvåg** (vridomkopplare):
  - **ORGELN**: den inbyggda, se nedan.
  - **SIDOKEDJA**: pluginets andra ingång, till exempel Kroppsmaskinens bas.
  - **BRUS**: viskning och vind.
- **Formant** (−12 till +12 halvtoner) förskjuter syntesbanden mot analysbanden. Uppåt blir rösten liten och ljus, nedåt jättelik.
- **Snabbhet:** följarnas attack och släpp i en ratt. Snabbt ger tal, långsamt ger en mjuk pad.
- **Skärpa:** bandens Q.
- **Begriplighet:** tonlösa ljud (s, f, t) känns igen och blandas in som högpassat original eller brus.
- **FRYS:** håller bandens nivåer, så att en stavelse blir en ton som orgeln kan spela.
- **Blandning:** vokoder mot torr röst.

**Orgeln** är den inbyggda bärvågen, åtta stämmor från MIDI.

- **Vågformer:** **SÅG**, **PULS** och **SÅG+PULS**, med oktav (−2 till +1) och glid.
- **Brus:** en ratt för brus i bärvågen.
- **Utan MIDI** spelar Orgeln tonen på ratten **TON** (en hållen ton). Är MIDI-brytaren av gäller också TON.
- **Stämmorna** utgår från Kolvbasens oscillatorer i Kroppsmaskinen (polyBLEP), utan filter.

### 3 Strupen – tonhöjden och formanten

En PSOLA-tonhöjdsförskjutare med tonhöjdsdetektering (YIN). Den har tre lägen:

- **SKIFT:** tonhöjd ±12 halvtoner och formant ±12 halvtoner var för sig.
- **OKTAV:** den torra rösten med en oktav under (eller över) blandad – parollrösten.
- **MONOTON:** röstens tonhöjd plattas till en enda ton, från senaste MIDI-ton eller ratten TON. Det är roboten utan vokoder.

Reglagen är tonhöjd, formant, blandning och följsamhet (hur hårt MONOTON drar). Strupens fönster ger den fasta latensen (se Kod och struktur).

### 4 Sändaren – talchippet, ringen och kortvågen

- **TALCHIP:** sampletakt 4–16 kHz utan filter före, och 4–8 bitar – datorrösten.
- **RING:** ringmodulator 20–2000 Hz, eller grundtonen från MIDI – metallrösten.
- **KORTVÅG:** smalt band, fädning, heterodynvisslan och brus – rösten på avstånd.
- **Reglage:** styrka, frekvens (sampletakt eller ringens frekvens, beroende på läget), ton och blandning.

Megafonen och telefonen finns redan i Pannrummet och upprepas inte här.

### 5 Kören – dubblingen

Läget väljs med en vridomkopplare:

- **DUBBEL:** en kopia med 15–40 ms förskjutning och litet tonhöjdssvaj.
- **KÖR:** fyra röster, spridda i tid, tonhöjd och stereo.
- **MASSA:** åtta röster, bredare och långsammare.

Reglagen är spridning, svaj, bredd och blandning. Utgår från `krang/dsp/Choruset.h`, med egna röster för tidsförskjutningen.

### 6 Ekot – ekot och rummet

- **Ekot:** synkad tid (1/16–1/2, prickad och triol), återkoppling, mörker och ping-pong. Ekot är Schaktet i Pannrummet.
- **Rummet:** en ratt för ett litet studiorum (`krang/dsp/Rummet.h`).
- **Blandning.**

### Omvandlingen – makrospaken

Spaken drar från människa till maskin, men bara på påslagna moduler:

- vokoderns blandning upp
- snabbare och hårdare följare
- Strupens följsamhet upp
- mer Sändare
- Kören tätare

Nivån kompenseras. Spaken visas som avstämningsindikatorns "magiska öga", som sluts mot maskin.

### Utgången

- **Reglage:** in-nivå, torr/våt, utnivå och limiter (`krang/dsp/Limiter.h`, tak −0,3 dBFS).
- **Mätare:** IN och UT som VU-mätare.
- **PÅ LUFTEN:** en röd glasskylt som lyser när bordet är inkopplat. Ett klick kopplar förbi hela bordet. DAW:ens bypass gör samma sak.
- **MIDI:** en brytare (av från början, som i Pannrummet). Av betyder att Röstverket struntar i inkommande MIDI.

## 3. Gränssnittet – radiohuset

### Idén

**Sändarbordet i en radiostation, 1950–70-tal.** Kroppsmaskinen är vardagsrummets svarta låda och Pannrummet en styrpanel på verkstadsgolvet. Röstverket är bordet där rösten sänds ut.

- **Chassit:** en lutande panel i kittgrå lackad plåt, infälld i ett bord av valnötsfaner med mässingslist. Skruvar och nitar finns, men ingen rost och inget maskingrönt.
- **Reglagen:** elfenbensvita bakelitrattar med räfflad kjol och kromad skalring. Skalan går 0–10 och är tryckt i svart. Vridomkopplarna har räfflad spets, och vipporna är små tumvippor i krom.
- **Bandmätaren:** en rad med 20 neonstaplar (glimrör i orange) visar vokoderns band. Staplarna lyser och slocknar med rösten och är panelens hjärta. FRYS gör dem stilla.
- **Instrumenten:**
  - VU-mätare för IN och UT med gulnad tavla
  - det magiska ögat (grön avstämningsindikator) för Omvandlingen
  - en tonskala som på en radio, för Orgelns och Strupens ton
- **Skyltarna:** graverade elfenbensplaketter med svart text, en röd **PÅ LUFTEN**-skylt i glas och modellskylten **RÖSTVERKET RV-83** i präglad mässing.
- **Motivet:** en bandmikrofon i profil (generisk, ingen modell) på modellskylten och en mönstrad högtalarduk till vänster.

Färger: kittgrått, valnöt, elfenben, mässing, neonorange, ögongrönt och signalrött. Panelens text är Linjeskriften (`krang/stil/Linjeskrift.h`) i svart på elfenben och på lacken.

### Layout (designstorlek 1200 × 760, skalas 60–160 %)

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│ [duk] [RÖSTVERKET RV-83 mässing]  [VU IN][VU UT]   (öga) OMVANDLINGEN   [PÅ LUFTEN] │
├──────────────────────────────────────────────────────────────────────────────────┤
│ ▮▮▮▮▮▮▮▮▮▮▮▮▮▮▮▮▮▮▮▮  BANDMÄTAREN – 20 neonstaplar        [tonskala ORGELN/STRUPEN] │
├─────────────┬──────────────────────────────┬─────────────────────────────────────┤
│ 1 PORTEN    │ 2 VOKODERN (bred)            │ 3 STRUPEN                           │
│             │ band · bärvåg · ORGELN       │                                     │
├─────────────┼──────────────────────────────┼─────────────────────────────────────┤
│ 4 SÄNDAREN  │ 5 KÖREN                      │ 6 EKOT                              │
├─────────────┴──────────────────────────────┴─────────────────────────────────────┤
│ [PROGRAM ◀ ▶ SPARA MENY]   IN-NIVÅ  TORR/VÅT  UTNIVÅ  LIMITER   MIDI   ▤ valnöt   │
└──────────────────────────────────────────────────────────────────────────────────┘
```

Det är en enda vy utan flikar.

### Teknik

- Allt ritas i kod, utan bild- eller typsnittsfiler: lack, valnöt, bakelit, krom, glimrör och glas.
- Bakgrunden cachas. Bara bandmätaren, VU-mätarna, ögat och lamporna ritas om, högst 30 bilder/s. Målet är under 1 % CPU för gränssnittet på en M1.
- `RVSnapshot` ritar panelen och stilprovet till PNG.

## 4. Kod och struktur

Stacken är JUCE 8, CMake, clap-juce-extensions, C++20 och AGPLv3.

| Vad | Värde |
| --- | --- |
| Namnrum | `rv` |
| Parameter-ID:n | `rv_<del>_<vad>` |
| Tillverkare | `Rvrk` |
| Plugin-kod | `Rv83` |
| Bundle-ID och CLAP-ID | `org.rostverket.rv83` |
| PRODUCT_NAME | `Rostverket` |
| Programändelse | `.rvprog` |

Bussarna är stereo in och ut och en sidokedja (stereo, på från början). AU är en *music effect* (`aumf`), eftersom MIDI behövs till Orgeln och Strupen.

**Latensen** är fast: Strupens fönster plus eventuell översampling i Sändaren, med målet under 20 ms i 48 kHz. Den rapporteras alltid, även när Strupen är av. Den torra vägen och förbikopplingen fördröjs lika mycket. I fas 0 är latensen 0.

```
rostverket/
  Source/
    PluginProcessor.*, PluginEditor.*, Parameters.h
    dsp/      Porten, Vokodern, Orgeln, Strupen, Sandaren, Koren, Ekot, Utgangen
    ui/       Panelen.h; ui/stil/ (Palett, Radiohuset, Panelbild, Stilprov)
    presets/  Fabriksbandet.h
  tools/      RVRender.cpp, RVSnapshot.cpp, reaper/
  docs/       stilprov.png, panelen.png, Manual.md, Rostverket_i_Reaper.md
  renders/, lyssning/
```

| Från / till krang-libs | Ursprung | Fas |
| --- | --- | --- |
| `krang/dsp/Grund.h`, `Limiter.h` | Finns | 0 |
| `krang/stil/Linjeskrift.h`, `Textur.h`, `ui/ValueFormat.h` | Finns | 0 |
| `krang/presets/Programbank.h` | Finns | 3 |
| `krang/dsp/Choruset.h`, `Rummet.h` | Finns (Pianot) | 2 |
| `krang/dsp/Schaktet.h` (ekot) | Flyttat från Pannrummet 6 november 2026. Bevisat med `PRRender --test` och bitidentiska renderingar. | 2 |
| `krang/dsp/Rummet.h` – `reset()` | Tillagd 6 november 2026 (tömmer utan att allokera) | 2 |
| `krang/dsp/Vokodern.h` | Ny i Röstverket. Flyttas till krang-libs först när Kroppsmaskinens *Ropet* behöver den. | 5 |
| `krang/app/Fristaende.h`, `Inspelaren.h` | Finns. Appen med mikrofon in. | 3 |

Regler:

- **Parameter-ID:n får aldrig döpas om**, och nya läggs sist. Alla modulers parametrar skapas i fas 0, så att ID:n och ordning ligger fast.
- Ljudtråden allokerar inget och låser inget. Vokoderns band förbereds i `prepareToPlay`, så att bandantalet byts utan allokering.
- Varje modul får testfall i `RVRender --test` innan den räknas som klar.

## 5. Faser och grindar

### Fas 0 – Grunden och utseendet

- Repo, CMake, krang-libs, `local.sh`, `real.sh` och CI.
- Ett tomt effektplugin som släpper igenom ljudet bitexakt, med sidokedja, MIDI in och alla parametrar.
- `RVRender --test`: bitexakt genomsläpp, förbikoppling, mono in, ingen NaN, och att parametrarnas ordning ligger fast.
- **Stilprov** i `RVSnapshot`: radiohuset med bandmätaren, rattarna, vridomkopplarna, vipporna, VU-mätarna, det magiska ögat, tonskalan, PÅ LUFTEN och modellskylten. Bara bilder.

**Grind 0:** Stilprovet är godkänt, och pluginet laddar i Reaper och släpper igenom ljudet bitexakt.

*Klar. Grind 0 godkänd (6 november 2026): stilprovet godkänt, pluginet laddat i Reaper på Macen och fungerar. Gjort:*

- *Repot med CMake, `local.sh`, `real.sh`, `.gitignore`, LICENSE och CI-arbetsflödet. CI hämtar krang-libs bredvid och är inte provkört. Ingen `git init` än.*
- *`Source/Parameters.h` med alla 54 parametrar för de sex modulerna, Orgeln, Omvandlingen och utgången. Processorn släpper igenom huvudbussen orörd. Sidokedjan är på från början i VST3 och CLAP men saknas i AU, som i Pannrummet.*
- *`RVRender --test`: 10 kontroller, och alla går igenom på Linux:*
  - *bitexakt i block om 512 och 37*
  - *förbikopplingen*
  - *mono*
  - *sidokedja och MIDI påverkar inte*
  - *alla reglage i topp och botten*
  - *latensen 0*
  - *tillståndet*
  - *parametrarnas antal och ordning*
- *`RVRender --parametrar` skriver ordningen.*
- *`RVSnapshot --stilprov docs/stilprov.png`, `--stilprov-detaljer` och `--tid`.*
- *Pluginets panel ritas med samma kod som stilprovet och visar parametrarnas värden. Modulerna är märkta EJ INKOPPLAD. Panelen går inte att vrida på än, så Reaper används med den generiska vyn. Storleken (60–160 %) ändras i hörnet.*
- *Ritningstider på Linux: bakgrunden 75 ms i 100 % och 290 ms i 200 % (en gång per storlek), reglagen 8 ms och 17 ms per omritning. Reglagen ritas bara om när ett värde ändras.*
- *pluginval nivå 5 på VST3 går igenom på Linux, även med editortesterna. Mac-bygget och Reaper provades av Mikke vid grind 0. AU, CLAP och appen är inte provade var för sig.*
- *Avvikelse: panelens text är svart på det kittgrå, inte vitt. Vitt blev för svagt på den ljusa lacken.*

### Fas 1 – Vokodern

- Porten, Vokodern med alla tre bärvågorna, Orgeln från MIDI och FRYS.
- Utgången: in-nivå, torr/våt, utnivå, limiter, PÅ LUFTEN och MIDI-brytaren.
- Panelen kopplad i enkel form (som Pannrummet i fas 1), så att fas 1 och 2 kan provas i Reaper.
- `RVRender`: röst genom vokodern med bärvåg från en MIDI-fil eller en WAV i sidokedjan, till `lyssning/fas1/`. Rösterna är talsyntes (espeak-ng) och en riktig röst som Mikke spelar in.
- Testfall: NaN, topp, DC, tystnad efter tystnad, klick vid bandbyte och bärvågsbyte, mono, begriplighet (energi i 4–8 kHz när ett s-ljud kommer in) och CPU (20 band under 3 % av en kärna).

**Grind 1:** Fyra exempel lyssnas A/B:

- tal med ett sågtandsackord från MIDI
- tal med Kroppsmaskinens bas i sidokedjan
- viskning med brusbärvåg
- en frusen stavelse som spelas som orgel

*Klar. Grind 1 godkänd (6 november 2026) av Mikke efter lyssning på `lyssning/fas1/` (talsyntes). Gjort och avvikelser:*

- *Porten (`Source/dsp/Porten.h`):*
  - *Ordningen är högpass (12 dB/okt), grind, kompressor och närhet.*
  - *Grinden och kompressorn har en gemensam detektor, den starkaste kanalen.*
  - *Grinden öppnar på 1 ms, stänger på 60 ms och går ned till −60 dB.*
  - *Kompressorn är en ratt: tröskel −12 till −30 dB och förhållande 1:1 till 6:1.*
  - *Närhet är en topp på 3,2 kHz, upp till +8 dB.*
  - *Avsibilanten tar ner s-bandet kring 6,5 kHz med upp till −10 dB, bara i den torra vägen. Vokodern får rösten före avsibilanten.*
- *Vokodern (`Source/dsp/Vokodern.h`):*
  - *Banden har fjärde ordningen (två RBJ-bandpass i rad), mellan 100 Hz och 8 kHz.*
  - *Snabbhet: attack 1–40 ms och släpp 12–400 ms.*
  - *Skärpa: bandbredden går från 1,5 till 0,55 bandavstånd.*
  - *Formanten förskjuter bara syntesbanden.*
  - *Nivån förs tillbaka till röstens med en långsam nivåkompensation, upp till +36 dB.*
  - *Ett bandbyte tonas ut och in med en halv cosinus på 12 ms.*
  - *Begripligheten känner igen tonlösa ljud på förhållandet mellan rösten över 4 kHz och under 1 kHz och blandar in rösten högpassad (4,5 kHz).*
- *Avvikelse i Vokodern: konsonanterna släpps bara igenom när bärvågen klingar. Utan ton (MIDI på utan toner, eller en tyst sidokedja) är vokodern helt tyst. Annars hördes s-ljuden ensamma.*
- *Orgeln (`Source/dsp/Orgeln.h`):*
  - *Åtta stämmor med polyBLEP-såg och -puls, attack 3 ms och släpp 40 ms.*
  - *En stulen stämma glider över till den nya tonen i stället för att klicka.*
  - *Bruset följer stämmorna, så att en orgel utan ton är tyst.*
  - *Med MIDI av håller stämma 0 tonen på TON.*
- *Utgången:*
  - *Ordningen är in-nivå, torr/våt, utnivå och limiter.*
  - *PÅ LUFTEN, Porten och Vokodern tonas in och ut på 10 ms. En modul som slås på börjar från noll.*
  - *Med allt avslaget och nivåerna på 0 dB hoppas hela kedjan över, så att ljudet passerar bitexakt.*
- *Avvikelse i grundvärdena: **Porten och Vokodern är av i Init** (var på i fas 0). Init är då bitexakt, som principerna kräver. ID:n och ordningen är oförändrade.*
- *Panelen (`Source/ui/Panelen.h`):*
  - *Den är kopplad till alla 54 parametrar.*
  - *Rattar dras upp/ned (Skift = fint), rullas och ger grundvärdet vid dubbelklick.*
  - *Vridomkopplare: klick = nästa, Skift-klick = förra, högerklick = lista.*
  - *Vippor, PÅ LUFTEN, LIMITER och MIDI växlar vid klick, och MENY ger storleken.*
  - *Tonskalan är Orgelns TON: klick sätter tonen, dragning finjusterar.*
  - *Bandmätaren visar vokoderns band.*
  - *Strupen, Sändaren, Kören och Ekot är märkta EJ INKOPPLAD.*
- *`RVRender --test`: 26 kontroller, och alla går igenom. Nytt är:*
  - *Orgelns stämning (0,0 ct) och stöld*
  - *vokodern med Orgeln, sidokedjan och brus*
  - *tystnad efter tystnad*
  - *begripligheten (+6,8 dB i 4–8 kHz)*
  - *formanten*
  - *FRYS*
  - *inga klick vid bandbyte (1,04×) eller vid PÅ LUFTEN och modulbyten (1,00×)*
  - *MIDI utan toner är tyst*
  - *mono*
  - *Portens högpass (−31 dB vid 50 Hz) och grind*
  - *CPU: Porten och Vokodern med 20 band i stereo tar 1,2–1,9 % av en kärna (Linux, molnet), under målet 3 %.*
- *`RVSnapshot --klicktest`: 182 kontroller och 0 fel. Det drar, dubbelklickar och rullar på varje ratt, klickar fram och tillbaka på varje vridomkopplare och växlar varje vippa. Det kontrollerar också att alla 54 parametrar nås från panelen.*
- *pluginval nivå 5 (VST3, Linux) går igenom. En körning med editortesterna kraschade när pluginval avslutades, efter att alla tester hade gått igenom. Det gick inte att upprepa på fyra körningar till, men det ska hållas under uppsikt på Macen.*
- *Grind 1-exemplen i `lyssning/fas1/` görs av `RVRender --exempel`. Rösten är talsyntes (espeak-ng, svensk röst: "Vi är maskinerna. Arbeta. Lyda. Dansa. Rösten går ut över hela landet."):*
  - *`0_rost_torr`: rösten utan effekt*
  - *`1a_sagackord_midi`: sågtandsackord Cm–Ab–Bb–G från MIDI*
  - *`1b_sidokedja_kolvbas`: Kroppsmaskinens bas, `grind0_kolv125_bas.wav`, i sidokedjan*
  - *`1c_viskning_brus`: brus som bärvåg*
  - *`1d_frusen_stavelse_orgel`: FRYS, och stavelsen spelas som en melodi*
  - *`1e_robot_puls_formant`: puls en oktav ned, formant −4 och en hållen ton*
- *Grind 1 bör också prövas med en riktig röst i Reaper.*

### Fas 2 – Resten av bordet

- Strupen (SKIFT, OKTAV och MONOTON), Sändaren, Kören, Ekot och Omvandlingen.
- Schaktet flyttas till krang-libs. Pannrummets tester ska gå igenom efteråt.
- Latensen sätts och rapporteras. Testet kontrollerar att torr och våt ligger i fas.

**Grind 2:** En testlåt i Reaper, byggd av `tools/reaper/`: paroller och en refräng genom Röstverket över ett Kroppsmaskinen-spår, med Orgeln spelad från MIDI. Lyssnad och godkänd.

*Klar. Grind 2 godkänd (6 november 2026) av Mikke efter lyssning på testlåten Sändningen i Reaper. Gjort och avvikelser:*

- *Strupen (`Source/dsp/Strupen.h`) är PSOLA:*
  - *YIN på en nedsamplad kopia (24 kHz) var 5:e ms, 70–600 Hz.*
  - *Analysmärkena läggs en period i taget och flyttas till periodens topp.*
  - *Kornen är Hann-fönster två perioder långa. De läses med formantkvoten och läggs ut med avståndet period/tonhöjdskvot, och fönstersumman delas bort.*
  - *Tonlösa ljud får korn med fast längd utan tonhöjdsändring.*
  - *Utan tonhöjdsändring läggs kornen exakt på märkena, så att rösten ligger i fas med den torra (korrelation 0,99).*
  - *MONOTON drar grundtonen mot TON. Följsamhet är exponenten på kvoten.*
- ***Avvikelse: latensen är 25 ms (1 200 sample vid 48 kHz), inte under 20 ms.** Två perioder av den lägsta tonen (70 Hz) behöver plats för att kornen inte ska tappa början. Inget läge med låg latens behövs (beslut 6 november 2026). Latensen rapporteras alltid, och den torra vägen och förbikopplingen fördröjs lika mycket. Init och förbikoppling är bitexakta, fördröjda med latensen.*
- *Sändaren (`Source/dsp/Sandaren.h`):*
  - *TALCHIP: sample-and-hold på Frekvens (4–16 kHz) utan filter före, och 8 till 4 bitar efter Styrka.*
  - *RING: ringmodulator 20–2 000 Hz, eller senaste MIDI-ton med MIDI-vippan på. Styrka går från AM till ren ring.*
  - *KORTVÅG: bandpass 700–2 100 Hz (Ton), smalare med Styrka, fädning var 0,4–1,2 s, en vissling på Frekvens, brus, mättnad och mottagarens lågpass 3 kHz.*
  - *Ton är ett lågpass 2–16 kHz i TALCHIP och RING. Nivån hålls med nivåkompensation.*
- *Kören (`Source/dsp/Koren.h`):*
  - *Lägena ger 1, 3 eller 7 extra röster, var och en en modulerad fördröjning (12–45 ms).*
  - *Svajet är upp till ±12 ct med egen takt per röst.*
  - *Bredden placerar rösterna i stereo, och rösterna tonas in och ut på 20 ms när läget byts.*
- *Ekot (`Source/dsp/Ekot.h`) är Schaktet ur krang-libs med svaj 20 %, plus `krang/dsp/Rummet.h` (SALONG, nära) som Rum blandar in.*
- *Schaktet är flyttat från Pannrummet till `krang/dsp/Schaktet.h`. Pannrummets `Source/dsp/Schaktet.h` är en vidarekoppling. Flytten provades så här:*
  - *`PRRender --test` går igenom, med 173 kontroller OK.*
  - *Pannrummet renderat före och efter flytten, med Schaktet i två lägen (ping-pong och svaj, och 90 % återkoppling med Trycket), ger bitidentiska filer.*
- *`krang/dsp/Rummet.h` fick `reset()`, som tömmer linjerna utan att allokera, så att Ekot börjar från tystnad när det slås på. Ljudet ändras inte, men Pianots och Stråkarnas tester (`PIRender --test`, `STRender --test`) är inte körda: de behöver samplingarna på Macen.*
- *Omvandlingen gäller bara påslagna moduler:*
  - *Vokodern: blandningen, snabbheten och skärpan dras mot fullt.*
  - *Strupen: följsamheten dras mot 1.*
  - *Sändaren: styrkan dras mot fullt och blandningen höjs upp till +50 %.*
  - *Kören: spridningen minskar 60 % och svajet 50 % (tätare).*
  - *Nivån ändras 1,0 dB mellan 0 och 100 % i testet.*
- *Rättning i Vokodern: FRYS håller nu nivåkompensationen. Förut sjönk den frusna stavelsen ~20 dB när rösten tystnade, eftersom kompensationen följde den tystnande ingången. Grind 1-exemplet `1d` låter därför starkare om det renderas om.*
- *`RVRender --test`: 41 kontroller, och alla går igenom. Nytt är:*
  - *latensen*
  - *SKIFT +7 st (−8 ct), OKTAV −12 st (+20 ct) och MONOTON till 220 Hz (0 ct)*
  - *fasen vid 0 st och formant utan tonändring*
  - *TALCHIP, RING och KORTVÅG (nivå och bandbredd)*
  - *körens bredd*
  - *ekots tid (375,8 ms för 1/8. i 120 BPM)*
  - *tystnad efter tystnad med alla moduler*
  - *inga klick när modulerna slås av och på*
  - *Omvandlingens nivå*
  - *mono*
  - *CPU för hela bordet med Kören MASSA: ~5 % av en kärna (Linux, molnet)*
- *`RVRender --projekt` och `--state` är nya, för testlåten.*
- *`RVSnapshot --klicktest`: 182 kontroller, 0 fel.*
- *pluginval nivå 5 (VST3, Linux, med editortesterna) går igenom två gånger av två.*
- *Testlåten `Reaper/Sandningen.RPP` byggs av `tools/reaper/gor_reaper_projekt.py` och har två spår:*
  - *Kroppsmaskinens Fabriksgolv (125 BPM, 112 takter, 3:35), kopierat ordagrant*
  - *"Rösten – Röstverket": 38 röstobjekt (talsyntes från `tools/reaper/gor_roster.sh`), ett MIDI-objekt med 96 ackordtoner till Orgeln, och 22 automationskurvor som byter moduler per del och drar Omvandlingen*
- *Låtens scener:*
  - *takt 9: kortvåg*
  - *takt 17: vokoder i c-moll*
  - *takt 33: djup röst med Strupen OKTAV och Kören DUBBEL*
  - *takt 41: talchippet räknar*
  - *takt 49: refrängen med vokoder, KÖR och Omvandlingen*
  - *takt 73: roboten med MONOTON och RING*
  - *takt 81: refrängen med MASSA*
  - *takt 97: en frusen stavelse spelad med ackord*
  - *takt 105: slut på sändningen*
- *Röstspåret ensamt är renderat i `lyssning/fas2/Sandningen_rosten.wav`.*

### Fas 3 – Panelen, appen och fabriksbandet (1.0)

- Hela panelen enligt stilprovet, skalning och animering av bandmätaren, ögat och VU-mätarna.
- Appen: mikrofon in (avstängd från början, mot rundgång), MIDI-klaviatur till Orgeln och inspelaren.
- 30 program i fem kategorier: Robot, Vokoder, Kör, Paroll och Radio. Programmen nivåanpassas med `RVRender --program`.
- Svensk manual med bilder, `Rostverket_i_Reaper.md` och pluginval nivå 5.

**Grind 3:** Alla parametrar nås utan den generiska vyn, och appen går att prata och spela in i utan DAW. Någon som inte byggt Röstverket gör rösterna till ett spår med bara fabriksbandet och manualen.

*Byggd, väntar på grind 3 (6 november 2026). Gjort och avvikelser:*

- *Fabriksbandet (`Source/presets/Fabriksbandet.h`) har 30 program: Init, och 29 i fem kategorier.*
  - *Robot (6), Vokoder (7), Kör (6), Paroll (6) och Radio (4).*
  - *Programmen läses och sparas av `krang/presets/Programbank.h`. Egna program sparas som `.rvprog` i `~/Library/Application Support/Rostverket/Program/`.*
  - *Ett program rör aldrig förbikopplingen eller MIDI in.*
  - *Porten är på i alla program utom Init.*
  - *Programmen med vokoder håller en ton på TON, så att de låter utan MIDI.*
- *Nivåerna är anpassade med Utnivå mot talsyntesen (`RVRender --program`, kalibrerat i tre varv). Alla 29 ligger inom ±0,2 dB från den torra rösten. Utnivån går från −2 till −13 dB: Porten och Kören höjer nivån, och utnivån tar ned den.*
- *Alla program är renderade i `renders/program/` (mp3).*
- *Panelen:*
  - *◀ ▶ bläddrar, klick på programfönstret ger listan per kategori och SPARA frågar efter ett namn.*
  - *Ett namn som ett fabriksprogram får "(eget)".*
  - *En stjärna efter namnet visar att programmet är ändrat. Stjärnan är ett nytt tecken i `krang/stil/Linjeskrift.h`; inga befintliga tecken ändrades. Det provades med Pannrummets stilprov, som är identiskt pixel för pixel före och efter (`PRSnapshot --stilprov`).*
  - *MENY har storleken och mappen med egna program.*
- *Appen (`Source/app/App.cpp`) är `krang::app::FristaendeApp`:*
  - *Mikrofonen är av från början, med JUCE:s tysta ingång som sparas mellan gångerna.*
  - *Under MENY finns mikrofonen, Ljud och MIDI (JUCE:s val av ljudkort och MIDI), inspelning av utgången till `~/Music/Röstverket/` (`krang/app/Inspelaren.h`) och Visa inspelningarna.*
  - *Programfönstret visar mikrofonen och inspelningstiden.*
  - *Appen är byggd på Linux men inte provkörd med ljudkort; det görs på Macen.*
- *Avvikelse: appens mikrofonbrytare går via två funktioner i processorn (`RostverketProcessor::app`), som `App.cpp` sätter. Panelen kan då inte inkludera JUCE:s fönsterklasser, som inte finns i plugin och verktyg.*
- *Rättning (6 november 2026): appen kraschade när den startades på Macen, eftersom den läser ljudkortets ingång utan att ha frågat om mikrofonen (`NSMicrophoneUsageDescription` saknades). CMakeLists.txt har nu `MICROPHONE_PERMISSION_ENABLED` med en text på svenska.*
- *Rättning (6 november 2026): samma fel som i Montagets app – `App.cpp` och `JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP` låg bara på `Rostverket_Standalone`. Nu ligger de på det delade målet, som i Pianot. Mikrofontillståndet är kvar, eftersom Röstverket behöver rösten.*
- *Manualen: `docs/Manual.md` och `docs/Rostverket_i_Reaper.md`.*
  - *Manualen har snabbstart, reglagen, varje modul med bild och tabell, Omvandlingen, utgången, MIDI och sidokedjan, appen, programmen, recept och felsökning.*
  - *Reaper-guiden har insert, MIDI till Orgeln, sidokedjan, automation, testlåten och rendering.*
  - *Bilderna görs av `RVSnapshot --manual docs` (panelen och nio utsnitt i 200 %).*
- *`RVRender --test`: 45 kontroller, och alla går igenom. Nytt är:*
  - *fabriksbandet: inget NaN, under taket och samlad nivå*
  - *att ett program inte rör förbikoppling och MIDI*
  - *att programmets namn följer med tillståndet*
- *`RVSnapshot --klicktest`: 186 kontroller, 0 fel. Nytt är ◀ ▶, programfönstret och stjärnan.*
- *pluginval nivå 5 (VST3, Linux, med editortesterna) går igenom två gånger av två.*
- *På Macen återstår att prova:*
  - *AU (auval), CLAP och appen med mikrofon, MIDI-klaviatur och inspelning*
  - *programbyten från Reaper*
  - *ritningstiden på M1*

### Fas 5 – Efter 1.0 (kandidater)

- **Talaren:** talsyntes från text med fonem och LPC – datorn som läser upp parollen.
- Vokodern i krang-libs, och Kroppsmaskinens *Ropet* byggt på den.
- 40 band och ett eget bandschema (fritt valda frekvenser).
- MIDI ut från röstens tonhöjd.

## 6. Risker

| Risk | Motåtgärd |
| --- | --- |
| Vokodern blir obegriplig | Detektor för tonlösa ljud, högpass in i bärvågen och A/B med espeak och en riktig röst vid grind 1. |
| PSOLA hackar eller låter metalliskt | YIN med hysteres och fönster som följer perioden. Lyssnas vid grind 2. MONOTON får låta maskin, men SKIFT ska vara ren upp till ±5 halvtoner. |
| Latensen stör när man sjunger live i appen | Den fasta latensen hålls under 20 ms. Ett läge med låg latens behövs inte nu (beslut 6 november 2026); kan bli en kandidat i fas 5. |
| Sidokedjan fungerar inte i AU (som i Pannrummet) | Provas i fas 1. Faller den kör AU med Orgeln och brusbärvåg, vilket skrivs i manualen. |
| Rundgång i appen | Mikrofonen är av från början, och det finns en varning i panelen när utgången är högtalare. |
| Omfånget växer | Listan "Inte i 1.0" gäller. Talsyntesen väntar till fas 5. |

## 7. Beslut

- **Fyra nya plugin planeras**, och röstplugin byggs först (beslut 6 november 2026).
- **Namnet är Röstverket, modell RV-83**, med världen **radiohuset** och koderna ovan (beslut 6 november 2026).
- **Inget läge med låg latens** för live i 1.0 (beslut 6 november 2026). Latensen är fast och rapporteras alltid.

Inga öppna frågor just nu.
