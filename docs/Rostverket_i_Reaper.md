# Röstverket i Reaper

Hur Röstverket används i Reaper: som insert på ett röstspår, med MIDI till Orgeln, med sidokedjan, med automation – och testlåten *Sändningen*. Reglagen och modulerna beskrivs i [manualen](Manual.md).

**Använd VST3-versionen** (*VST3: Rostverket (Rostverket)*) eller CLAP. AU-versionen fungerar men har ingen sidokedja.

## Installation

```sh
./real.sh     # bygger VST3, AU och CLAP och kopierar dem till ~/Library/Audio/Plug-Ins
```

Starta om Reaper, eller *Options → Preferences → Plug-ins → VST → Re-scan*. Pluginet är inte signerat. Godkänn det i Systeminställningar om macOS frågar.

## Som insert

1. Lägg rösten på ett spår.
2. **FX** → *VST3: Rostverket*.
3. Välj ett program i programfönstret.

Latensen är fast, 25 ms, och Reaper kompenserar för den (*Options → Preferences → Audio → Buffering*, kompensationen är på från början). Röstspåret ligger då i fas med resten.

## MIDI till Orgeln

Vokodern behöver en bärvåg. Det enklaste är Orgeln, spelad från MIDI på samma spår:

1. Slå på **MIDI IN** nere till höger i Röstverket.
2. Lägg ett MIDI-objekt med ackord på röstspåret (*Insert → New MIDI item*). Det får överlappa rösten, och Reaper spelar båda.
3. Eller spela live: arma spåret och välj en MIDI-ingång (*Record: MIDI input*), med monitorn på.

Utan MIDI IN håller Orgeln tonen på tonskalan (TON).

## Sidokedjan (synten talar)

1. Röstverkets spår: **I/O** → kanaler **4** (*Track channels: 4*).
2. Syntens eller basens spår: **Route** → *Add new send* → röstspåret. Ställ sändningen till **Audio: 1/2 → 3/4**.
3. I Röstverket: Vokodern **PÅ**, **BÄRVÅG** på **SIDOKEDJA**, eller programmet *Basen talar*.

## Automation

Alla reglage är parametrar som kan automatiseras. Klicka på **Param** i FX-fönstret, eller vrid på ett reglage och välj *Show envelope*. Bra att automatisera:

- modulernas PÅ/AV per del i låten
- **Omvandlingen** – mer maskin i refrängen
- **FRYS** – frys en vokal och spela den som orgel

Ett programbyte från Reaper (*FX → Presets*) sätter alla parametrar utom PÅ LUFTEN och MIDI IN.

## Testlåten Sändningen

`Reaper/Sandningen.RPP` är grind 2-låten. Den har Kroppsmaskinens *Fabriksgolv* på ett spår och ett röstspår genom Röstverket:

| Takt | Rösten |
| --- | --- |
| 9 | Radio Röstverket – kortvåg och eko |
| 17 | Parollerna genom vokodern, Orgeln i c-moll |
| 33 | Den djupa rösten – Strupen en oktav ned, dubblad |
| 41 | Breaket – talchippet räknar |
| 49 | Refrängen – vokoder, kör och Omvandlingen |
| 73 | Avbrottet – roboten (MONOTON och ring) |
| 81 | Refrängen igen – massan och mest maskin |
| 97 | Utklingningen – stavelsen fryses och spelas |
| 105 | Slut på sändningen |

Rösterna är talsyntes. Låten byggs om med:

```sh
sh tools/reaper/gor_roster.sh Reaper/ljud
python3 tools/reaper/gor_reaper_projekt.py build/RVRender_artefacts/Release/RVRender ../kropps-maskinen/Reaper/Fabriksgolv.RPP Reaper
```

Röstspåret ensamt, utan Reaper: `RVRender --projekt Reaper/Sandningen.json lyssning/fas2/Sandningen_rosten.wav`.

## Rendering

*File → Render* som vanligt. Röstverket har en svans (ekot och rummet, upp till 8 s); ta med den med *Tail* i renderingen om rösten slutar i låtens sista takt.
