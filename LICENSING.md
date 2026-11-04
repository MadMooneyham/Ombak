# Ombak licensing

Ombak is free software under the GNU General Public License, version 3 only (`GPL-3.0-only`, full text in
`LICENSE`). It is built on Penumbra (Lunar Eclipse Forge), a fork of X0X (charlesvestal), itself a fork of
Felucca, and keeps their licence. If you distribute Ombak, or firmware derived from it, you must give your
recipients its complete corresponding source under the same licence.

## Where the code comes from

| What | Origin | Licence |
| --- | --- | --- |
| Platform: `firmware/hal/`, `firmware/loader/`, `firmware/src/{libc,lcd,gfx,usb,storage,ota}.c`, `tools/` (build, package, install, upload) | [Felucca](https://github.com/hugelton/Felucca), Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments; `usb.c` and `storage.c` changed for X0X | GPL-3.0-only |
| `firmware/src/app/{panel,main_fm1}.c` | adapted from Felucca's `panel.c`, `main.c` / `audio.c` | GPL-3.0-only |
| The effects bus (`dsp/fxbus*`, `dsp/fxkit.h`) | ported from [9W9](https://github.com/athousanddetails/schwung-9W9) (athousanddetails), after [ER-99](https://github.com/matthewcieplak/er-99) (Matthew Cieplak) | GPL-3.0 |
| The master (`dsp/master*`), the platform glue | X0X | GPL-3.0-only |
| The second core's start-up and job hand-off (`firmware/hal/fm1_cpu1.*`) | X0X, after what [Melodee](https://github.com/keremimo/melodee) (keremimo)'s dual-core audio found, with [Jangada](https://github.com/zednaked/jangada) (zednaked)'s measurements and mailbox layout | GPL-3.0-only |
| The shimmer reverb (`dsp/spacevb*`), the tides and pan, the render's pattern, the UI's drawing, the simulator (`host/`) | [Penumbra](https://github.com/MadMooneyham/Penumbra) (Lunar Eclipse Forge) | GPL-3.0-only |
| Everything else in `firmware/src/{app,dsp}`, `prototype/`, `tests/` | Ombak (Lunar Eclipse Forge) | GPL-3.0-only |

## Felucca Assets

Felucca's non-GPL assets — the icon atlas, the panel photo and the Hügelton drum pack — are **not** part of this
fork and are not used by it. Felucca's section 7 additional permission concerns those assets only, so it is not
needed here.

## Third-party material

| What | Licence | Where |
| --- | --- | --- |
| Barlow Semi Condensed (The Barlow Project Authors), the default UI face | SIL OFL 1.1 | `assets/fonts/BarlowSemiCondensed-*.ttf`, `assets/fonts/Barlow-OFL.txt` |
| The Ongkara (ᬒᬁ) on HOME, as Noto Sans Balinese (Google, the Noto Project Authors) draws it: rendered once to `assets/ongkara.png` (the font itself is not in this tree) | SIL OFL 1.1 | `assets/ongkara.png`, `tools/gen_ongkara.py` |
| JieLi AC79 SDK: `uboot.boot`, `cfg_tool.bin`, `eq_cfg_hw.bin` are read from your SDK checkout at build time and placed in the package; no SDK files are in this tree | Apache-2.0 | <https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK> |

The tunings are measured sets, as the Scala scale archive (Manuel Op de Coul) transcribes them from Colin McPhee,
*Music in Bali* (1966), Surjodiningrat, Sudarjana & Susanto, *Tone measurements of outstanding Javanese gamelans*
(1972), and others (`firmware/src/app/gamelan.c` names each).

## Respect

Ombak is an instrument inspired by gamelan, not a gamelan. The Ongkara is a sacred syllable in Balinese
Hinduism; Ombak shows it with care, its light following the music, never pulsing to the beat.

## Trademarks

"Felucca" and "Hügelton Instruments" are names of Hügelton Instruments. "M-VAVE" and "FM-1" are trademarks of
their respective owners. Ombak is independent firmware; it is not affiliated with, endorsed by or supported by any
of them.

## Radio

Ombak, like Penumbra, X0X and Felucca, never enables the Bluetooth / Wi-Fi radio of the hardware.
