#!/bin/sh
# Gör rösterna till testlåten Sändningen med talsyntes (espeak-ng, svensk röst), 48 kHz stereo, utan tystnad
# i ändarna. Kräver espeak-ng och ffmpeg.   sh tools/reaper/gor_roster.sh Reaper/ljud
#
# Copyright (C) 2026 Röstverket-teamet. Licens: AGPLv3, se LICENSE.
set -e
ut="${1:-Reaper/ljud}"
mkdir -p "$ut"
while IFS='|' read -r fil text; do
    espeak-ng -v sv -s 130 -p 30 -w "$ut/_tmp.wav" "$text"
    ffmpeg -nostdin -loglevel error -y -i "$ut/_tmp.wav" \
        -af "silenceremove=start_periods=1:start_threshold=-50dB,areverse,silenceremove=start_periods=1:start_threshold=-50dB,areverse" \
        -ar 48000 -ac 2 -c:a pcm_s16le "$ut/$fil.wav"
done <<'TEXT'
radio|Här är Radio Röstverket.
arbeta|Arbeta. Lyda. Dansa.
maskinerna|Vi är maskinerna.
landet|Rösten går ut över hela landet.
ett|Ett.
tva|Två.
tre|Tre.
fyra|Fyra.
slut|Slut på sändningen.
TEXT
rm -f "$ut/_tmp.wav"
echo "Rösterna i $ut"
