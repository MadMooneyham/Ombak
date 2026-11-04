# Ombak: design brief

A gamelan instrument for the M-VAVE FM-1, by Lunar Eclipse Forge. Mostly Balinese, made for meditative
atmospheres. It is Lunar Eclipse Forge's second firmware, after Penumbra (a space-drone instrument,
`~/fm1/fm1-x0x`, https://github.com/MadMooneyham/Penumbra). Ombak follows Penumbra's philosophy:
**playable, not self-playing.** It holds a world around you, and you play in it.

*Ombak* is Balinese for "wave": the shimmer of two instruments tuned slightly apart, beating as one note.
It is what makes the sound.

## Decisions so far (2026-11-04)

- **Mostly Balinese, meditative.** Not gong kebyar's fireworks, but the quiet traditions:
  - **gendèr wayang**: four metallophones, interlocking, slendro, intimate. Probably the heart of it;
  - **semar pegulingan**: the soft, sweet court ensemble, in pelog;
  - **gamelan angklung**: four-note slendro, gentle;
  - **gong gede**: the old, slow ceremonial ensemble, for the big gongs and long cycles.
  Long gong cycles and tempo that breathes (from gong gede and the Javanese side) are welcome.
- **The gong cycle is switchable**, as Penumbra's drone is: the cycle keeps it (gong, kempur/kempul,
  kemong, kajar/kethuk at their places in the cycle) until you take it over by hand, and back.
- **Name:** Ombak.
- **Its own place:** this folder, its own GitHub repository (to be created by David, e.g.
  `MadMooneyham/Ombak`), its own Pages site, releases and hub listing.
- **First a browser prototype,** before any firmware. Sound and feel are the hard parts.

## The sketch to refine (from the first discussion)

- **Ombak itself:** each instrument a pair, *pengumbang* (lower) and *pengisep* (higher), tuned a set number
  of beats per second apart (not a set number of cents, as real sets are tuned). The rate is a knob, from a
  slow breath to a bright shimmer.
- **Tunings:** slendro (gendèr wayang, angklung) and pelog (semar pegulingan, its five-note modes, the
  *saih*); several sets, each from a real gamelan's measured tuning, so each has its own character.
- **The keys:** the white keys play the core melody (the *pokok*, Java's *balungan*), one note a beat, in the
  tuning: five notes an octave, so 16 keys reach about three octaves. The black keys: the gong-cycle
  instruments by hand, the drum's (kendang's) cues, perhaps tuning or mode changes.
- **The ensemble follows you:** each core note you play is what the elaborating parts head for. The
  interlocking pair (*kotekan*: *polos* on the beat, *sangsih* between, two parts making one fast line) and
  the gendèr's ornaments anticipate the next core note, as real elaboration does. Slow and soft here, a
  gentle ripple, not a blaze.
- **Knobs on HOME (first ideas):** density (how much the elaboration does), OMBAK (the shimmer's rate),
  the space (an open pavilion's resonance), the cycle's tempo and its breathing (*ngubeng* to *ngembat*:
  steady to swelling and slowing).
- **Cues:** speed up, slow down, and the ending, where the ensemble slows and closes on the gong.
- **A looper for the core melody:** record a melody of 8 or 16 beats and it becomes the cycle's melody, so
  you can play an elaborating part yourself over it.
- **The screen:** the gong cycle as a circle, with marks where the cycle's instruments fall and a light
  travelling round it.
- **The sound, all FM:** bars and keys are a few operators at inharmonic ratios; gongs need care, an
  inharmonic body plus the slow hum that swells after the stroke. Two cores leave room for a full ensemble.

## Respect

This is living music with deep cultural roots. The names, tunings and forms should be right. The README
should credit the tradition and point to real gamelan and the communities that play it, and present Ombak
as an instrument inspired by gamelan, not as one.

## The prototype (the first task)

A playable browser page, no firmware:

- the FM instruments: gendèr, gangsa/saron-like bars, bonang/reyong kettles, the gongs;
- the tunings, and ombak's beating at different rates;
- the core-melody-and-elaboration mechanic on a computer keyboard laid out like the FM-1's (16 white keys,
  11 black), with the gong cycle switchable between the cycle and the hand.

It sounds right before anything else is built.

## When the firmware starts: what comes from Penumbra

Penumbra (GPL-3.0-only, itself on Felucca and X0X) has a platform proven on the FM-1's JieLi AC791N.
Ombak takes that and writes its own sound and UI:

| Take (rename to Ombak's) | Penumbra's own: leave |
| --- | --- |
| `firmware/hal/`, `firmware/loader/` (update loader, rescue) | `firmware/src/dsp/` (the voices) |
| `firmware/src/` platform: usb.c (USB audio and MIDI, timed clock ticks), midi_uart.c, storage.c, ota.c, lcd.c, gfx.c, libc.c, announce.c (screen reader) | `firmware/src/seq/` (journeys, looper: maybe the looper, as a start) |
| `firmware/src/app/plat.h`, plat_fm1.c, main_fm1.c, panel.c, balance.h (two-core split) | `firmware/src/app/engine.c`, ui.c, settings.c (their patterns are worth copying, not their content) |
| `firmware/src/seq/clock.c` (MIDI clock both ways, song position) | `MANUAL.md`, cheat sheet content, presets |
| `build.sh`, tools/ (build.py, fm1pkg_make.py, fm1_install.py, fm1_rescue.*, gen_font.py, gen_words.py, gen_dsp_tables.py, make_pages.py, target_obj_check.sh, get_toolchain.sh, lz4blk.py) | tools/gen_card.py, gen_logo.py (Ombak's own look) |
| `host/` (the simulator and its .pen scripts), `tests/` runner and click_check.py | Penumbra's scenarios |
| `web/` installer, fm1ota.js, fm1pkg.js, emu/ (the browser emulator), .github/ (Pages with the release hand-over, issue templates) | `fmhub.json`'s text, Penumbra's words |

To change on the way: the firmware identity (Penumbra's is `FM-1_73XXYYZZ`; Ombak needs its own prefix,
not 73), the settings and save magics (`PNS1`, `PNJ1` ...), the names in build.py and the installer, the
screen reader's words URL. Keep LICENSING.md's credits (Felucca, X0X) and add Penumbra's.

## Working with David

- Commit with `git -c user.name=David -c user.email=david@lunareclipseforge.com`, never saving the config.
  Commit at tested steps; push only when asked.
- David flashes and tests on the FM-1, and publishes GitHub releases themselves (no API token here).
- Nothing is posted to thefirmwarehub.com without David's go-ahead.
- Never use or type passwords or credentials.
