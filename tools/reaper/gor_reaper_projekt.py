#!/usr/bin/env python3
# Bygger grind 2-testlåten "Sändningen" som Reaper-projekt: Kroppsmaskinens Fabriksgolv på ett spår och ett
# röstspår med Röstverket – paroller, en refräng genom vokodern med Orgeln spelad från MIDI, och alla sex
# moduler i tur och ordning, med Omvandlingen som automation.
#
#   python3 tools/reaper/gor_reaper_projekt.py <RVRender> <Fabriksgolv.RPP> <utkatalog> [--reaper-id <tal>]
#
# <Fabriksgolv.RPP> är Kroppsmaskinens testlåt (kropps-maskinen/Reaper/Fabriksgolv.RPP); dess spår kopieras
# ordagrant, med läge, automation och transponering. Rösterna ligger i <utkatalog>/ljud/ (talsyntes, gjord av
# tools/reaper/gor_roster.sh). Röstverkets parametrar läses med "RVRender --parametrar" och läget skrivs med
# "RVRender --state". Skriptet skriver också Sandningen.json, så att röstspåret kan renderas utan Reaper:
#
#   RVRender --projekt Reaper/Sandningen.json Reaper/Sandningen_rosten.wav
#
# Röstverkets Reaper-id läses ur Reapers pluginlista (~/Library/Application Support/REAPER/
# reaper-vst*plugins*.ini) när skriptet körs på Macen; annars --reaper-id eller ett reservvärde ('Rvrk').
# Reaper hittar ändå pluginet på filnamnet Rostverket.vst3.
#
# Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
import base64, glob, json, os, re, struct, subprocess, sys, tempfile, uuid, wave

PPQ = 960
RV_GUID = 'ABCDEF019182FAEB5276726B52763833'   # JUCE: ABCDEF01 9182FAEB + 'Rvrk' + 'Rv83'
RV_RESERV_ID = 0x5276726B                        # 'Rvrk'
# JUCE:s privata data efter tillståndet i VST3 (samma som Pannrummets och Kroppsmaskinens skript).
JUCE_PRIVAT = bytes.fromhex('0000000000000000004a554345507269766174654461746100010142797061737300010103001d00000000000000'
                            '4a5543455072697661746544617461')

# --- Låten --------------------------------------------------------------------------------------------
# Fabriksgolvets delar: A 1 intro, B 9 bas, C 17 bas 2 och stötar, D 33 mer rörelse, E 41 break,
# F 49 fullt, G 73 avbrott, H 81 fullt med mest spänning, I 97 utklingning. Takter räknas från 1.

LAGE = {   # Röstverket från början: Porten på, MIDI in på (Orgeln spelas av MIDI-objektet), limitern på
    'rv_port_pa': 1, 'rv_port_troskel': -50, 'rv_port_kompression': 0.4, 'rv_port_narhet': 0.3,
    'rv_midi': 1, 'rv_ut_limiter': 1,
    'rv_vok_band': 3, 'rv_vok_snabbhet': 0.7, 'rv_vok_begriplighet': 0.6, 'rv_org_vag': 0, 'rv_org_brus': 0.08,
    'rv_eko_tid': 3, 'rv_eko_aterkoppling': 35, 'rv_eko_morker': 0.6, 'rv_eko_rum': 0.25, 'rv_eko_blandning': 0.2,
    'rv_kor_spridning': 0.5, 'rv_kor_bredd': 0.8, 'rv_kor_blandning': 0.5,
}

