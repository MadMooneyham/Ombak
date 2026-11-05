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
    ending; then the kotekan, reyong and ornaments in and out (the first prototype had cycle ⇄ hand, next mode
    and loop here: they moved to ARP, SELECT and REC/PLAY, see the interface below).
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
  - ~~which measured tone is *ding* in the slendro sets~~ (0.1.5): the gendèr wayang's lowest key is *dong*,
    sung dong, deng, dung, dang, ding (*Gamelan Gender Wayang of Bali: Form and Style*, Wesleyan), so the three
    gendèr sets name their lowest measured tone dong; the pitches are unchanged. Angklung Sangsit's five tones
    keep ding first: no source found yet for its order;
  - ~~the Scala file's "Tampak Gangsai"~~ (0.1.5): Banjar Tampakgangsul, Dangin Puri Kauh, Denpasar Utara, had a
    sekaa semar pagulingan until 1940, within McPhee's years in Bali (1931–38); its gamelan went to the new
    sekaa of Pagan Klod (Denpasar's culture site, culture.denpasarkota.go.id). The reading stands;
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
  change), real colotomic forms (0.1.4: gilak, lancaran, ketawang, ladrang), a "free" mode with no beat, and the ombak knob reaching ringing notes.

## Performance and the interface (first layout, 2026-11-04)

Built on Penumbra's controls and conventions, so the two feel like one family: the same keys, encoders and
buttons, HOME after 30 seconds untouched, ALGO as the tempo and as each SHAPE page's level, FX held for throws,
any button held to say what it does. The prototype's screen panel mocks all of it (the buttons and encoders
under it, and on the keyboard as Penumbra's emulator has them).

### The workflows

- **Sit down and play.** Switch on: HOME, the Ongkara, stopped. The white keys already play the gendèr and
  the black keys the gongs, so you can find the tuning before anything moves. **SEQ** starts the cycle: the
  gong opens it (when the cycle keeps the gongs) and the light sets off round the rim. SEQ again stops it, and
  everything rings out.
- **Lead the melody.** Each white key sounds your gendèr at once and becomes the next core note; the core
  lands on it at the next beat and the elaboration spends the beat turning toward it. Play nothing and the
  parts circle the note they have. **OCT−/OCT+** move the keys an octave either way.
- **Let it grow and settle.** HOME's knobs are the performance: **OMBAK** (the shimmer's rate), **DENSITY**
  (from the core alone to a flowing kotekan), **SPACE**, **BREATH** (steady to swelling and slowing into the
  gong). **ALGO** is the tempo. The black keys bring the elaborating parts in and out (9 kotekan, 10 reyong,
  11 ornaments), each at once, so a piece can begin with the core and the gongs and open out.
- **Change place at the gong.** On HOME, **SELECT** walks the modes (selisir, tembung, sunaren, baro; pathet
  nem, barang); **PRESETS** walks the gamelan (gendèr wayang, semar pegulingan, angklung, STILL, gong gede). Either
  arrives with the next gong, as a real ensemble would change, and HOME shows what is coming (`SELISIR →
  TEMBUNG`). Stopped, they change at once.
- **Take the gongs.** **ARP** switches the gongs between the cycle and your hand (lit: the cycle keeps them).
  By hand, black 1-5 strike gong, gong lanang, kempur, kemong and kajar, and the rim's marks dim. The beat runs
  on either way.
- **Cue the ensemble,** as the kendang would: black 6 faster, 7 slower, 8 the ending (slowing, thinning,
  closing on the next gong at least four beats away).
- **Loop the core.** **REC** records the core over the next cycle from its start (8 or 16 beats; REC again
  cancels); then it plays as the core and your keys play the gendèr freely over it. **PLAY** holds the loop
  (you lead again) and brings it back; **PLAY held** clears it. REC while it plays records it anew.
- **Shape the sound.** **ENV** opens SHAPE, a page at a time (ENV again or SELECT for the next); **FX** the
  space; **EDIT** the gamelan playing; **LFO** the library; **GLO** the system. **HOME** comes back.

### The controls

