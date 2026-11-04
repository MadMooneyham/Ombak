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

## The prototype: what it settled (2026-11-04)

`prototype/index.html`: one page, Web Audio, no build step. Open it in a browser and play. These are the
prototype's answers, for David to listen to and overrule; the firmware follows what survives.

- **The keys,** as Penumbra's emulator maps them: `Q`…`I`, `A`…`K` are the 16 white keys, `1`…`0`, `-` the
  11 black ones, `Space` starts and stops.
  - **White keys** play your gendèr at once (the heart: it is always the gendèr under your hands) and set the
    **next core note**. The core (jublag, with the jegogan an octave down on every other beat) strikes it at the
    next beat, and the elaboration spends the beat turning toward it. Pressed again before the beat, the last
    key wins. No new note: the core holds and the parts circle the note they have.
  - **Black keys:** gong (wadon), gong lanang, kempur, kemong, kajar by hand; then the cues faster, slower,
    ending; then cycle ⇄ hand, next mode (or next tuning, for sets without modes), loop.
- **Ombak** is a pair per stroke, pengumbang at f − b/2 and pengisep at f + b/2, panned a little apart, with
  b the same number of beats a second across the range. The knob runs 0.25–10 Hz (slow breath, gentle wave,
  shimmer, bright shimmer). The gongs beat slower than the bars (a fraction of b), so their hum breathes.
  The rate is set at each stroke, so a turn of the knob is heard on the next notes, not on ringing ones.
- **The FM voices:** sine carriers at inharmonic ratios, some with a sine modulator whose index falls fast
  (the mallet's brightness), plus a band of noise for the mallet. Gendèr: 1, 2.76, 5.40 and a slow-blooming
  fundamental for the bamboo resonator. Gangsa: brighter, shorter, damped as the next note is struck.
  Reyong: 1, 2.37, 3.94, 5.6, short. Gongs: 1, 2.03, 2.86, 3.97, 5.42 with a slight downward glide, and the
  hum: a sine at the fundamental that swells for about a second after the stroke and fades over 14 s
  (kempur and kemong smaller and quicker). These are the starting tables for `dsp/`.
- **Elaboration** (it only ever knows the next core note, which is all a player gives it):
  - *kotekan*: polos on the beat, sangsih between, panned left and right; together one line of 4 or 8
    notes a beat (density). The first half circles the core note just struck, the second half circles the
    next one, approaching from below when the melody rises and from above when it falls, landing on it at
    the beat. On gendèr in gendèr wayang, on gangsa elsewhere.
  - *reyong*: half as fast, norot-like: the note and its upper neighbour, switching to the next note
    halfway through the beat (the anticipation is audible here).
  - *gendèr ornaments* (semar pegulingan): the next note as a pickup just before the beat, the core note in
    octaves on it.
  - Slow and soft: the parts sit under the gendèr, and the elaboration swells mid-cycle with the breath.
- **The gong cycle:** 8, 16 or 32 beats, the gong at the top of the circle (the end of one cycle, the start
  of the next), the kemong at half, the kempur at the quarters, the kajar on every beat. These placements
  are a sketch in the spirit of Balinese forms, not specific pieces; real forms (gilak, lelambatan's long
  cycles, the batel that gendèr wayang adds) should replace them. "The cycle keeps it" plays them; "by hand"
  leaves them to the black keys and dims the marks, while the beat runs on.
- **Tempo:** the core-beat rate (18–96 a minute, 32–48 for these meditative presets), gliding toward its
  target; faster and slower move the target by a third. **Breath** (0 = steady) slows the last beats into the
  gong and swells the dynamics mid-cycle. **Ending** slows each beat, thins the elaboration and closes on the
  next gong at least four beats away, then stops.
- **The looper** works as sketched: armed, it records the next 8 (or 16) core beats from the cycle's start,
  then plays them as the core while your keys play the gendèr freely over it.
- **Ensembles** are starting points (tuning, cycle, parts, ombak, tempo), all adjustable after: gendèr wayang
  (Peliatan slendro, gendèr for core and kotekan, the cycle by hand, since gendèr wayang has no gongs),
  semar pegulingan (saih pitu, gangsa kotekan, gendèr ornaments), angklung (four-tone slendro, with reyong),
  gong gede (five-tone pelog, 32 beats, slow, breathing, sparse).
- **Tunings,** all from measured sets via the Scala archive (sources on the page):
  - slendro: gendèr wayang of Peliatan and of Ubud; low gendèr, Tabanan (McPhee 1966); angklung, Sangsit
    (five tones) and Mas (four, McPhee); Kyai Kanyut Mesem, Surakarta (Surjodiningrat et al. 1972);
  - pelog: semar pegulingan saih pitu, Tampak Gangsul (McPhee) and Ksatria, Denpasar, with the modes selisir,
    tembung, sunaren and baro; gamelan gong, Peliatan (five-tone selisir, McPhee); Kyai Kanyut Mesem pelog
    with pathet nem and barang.
  - White key 1 is always the mode's first tone (ding), placed between 130 and 260 Hz (105–210 for four
    tones), so 16 keys climb three octaves and a tone (four octaves with four tones).
