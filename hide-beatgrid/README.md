# Hide Beat Grid

Hides the beat rulers above and below the waveforms for beatmatching practice.
The waveform, BPM, cues and audio are unchanged; the ruler images are cleared.

EP147's core owns `gridMaker.makeGridImage` while Bright Grid/Day Mode is in use,
so this mod writes `settings/azmod-gridaa.off` and needs **one Reload** after
installation to claim the hook. Bright Grid and Day Mode cannot be used at the
same time.

## Test

`make test` clears a fake 1220x15 ARGB ruler and checks every row is zero.

## Install

Community Mods > Repositories > `dnlsndr/az-mods` > Hide Beat Grid. After install,
press Reload once. The Mod Menu row is **Hide Beat Grid**, on by default.