| Control | HOME | Elsewhere |
| --- | --- | --- |
| White keys | Your gendèr; the next core note | The same |
| Black 1-5 | Gong, gong lanang, kempur, kemong, kajar (held: shape that gong) | The same |
| Black 6-8 | Faster, slower, the ending (held: shape the cue; 6 and 7 keep going) | The same |
| Black 9-11 | Kotekan, reyong, ornaments in and out (lit: in; held: shape that part) | The same |
| K1-K4 | OMBAK, DENSITY, SPACE, BREATH | The page's four |
| SELECT | The mode, at the next gong | The page or row |
| PRESETS | The gamelan, at the next gong | The same |
| ALGO | Tempo | SHAPE: the page's level (or its own); EDIT: change the row |
| SEQ | The cycle: start, stop | |
| ARP | The gongs: the cycle or the hand | |
| REC / PLAY | Loop: record (cancel); hold, play; PLAY held: clear | |
| ENV / FX / EDIT / LFO / GLO | SHAPE / SPACE / GAMELAN / LIBRARY / SYSTEM (FX held: the throws) | Again: the next page or row |
| SEL | | LIBRARY: open; GAMELAN: change the row |
| SAVE | Save now | |
| OCT− / OCT+ | The white keys' octave | |

### The pages

The screen is Penumbra's three bands: the header (the page; the tempo, or ALGO's value), the main band, the
knob strip (what K1-K4 play). Values are shown Penumbra's way: shares as %, timbre as -/+% from AS MADE.

| Page | K1 | K2 | K3 | K4 | ALGO |
| --- | --- | --- | --- | --- | --- |
| HOME | OMBAK | DENSITY | SPACE | BREATH | Tempo |
| ENSEMBLE | CORE | KOTEKAN | REYONG | GONGS (levels) | Your gendèr's level |
| GENDÈR | BRIGHT | RING | BLOOM (the resonator) | DAMP (your hand) | Its level |
| BARS (gangsa, jublag, jegogan) | BRIGHT | RING | MALLET | BLOOM | The core's level |
| REYONG | BRIGHT | RING | MALLET | | Its level |
| GONGS | HUM | SWELL (how slowly it blooms) | BODY (the stroke) | OMBAK (how slowly they beat) | Their level |
| TIDES | RATE | SPACE | SWELL (louder and brighter) | OMBAK | SHAPE |
| PAN | GENDÈR | CORE | KOTEKAN (all the elaboration) | GONGS | RATE |
| PAN 2 | (the same four's sway) | | | | OFFSET |
| FX: SPACE | DECAY | SIZE | SHIMMER | TONE | |
| FX: SPACE 2 | MOTION | INTERVAL | LOW CUT | LEVEL | |

- **GAMELAN (EDIT)**, a list: TUNING, MODE, CYCLE, THE GONGS, CORE ON (jublag or gendèr), KOTEKAN ON (gangsa or
  gendèr), REYONG, ORNAMENTS. Later: SEL to keep it as a user gamelan.
- **LIBRARY (LFO)**: the four gamelan and four user slots; SEL opens one, arriving at the gong.
- **SYSTEM (GLO)**: Penumbra's (MIDI, clock, output level, play at power-on, sleep, about), to fill in.

Settled on the way: TIDES' BRIGHT and SWELL became one SWELL (struck bronze grows louder and brighter
together), and PAN has four places, the elaboration moving as one (the reyong sits right of the kotekan
within it).

### Slow and still: a principle for everything

Ombak plays traditional forms, and it also slows right down for ambient meditation (David, 2026-11-04). So
nothing may assume a beat comes soon, and every design is checked at both ends:

- **The tempo** reaches from 96 down to 3 core beats a minute: a core note every 20 seconds. ALGO moves it in
  steps of 4%, so the slow end is as fine as the fast, and below 12 a minute the screen shows seconds a beat
  (`10.0 S A BEAT`). Holding SLOWER sinks it further, as long as you hold.
- **Rings and spaces** last: RING reaches four times as made (a gendèr note rings over 20 seconds), each gong
  has its own RING and HUM, the reverb's DECAY reaches 40 seconds, and FREEZE holds the space indefinitely.
- **The cycle can thin out:** 4-beat cycles as well as 8, 16 and 32; each gong can sound every other cycle or
  not at all; the kajar can stop keeping the beat.
- **The elaboration** keeps its own grid (2 to 16 notes a beat), so at ten seconds a beat the kotekan can still
  ripple, or thin to a note now and then (SPARSE), or stop.
- **STILL** is a gamelan in the library that is Ombak's own, not a tradition, and says so: the gendèr and the
  gongs, a core note every ten seconds, long rings, no kajar, a deep space.

### The holds

While a key or FX is held, K1-K4 (and ALGO) play something else, as in Penumbra; the screen shows the hold
(after 0.3 s, so playing never flickers it) with its knobs as dials and the strip. A gong, a part or a cue:
the change stays. A white key (bends) and FX (throws): let go and they drift back over a couple of seconds;
SAVE while holding FX keeps the throws until FX is next held and a knob turned.