# Scenerna: vid takten sätts värdena (stegvis automation). Bråktal = takt + del av takt.
SCENER = [
    (9,  'Radio Röstverket – kortvåg och eko',
         {'rv_sand_pa': 1, 'rv_sand_lage': 2, 'rv_sand_styrka': 0.7, 'rv_sand_ton': 0.4, 'rv_sand_blandning': 1,
          'rv_eko_pa': 1, 'rv_eko_tid': 4, 'rv_eko_aterkoppling': 40, 'rv_eko_blandning': 0.3}),
    (17, 'Parollerna genom vokodern, Orgeln i c-moll',
         {'rv_sand_pa': 0, 'rv_vok_pa': 1, 'rv_vok_blandning': 0.9, 'rv_eko_tid': 3, 'rv_eko_blandning': 0.2}),
    (33, 'Den djupa rösten – Strupen en oktav ned, dubblad',
         {'rv_vok_pa': 0, 'rv_strup_pa': 1, 'rv_strup_lage': 1, 'rv_strup_tonhojd': -12, 'rv_strup_blandning': 0.5,
          'rv_kor_pa': 1, 'rv_kor_lage': 0}),
    (41, 'Breaket – talchippet räknar, ekot i åttondelar',
         {'rv_strup_pa': 0, 'rv_kor_pa': 0, 'rv_sand_pa': 1, 'rv_sand_lage': 0, 'rv_sand_frekvens': 6000, 'rv_sand_styrka': 0.8,
          'rv_eko_tid': 2, 'rv_eko_aterkoppling': 55, 'rv_eko_blandning': 0.35}),
    (49, 'Refrängen – vokoder, kör och Omvandlingen',
         {'rv_sand_pa': 0, 'rv_vok_pa': 1, 'rv_kor_pa': 1, 'rv_kor_lage': 1, 'rv_omvandlingen': 0.3,
          'rv_eko_tid': 3, 'rv_eko_aterkoppling': 35, 'rv_eko_blandning': 0.25}),
    (61, 'Refrängen drar mot maskin', {'rv_omvandlingen': 0.6}),
    (73, 'Avbrottet – roboten: MONOTON och ringmodulator',
         {'rv_vok_pa': 0, 'rv_kor_pa': 0, 'rv_omvandlingen': 0.0, 'rv_strup_pa': 1, 'rv_strup_lage': 2, 'rv_strup_ton': 43,
          'rv_strup_foljsamhet': 1, 'rv_strup_blandning': 1, 'rv_sand_pa': 1, 'rv_sand_lage': 1, 'rv_sand_frekvens': 220,
          'rv_sand_styrka': 0.6, 'rv_sand_blandning': 0.7}),
    (81, 'Refrängen igen – massan och mest maskin',
         {'rv_strup_pa': 0, 'rv_sand_pa': 0, 'rv_vok_pa': 1, 'rv_kor_pa': 1, 'rv_kor_lage': 2, 'rv_omvandlingen': 0.8}),
    (97, 'Utklingningen – stavelsen fryses och spelas', {'rv_kor_lage': 1, 'rv_omvandlingen': 0.4}),
    ('frys', '', {'rv_vok_frys': 1}),   # tiden räknas ut: där 'maskinerna' i takt 97 är starkast
    (105, 'Slut på sändningen – kortvåg',
         {'rv_vok_pa': 0, 'rv_vok_frys': 0, 'rv_sand_pa': 1, 'rv_sand_lage': 2, 'rv_sand_styrka': 0.7, 'rv_sand_ton': 0.4,
          'rv_eko_tid': 4, 'rv_eko_aterkoppling': 45, 'rv_eko_blandning': 0.35}),
]

# Rösterna: (takt, fil). Refrängen: "Vi är maskinerna" varannan takt.
ROSTER = [(9, 'radio.wav'), (13, 'radio.wav'), (17, 'arbeta.wav'), (21, 'arbeta.wav'),
          (25, 'maskinerna.wav'), (27, 'maskinerna.wav'), (29, 'maskinerna.wav'), (31, 'maskinerna.wav'),
          (33, 'landet.wav'), (37, 'landet.wav'), (41, 'ett.wav'), (43, 'tva.wav'), (45, 'tre.wav'), (47, 'fyra.wav'),
          (73, 'arbeta.wav'), (77, 'arbeta.wav'), (97, 'maskinerna.wav'), (105, 'slut.wav')]
ROSTER += [(t, 'maskinerna.wav') for t in range(49, 73, 2)] + [(t, 'maskinerna.wav') for t in range(81, 97, 2)]