- **Still to check** (respect: names and forms right):
  - which measured tone is *ding* in the slendro sets (the prototype assumes the lowest listed);
  - the Scala file's "Tampak Gangsai" read as Tampak Gangsul, Denpasar;
  - *ngubeng* / *ngembat* for the breathing knob: *ngubeng* is usually a static figuration around one tone,
    not a tempo, so the prototype says "breath" until the right words are found;
  - baro has several versions; the page uses one common one (3 4 5 7 1).
- **Penumbra's movement, brought over (2026-11-04):**
  - *The space* is Penumbra's reverb (`spacevb.c`), ported sample by sample: eight lines, damped, through a
    Hadamard; the first four move (MOTION); the shimmer is two grains reading an octave up (or a fifth, a
    twelfth), low-passed at 4.5 kHz and fed back through tanh, kept under unity as Penumbra keeps it. The
    120 Hz low cut keeps the gongs' hum dry. The octave fits any tuning; a fifth or a twelfth (3:2, 3:1) is
    not in slendro or pelog, so it colours the set. The SPACE knob is the send.
  - *TIDES:* one slow swell (8 s to 4 min, EVEN to WANDERING), as Penumbra's. Here it reaches the space (the
    send), the mallets' brightness (the FM index at each stroke), the ensemble's swell, and the ombak's own
    rate: the wave breathes on the tide.
  - *PAN and PAN 2:* each part on its own bus (your gendèr, core, kotekan, reyong, gongs) with a place and a
    sway, on the tide or a rate of its own (60 s to 0.5 s), with OFFSET (in sync to mirrored). As made, your
    gendèr sways 25% on the tide, the core sits a little left and the reyong right; polos and sangsih keep
    their own spread inside the kotekan's place.
  - Web Audio cannot build this reverb from its own nodes: Chrome's feedback between two or more delay nodes
    gains energy (even two cross-coupled lines at 0.9 run away). So the network runs in an AudioWorklet where
    the page is a secure context (localhost, and file:// in Chrome), else in a ScriptProcessor. In the
    firmware, `spacevb.c` itself comes over, with its tables, and the tide and pan code from `engine.c`.
- **HOME is the Ongkara (decided 2026-11-04),** on the FM-1's 240×240 panel (Penumbra's ST7789).
  - *The Ongkara* (ᬒᬁ, Om in Balinese script, not the Devanagari ॐ), set in Noto Sans Balinese (OFL; the
    firmware rasterises it with gen_font.py), sized to sit inside its disc and centred on its own outline.
    David was assured its use is fine so long as Ombak is reasonably respectful: it may pulse and glow
    intensely, but never to the beat like a cheap rave. So its light follows the music's swell, not the
    tempo: the ensemble's loudness in dB (in over 0.4 s, out over 1.5 s) against where it settled over the
    last few seconds (so it answers the music growing and falling), the gong's flare and hum, and the tide.
    Past the middle of the swell it burns white-hot from within; the ombak only shimmers in its outer halo.
    It blooms as the music starts from silence. The mapping is a first guess, to be tuned by eye.
  - *The gong* stays in the prototype as the alternative: the gong ageng face on, the cycle round its rim. Its boss (*pencon*) is the
    sound: it flares at the stroke, swells and fades with the hum (the gong voice's own envelope), and beats
    at the ombak rate; a halo behind it widens and narrows with the tide. Each core note lights a bead at
    its place on the rim, fading over a cycle, and a faint ripple runs ahead of the light for the kotekan.
  - Under either (the Ongkara's rim carries the same cycle, beads and light): the mode and tuning, PLAYING / STOPPED with CYCLE or BY HAND and the loop's state, and
    HOME's four knobs as a first guess: OMBAK, DENSITY, SPACE, BREATH (SELECT: mode or tuning). The header
    carries the name and the tempo, as Penumbra's.
- **Next for the prototype,** if David wants: the kendang's cues as sound (a stroke pattern before each
  change), real colotomic forms, a "free" mode with no beat, and the ombak knob reaching ringing notes.

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