| Hold | K1 | K2 | K3 | K4 | ALGO |
| --- | --- | --- | --- | --- | --- |
| A gong (black 1-5) | LEVEL | TONE (which tone of the mode it sounds) | HUM | RING | PLACES: off, as made, every other cycle (the lanang: off as made, or answering the gong at the half; the kajar: every beat or every other) |
| FASTER / SLOWER (6, 7) | STEP (each press; held, a step every two seconds) | GLIDE (how fast the ensemble follows) | | | |
| ENDING (8) | AFTER (the next gong, or the one after) | SLOWING | THINNING | | |
| KOTEKAN (9) | DENSITY (as HOME's, or 2, 4, 8, 16 a beat) | REGISTER | TURN (where in the beat it turns to the next note) | SPARSE | Its level |
| REYONG (10) | RATE (half the kotekan, or a quarter) | REGISTER | TURN | SPARSE | Its level |
| ORNAMENTS (11) | PICKUP (how early the next note comes) | OCTAVES (the core in octaves on the beat) | REGISTER | SPARSE | Their level |
| A white key (bends, on that note) | SHIMMER (into the space) | OMBAK (its own beat: still to ×3) | PLACE | SWELL | |
| FX (throws) | SHIMMER | FREEZE (the space holds; new sound stops going in) | DIP (low-pass left, high-pass right) | HUSH (the core and elaboration fall away; the gongs and your gendèr stay) | OMBAK (thrown slower or faster) |
| ENV | A/B, as Penumbra's (later) | | | | |

- **A gong key strikes when pressed** (it is an instrument); holding it to shape it shapes the next strokes.
- **A part key toggles when let go,** unless its knobs were turned, so shaping a part never switches it.
- **A white key held is not damped** by the next note you play: two hands, as a gendèr player's, one holding
  a note ringing while the other moves on.
- **FX pressed** opens the FX page; **FX held** is the throws.

FREEZE, HUSH and holding SLOWER are the meditative gestures: let the space hold, let the ensemble fall away
to the gongs and your own notes, let the tempo sink.

### Still to design

A/B as Penumbra's ENV hold, user gamelan and saving, MIDI (the core and the gendèr on channels, the cues as
notes, the macros as CCs), SLEEP, and the cheat sheet. Penumbra's delay is left out for now: the gamelan's
own ringing and the ombak fill that space.

## The firmware: 0.1 beta (2026-11-04), the first build to play

`./build.sh --release 0.1` makes `build/ombak-0.1.fwsc` (identity `FM-1_74000100`: 74 is Ombak's own, free on
thefirmwarehub.com as David found on 2026-11-04; Penumbra's is 73). It installs as Penumbra does (BUILDING.md).

**What it has:**
- Penumbra's platform as is (boot, safe mode, update mode, USB, storage, the update path, the two cores, the
  reverb, the master stage, the screen reader's channel, the simulator), renamed, in a bronze palette.
- The voices (`dsp/bars.c`): the prototype's FM instruments (gendèr, gangsa, jublag, jegogan, reyong, gong,
  kempur, kemong, kajar), a pool of 48, split across the two cores. The pengumbang-pengisep pair is computed as
  one tone times its beating (exact for the carriers, half the cost), so the pair sounds from one place rather than
  a little apart as in the prototype. The overload guard lets only 24 sound at once when the FM-1 struggles.
- The gamelan (`app/gamelan.c`): the ten measured tunings, the modes, the five gamelan (STILL among them).
- The engine (`app/engine.c`): the cycle on a 16-step grid a beat, the core led by the white keys, the kotekan,
  reyong and ornaments, the gongs and their places, the breath, the cues, the ending (now at most 2.5 times as
  slow at the gong), the loop, the tides, the pan, the throws, a mode or gamelan arriving with the gong, the tempo
  from 3 to 96 a minute.
- The interface (`app/ui.c`): the Ongkara HOME (its glyph and glow from `assets/ongkara.png`; SYSTEM's HOME row
  switches to the gong), the SHAPE pages, FX, GAMELAN, LIBRARY, SYSTEM, BENCH, every hold, the lights, the screen
  reader's words. Everything shaped is saved (`OBJ_WORK`, its own markers: Penumbra's records load as defaults).
- The simulator's scenarios (`tests/scenarios/play.pen`, `still.pen`) pass: the cycle, a melody led, the pages, the
  holds, FREEZE and HUSH, STILL sinking below 6 a minute, the ending, a reboot keeping the work; no clicks.

**Not yet:** MIDI (in and out, the clock), user gamelan in the library, A/B, SLEEP, the web installer's own pages
and the cheat sheet, the manual. (0.1.4: MIDI in, MINE, A/B and SLEEP done; MIDI out and the clock still to
come.)