# Orgeln: ackorden från MIDI. (första takt, sista takt + 1, takter per ackord, följd)
ACKORD = {'Cm': [48, 55, 60, 63], 'Ab': [44, 51, 56, 60], 'Bb': [46, 53, 58, 62], 'G': [43, 50, 55, 59]}
ORGELN = [(17, 25, 8, ['Cm']), (25, 33, 8, ['Ab']), (49, 73, 2, ['Cm', 'Ab', 'Bb', 'G']),
          (81, 97, 2, ['Cm', 'Ab', 'Bb', 'G']), (97, 105, 4, ['Cm', 'Ab'])]

# --- Reaper-formatet ----------------------------------------------------------------------------------

def guid():
    return '{' + str(uuid.uuid4()).upper() + '}'

def reaper_id():
    if '--reaper-id' in sys.argv:
        return int(sys.argv[sys.argv.index('--reaper-id') + 1])
    for ini in glob.glob(os.path.expanduser('~/Library/Application Support/REAPER/reaper-vst*plugins*.ini')):
        for rad in open(ini, encoding='utf-8', errors='replace'):
            if rad.lower().startswith('rostverket.vst3='):
                falt = rad.split('=', 1)[1].split(',')
                if len(falt) > 1 and '{' in falt[1]:
                    return int(falt[1].split('{')[0])
    print('Hittade inte Röstverket i Reapers pluginlista; använder reservvärdet %d.' % RV_RESERV_ID)
    return RV_RESERV_ID

def vst3_block(state_xml, program, rid, ingangar, utgangar):
    """Reapers VST3-block: huvud (id, magi, stiften), JUCE:s tillstånd ('VC2!' + längd + XML + privat data)
    och programnamnet. Samma form som Pannrummets skript."""
    xml = state_xml.encode('utf-8')
    komponent = b'VC2!' + struct.pack('<I', len(xml)) + xml + JUCE_PRIVAT
    kropp = struct.pack('<II', len(komponent), 1) + komponent + bytes(8)
    huvud = struct.pack('<II', rid, 4276969198)
    for stift in (ingangar, utgangar):
        huvud += struct.pack('<I', len(stift))
        for m in stift:
            huvud += struct.pack('<Q', m)
    huvud += struct.pack('<III', len(kropp), 1, 0)
    namn = b'\x00' + program.encode('utf-8') + bytes(5)
    rader = [base64.b64encode(huvud).decode()]
    for i in range(0, len(kropp), 96):
        rader.append(base64.b64encode(kropp[i:i + 96]).decode())
    rader.append(base64.b64encode(namn).decode())
    return rader

def parmenv(index, punkter):
    """punkter: [(sekunder, normaliserat värde)]. Fyrkantiga steg (form 1)."""
    ut = ['      <PARMENV %d 0.000000 1.000000 %.6f' % (index, punkter[0][1]),
          '        EGUID %s' % guid(), '        ACT 1 -1', '        VIS 1 1 1', '        LANEHEIGHT 0 0',
          '        ARM 0', '        DEFSHAPE 1 -1 -1']
    ut += ['        PT %.6f %.6f 1' % (s, v) for s, v in punkter]
    return ut + ['      >']

def midi_handelser(noter, langd):
    """noter: [(start i fjärdedelar, längd i fjärdedelar, ton, anslag 0..1)]"""
    h = []
    for start, l, ton, vel in noter:
        pa = int(round(start * PPQ)); av = int(round((start + l) * PPQ)) - 1
        h.append((pa, 1, 0x90, ton, max(1, min(127, int(round(vel * 127))))))
        h.append((max(pa + 1, av), 0, 0x80, ton, 0))
    h.sort()
    ut, senast = [], 0
    for t, _, status, d1, d2 in h:
        ut.append('E %d %02x %02x %02x' % (t - senast, status, d1, d2))
        senast = t
    ut.append('E %d b0 7b 00' % max(0, int(langd * PPQ) - senast))
    return ut

