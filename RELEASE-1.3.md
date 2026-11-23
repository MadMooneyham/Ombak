# Ombak 1.3

A gamelan-inspired instrument for the M-VAVE FM-1, by Lunar Eclipse Forge. **1.3 plays itself.** Switch it on and
the ensemble is already playing, leading its own melody; touch a white key and it follows yours; rest, and it
carries on. And its instruments can now be played from a DAW, one channel each.

**Package:** `ombak-1.3.fwsc`, identity `FM-1_74010300`. Install it with the web installer in Chrome
(https://madmooneyham.github.io/Ombak/install/). It carries the update loader. **Coming from an earlier Ombak:**
everything you shaped, your loop, your MINE slots and your settings come over, as they were (your PLAY AT
POWER-ON setting too: turn it on in SYSTEM to have Ombak play as it starts).

Play it in the browser: https://madmooneyham.github.io/Ombak/emu/ · cheat sheet:
https://madmooneyham.github.io/Ombak/cheatsheet/ · manual: [MANUAL.md](MANUAL.md)

## New

**The ensemble leads when you rest**
- From the start, and whenever you have not played a white key for a whole cycle, the ensemble makes its own core
  melody: mostly by step, sometimes by a skip, now and then holding, landing on the first tone at every gong and on
  the third at the half, so the kotekan, reyong and gongs have a melody to weave around.
- Your next white key takes the lead at once. HOME's bottom line says who leads: IT LEADS, or YOU LEAD.
- GAMELAN's new **THE MELODY** row makes the melody yours alone, as before.

**It plays at power-on**
- As made, Ombak now starts playing as it switches on, on GILAK. (An FM-1 that already has Ombak keeps its own
  PLAY AT POWER-ON setting.)

**A sound module** (SYSTEM: MIDI MODE: SOUND MODULE)
- Each instrument on its own MIDI channel, from MIDI CHANNEL up: gendèr, gangsa, jublag, jegogan, reyong, the
  gongs, the drums, the suling and the selonding's iron keys. Velocity is the stroke, a note off damps it, the
  pedal lets it ring; the cycle need not run. The table is in the manual's MIDI section.

GPL-3.0-only. Built on Penumbra, X0X (Charles Vestal) and Felucca (Leo Kuroshita, Hügelton Instruments); the
Ongkara from Noto Sans Balinese (SIL OFL). See LICENSING.md.