**To find out on the FM-1:** how hard it works (BENCH, and SEL there for the load test: 48 long gendèr notes at
once), whether the gongs' lows and the glow read well, how the knobs feel in their steps, and how it all sounds.

## 0.1.4: the forms, the kotekan's styles, the boot screen

David on 0.1.3 (2026-11-05): it sounds great, and the load is reasonable even at high density and tempo. Asked
for a few more patterns, and the Penumbra eclipse gone from the boot screen.

- **Gong forms** (GAMELAN: FORM; `G_FORM` in `gamelan.c`): FREE (Ombak's own: the gong, the half, the quarters,
  any CYCLE), and four from the tradition, each its own length: **GILAK** (8: the female gong on 1, the male on 5,
  the kempur on 6 and 8, the kempli on the odd beats, as the Guelph/Waterloo teaching gamelan sets it out in
  *Music of the World*, openwa.pressbooks.pub), **LANCARAN** (16, TWTN TPTN TPTN TPTG), **KETAWANG** (16, pTpW
  pTpN pTpP pTpG) and **LADRANG** (32), the Javanese gendhing structures as Wikipedia's *Colotomy* gives them.
  Ombak's instruments stand in: the kempul is the kempur, the kenong the kemong, the ketuk and kempyang the kajar
  (kempyang lighter), the kempli the kajar. Java counts to the gong at the end, so its last beat is Ombak's
  place 0. A form with the lanang in it (gilak) brings the lanang in; CYCLE turned from a form goes back to FREE.
- **Kotekan styles** (GAMELAN: KOTEKAN): CIRCLING (as made), NOROT (the note and the one above in turn, then the
  next and its own, anticipating it), NYOG CAG (one line, the parts in turn, walking to the next note), EMPAT
  (four tones: the polos the lower three, the sangsih the upper two, together on the first; after ubit empat,
  simplified). These are Ombak's readings of the techniques' outlines, not transcriptions.
- **Two gamelan:** GILAK (gong Peliatan, 8 beats, nyog cag, 60 a minute) and LADRANG (Kanyut Mesem pelog, pathet
  nem, 32 beats, the core and gendèr ornaments without kotekan, 30 a minute, little ombak). ANGKLUNG now plays
  norot.
- **The boot screen:** a bronze gong lit from above left, ripples going out from it, the name, and the Lunar
  Eclipse Forge mark by the maker's name.