# --- Bygget -------------------------------------------------------------------------------------------

def las_km(rpp_fil):
    text = open(rpp_fil, encoding='utf-8').read()
    tempo = float(re.search(r'^\s*TEMPO ([\d.]+)', text, re.M).group(1))
    langd = float(re.search(r'^\s*SELECTION 0 ([\d.]+)', text, re.M).group(1))
    rader = text.split('\n')
    start = next(i for i, r in enumerate(rader) if r.startswith('  <TRACK'))
    slut = next(i for i in range(start + 1, len(rader)) if rader[i] == '  >')
    spar = rader[start:slut + 1]
    spar[0] = '  <TRACK %s' % guid()
    return tempo, langd, spar

def main():
    if len(sys.argv) < 4:
        print('gor_reaper_projekt.py <RVRender> <Fabriksgolv.RPP> <utkatalog> [--reaper-id <tal>]')
        sys.exit(1)
    rvrender, km_rpp, utkatalog = os.path.abspath(sys.argv[1]), sys.argv[2], sys.argv[3]
    os.makedirs(utkatalog, exist_ok=True)
    tempo, langd, km_spar = las_km(km_rpp)
    rid = reaper_id()
    sek_per_takt = 4 * 60.0 / tempo
    tid = lambda takt: (takt - 1) * sek_per_takt

    with tempfile.TemporaryDirectory() as tmp:
        par = os.path.join(tmp, 'p.txt')
        subprocess.run([rvrender, '--parametrar', par], check=True, stdout=subprocess.DEVNULL)
        index, intervall, grundvarde = {}, {}, {}
        for rad in open(par, encoding='utf-8'):
            falt = rad.rstrip('\n').split('\t')
            index[falt[1]] = int(falt[0]); intervall[falt[1]] = (float(falt[3]), float(falt[4]))
            grundvarde[falt[1]] = float(falt[5])
        norm = lambda pid, v: (v - intervall[pid][0]) / (intervall[pid][1] - intervall[pid][0])
        xml_fil = os.path.join(tmp, 'state.xml')
        subprocess.run([rvrender, '--state', xml_fil] + ['%s=%s' % kv for kv in LAGE.items()], check=True)
        state = open(xml_fil, encoding='utf-8').read().strip()

    # FRYS där rösten i takt 97 är starkast (20 ms-fönster inom första sekunden), så att en vokal fryses.
    with wave.open(os.path.join(utkatalog, 'ljud', 'maskinerna.wav')) as w:
        fs_r, data = w.getframerate(), w.readframes(w.getnframes())
        prov = struct.unpack('<%dh' % (len(data) // 2), data)[::w.getnchannels()]
    f = int(0.02 * fs_r)
    energi = [sum(x * x for x in prov[i:i + f]) for i in range(0, min(len(prov), fs_r) - f, f // 2)]
    frys = 97 + (energi.index(max(energi)) * (f // 2) + f) / fs_r / sek_per_takt
    scener = [(frys if t == 'frys' else t, namn, v) for t, namn, v in SCENER]

    # Automationen: varje parameter som någon scen nämner, med läget (LAGE, annars grundvärdet) från början.
    kurvor = {}
    for takt, _, varden in scener:
        for pid, v in varden.items():
            kurvor.setdefault(pid, []).append((tid(takt), float(v)))
    for pid, pts in kurvor.items():
        pts.insert(0, (0.0, float(LAGE.get(pid, grundvarde[pid]))))

    # MIDI till Orgeln
    noter = []
    for forst, slut, per, folj in ORGELN:
        for n, t in enumerate(range(forst, slut, per)):
            for ton in ACKORD[folj[n % len(folj)]]:
                noter.append(((t - 1) * 4.0, per * 4.0 - 0.05, ton, 0.8))

    # Röstspåret i Reaper
    fx = ['      <VST "VST3: Rostverket (Rostverket)" Rostverket.vst3 0 "" %d{%s} ""' % (rid, RV_GUID)]
    fx += ['        ' + r for r in vst3_block(state, 'Sändningen', rid, [1, 2, 0, 0], [1, 2])]
    fx += ['      >', '      FLOATPOS 0 0 0 0', '      FXID %s' % guid()]
    for pid in sorted(kurvor):
        fx += parmenv(index[pid], [(s, norm(pid, v)) for s, v in kurvor[pid]])
    fx += ['      WAK 0 0']

    objekt = []
    for takt, fil in sorted(ROSTER):
        with wave.open(os.path.join(utkatalog, 'ljud', fil)) as w:
            l = w.getnframes() / w.getframerate()
        objekt += ['    <ITEM', '      POSITION %.6f' % tid(takt), '      LENGTH %.6f' % l, '      LOOP 0',
                   '      NAME "%s"' % fil[:-4], '      IGUID %s' % guid(),
                   '      <SOURCE WAVE', '        FILE "ljud/%s"' % fil, '      >', '    >']
    slag = langd * tempo / 60.0
    objekt += ['    <ITEM', '      POSITION 0', '      LENGTH %.6f' % langd, '      LOOP 0', '      NAME "Orgeln"',
               '      IGUID %s' % guid(), '      <SOURCE MIDI', '        HASDATA 1 %d QN' % PPQ]
    objekt += ['        ' + e for e in midi_handelser(noter, slag)]
    objekt += ['        GUID %s' % guid(), '      >', '    >']

    rost = ['  <TRACK %s' % guid(), '    NAME "Rösten – Röstverket"', '    VOLPAN 1 0 -1 -1 1', '    MUTESOLO 0 0 0',
            '    NCHAN 4', '    FX 1', '    MAINSEND 1 0',
            '    <FXCHAIN', '      SHOW 0', '      LASTSEL 0', '      DOCKED 0', '      BYPASS 0 0 0']
    rost += fx + ['    >'] + objekt + ['  >']

    ut = ['<REAPER_PROJECT 0.1 "7.0/macOS-arm64" 0 0',
          '  <NOTES 0 2', '    |Sändningen – Röstverkets grind 2-låt, byggd av rostverket/tools/reaper/gor_reaper_projekt.py',
          '    |Kroppsmaskinens Fabriksgolv och ett röstspår (talsyntes) genom Röstverket. Orgeln spelas av MIDI-objektet',
          '    |på röstspåret; modulerna och Omvandlingen är automation på Röstverket.']
    for takt, namn, _ in SCENER:
        if namn:
            ut.append('    |Takt %g: %s' % (takt, namn))
    ut += ['  >', '  TEMPO %g 4 4 0' % tempo, '  SAMPLERATE 48000 0 0', '  LOOP 0',
           '  MASTER_VOLUME 1 0 -1 -1 1', '  SELECTION 0 %g' % langd, '  CURSOR 0']
    ut += km_spar + rost + ['>']
    with open(os.path.join(utkatalog, 'Sandningen.RPP'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(ut) + '\n')

    # Samma låt för RVRender --projekt (röstspåret utan Reaper)
    projekt = {'tempo': tempo, 'langd': langd + 4.0, 'lage': LAGE,
               'automation': {pid: [[s, v] for s, v in pts] for pid, pts in kurvor.items()},
               'objekt': [{'fil': 'ljud/' + fil, 'start': tid(takt)} for takt, fil in sorted(ROSTER)],
               'noter': [[s * 60.0 / tempo, l * 60.0 / tempo, ton, vel] for s, l, ton, vel in noter]}
    with open(os.path.join(utkatalog, 'Sandningen.json'), 'w', encoding='utf-8') as f:
        json.dump(projekt, f, ensure_ascii=False, indent=1)
    print('%s: %g takter i %g BPM (%d:%02d), 2 spår, %d röstobjekt, %d toner, %d automationskurvor' % (
        os.path.join(utkatalog, 'Sandningen.RPP'), slag / 4, tempo, int(langd // 60), int(round(langd % 60)),
        len(ROSTER), len(noter), len(kurvor)))

if __name__ == '__main__':
    main()