- The work is format 2 (the form and the style); a format 1 work loads with them as made.
- **MIDI in** (SYSTEM: MIDI CHANNEL, 1 as made, or OFF): white keys from middle C play the tones in turn (the
  gendèr at once, velocity its stroke, and the next core note), black keys strike the gongs (C# gong, D# lanang,
  F# kempur, G# kemong, A# kajar), the sustain pedal lifts the damping hand, CC 20–23 HOME's OMBAK, DENSITY,
  SPACE, BREATH (CC 1 DENSITY too), CC 24 the tempo, a program change the gamelan at the next gong (stopped: as
  the cycle starts), Start and Stop the cycle, CC 120/123 damp everything. MIDI changes are not autosaved.
  Not yet: MIDI out, the clock.
- **SLEEP** (SYSTEM, Penumbra's): off, 20, 40, 60 or 90 minutes, this session only. Five minutes before, it cues
  the ending (slowing, thinning, closing on the gong); at the time, anything still going stops and rings out.
  HOME shows the minutes left.
- **MINE** (LIBRARY): four slots for works of your own after the seven gamelan (the list scrolls now: with seven
  it had run off the screen). SAVE on a slot keeps the work playing there (`OBJ_USER`), SEL opens it, at once.
- **A/B** (SHAPE's SEL): two shapes of the gamelan playing, this session's: everything from HOME's knobs to the
  FX, not the gamelan, form or kotekan style. B starts as A.

David on 0.1.4 (2026-11-05), flashed and tested on the FM-1: "It runs great. Sounds amazing."

## 0.1.5: the MIDI clock, notes out, the names checked

- **FOLLOW CLOCK IN** (on as made): the ticks' stamps (usb.c, midi_uart.c, as Penumbra's) time the tempo over
  the last two beats' ticks; at each beat the cycle compares its beats with the ticks counted since it started
  and nudges that beat's length by half the difference (a beat between half and one and a half times a tick
  period of 24), so it stays in phase however BREATH would shape it. Start (or Continue) runs the cycle, Stop
  stops it; with FOLLOW on, an incoming Start is never echoed. Half a second without a tick: Ombak's tempo.
- **MIDI CLOCK OUT** (on as made): 24 ticks spread over each beat as it is played (the breath and the ending's
  slowing in them), Start and Stop with the cycle (and at the ending's last gong); none while following.
- **MIDI NOTES OUT** (off as made): the FM-1's white keys as MIDI in reads them (the tones counted in white keys
  from middle C) and the gongs by hand (C#, D#, F#, G#, A# above middle C); the cycle's notes are not sent.
- Settings saved by 0.1.4 load with the three new rows OFF (their bytes were reserved as zero).
- Tested in the simulator (`tests/scenarios/clock.pen`): 97 ticks over four beats at 40, a DAW's 120 followed
  to the BPM and the cycle on its gong after 16.5 beats, a minute at 100 with BREATH up still in phase, the
  keys and gongs out.

**Released** (2026-11-05): 0.1.5 published by David as a pre-release on github.com/MadMooneyham/Ombak (tag
v0.1.5); the site and installer at madmooneyham.github.io/Ombak, built by `.github/workflows/pages.yml`. Pushes
go through Ombak's own deploy key (`~/.ssh/id_ombak`, the `github-ombak` host alias in `~/.ssh/config`).

## 0.1.6: the newer flash chip; the hub's card

The hub's submit page (2026-11-05) warns that FM-1s sold since autumn 2026 carry a UC25HQ80 flash (JEDEC
0xB36014) instead of the Puya P25Q80 (0x856014), with the same commands. Ombak (and Penumbra, whose code it is)
accepted only the Puya: on a newer unit nothing would be saved. 0.1.6 accepts both (`main_fm1.c`); the update
loader checks no ID. Penumbra needs the same fix.

For the hub: `fmhub.json` in the root (its card's text, kind "other", the manual's cheat sheet, the bronze
#D6A54E) and `assets/card.png` (HOME playing, from `tests/scenarios/card.pen`). Submission is a mail to the
hub's maintainer, which David sends.

## 1.0 (2026-11-05)

David published 0.1.6 as a full release and asked for Ombak to be one: 1.0 is 0.1.6's firmware as a full
release (identity `FM-1_74010000`, ABOUT says `1.0`, no BETA), with RELEASE-1.0.md, the README, the manual and
BUILDING saying so.

Published as v1.0 the same day; the site serves it (FM-1_74010000). David ran the hub's install tests (from V15
and from another firmware, and leaving again) and sent the submission mail to thefirmwarehub.com. David has no
DAW: the MIDI clock (in and out) and notes out are tested in the simulator only, and wait on players' reports.

## 1.1: the drums, angsel, the suling, GONG KEBYAR; the emulator and the cheat sheet

David (2026-11-05): the knob noise is completely resolved. Then: the cheat sheet, and every feature suggested,
in the most logical order (sounds first, so the cheat sheet and the emulator show them; then the release).

- **Kendang and ceng-ceng** (`dsp/bars.c`: IN_WADON, IN_LANANG, IN_CENG; `engine.c` drums()): a skin's tone
  falling into place fast plus a slap of noise; the ceng-ceng a hiss over clangorous FM partials. Patterns on an
  eight-step grid a beat, two figures in turn; strokes a beat 8 (BUSY low: 4), 2 over a 2.4 s beat, 1 over 4 s.
  They sit in the elaboration's bus (HUSH lets them go). Two partials a drum, three for the cymbals.
- **Angsel** (SEL on HOME, CC 25): the next beat is a break: unison accents at steps 0 and 10 of 16, nothing else.
- **Suling** (`engine.c` suling_block): three sines and a narrow band of breath noise on the first core, gliding,
  its vibrato growing in; circular breathing is real Balinese practice, so it never breaks. On the gendèr's bus.
- **GONG KEBYAR** (gamelan 8): Peliatan's five-tone pelog (its famous ensemble is a kebyar), 75 a minute, 7 Hz.
  The heaviest gamelan: about 1.1 to 1.2 million sines a second a core in the simulator at full density, against
  0.7 for semar pegulingan; economy is there for it, and BENCH on the FM-1 is the judge.
- **MIDI CC OUT** (SYSTEM): the knobs, tempo, angsel and gamelan out, sent from the audio ISR (one sender).
- **The work is format 3** (the drums' and suling's params); `settings.c` reads formats 1, 2 and 3 byte by byte,
  for the work and for MINE (`tests/scenarios/ensemble.pen` saves 1.0's format and loads it).
- **The emulator** (`web/emu/`, Penumbra's, X0X's before): the simulator in WebAssembly; the site builds it.
- **The cheat sheet** (`web/cheatsheet/`, with a PDF printed by headless Chrome), linked from the hub's card.
- Not done: selonding (the sacred iron ensemble; it needs careful sources).

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
