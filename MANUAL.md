# Ombak manual

Ombak 0.1.5 for the M-VAVE FM-1, by Lunar Eclipse Forge.

Ombak is an instrument inspired by the gamelan of Bali and Java. It is not a gamelan, and it does not
pretend to be one. It plays measured tunings from real ensembles, and it builds its parts the way a gamelan
does. You lead the melody, and the ensemble answers.

- **The core** is the melody you lead. Each white key you play sounds your gendèr at once and becomes the
  next core note. The core instruments strike it on the next beat.
- **The elaboration** fills each beat: the interlocking kotekan, the reyong and the gendèr's ornaments. Each
  one leaves the core note and turns toward the next one, landing on it as it arrives.
- **The gongs** mark the cycle: the gong at its start, and the smaller gongs and the kajar inside it.
- **The ombak** is what gives Ombak its name. *Ombak* means "wave": the shimmer of two instruments tuned a
  little apart, beating against each other.

Ombak plays the traditional forms, and it also slows right down for meditation, to a core note every 20
seconds. Everything in it works at both ends.

Contents:

1. [Starting](#1-starting)
2. [HOME and the screen](#2-home-and-the-screen)
3. [The keys](#3-the-keys)
4. [Leading, cueing and ending](#4-leading-cueing-and-ending)
5. [The holds](#5-the-holds)
6. [The loop](#6-the-loop)
7. [The gamelan, the tunings and the forms](#7-the-gamelan-the-tunings-and-the-forms)
8. [SHAPE: the sound](#8-shape-the-sound)
9. [FX, the library, SYSTEM and saving](#9-fx-the-library-system-and-saving)
10. [Slow and still](#10-slow-and-still)
11. [MIDI](#11-midi)
12. [Cheat sheet](#12-cheat-sheet)
13. [The tradition, and thanks](#13-the-tradition-and-thanks)

---

## 1. Starting

Switch the FM-1 on. Ombak shows the bronze gong, then HOME, stopped. Nothing plays by itself. The white keys
already play your gendèr, and the black keys strike the gongs, so you can find the tuning before anything
moves. To start playing at power-on instead, turn **PLAY AT POWER-ON** on in SYSTEM.

- **SEQ** starts the cycle. When the cycle keeps the gongs, the gong opens it, and a light sets off round the
  rim. Press SEQ again to stop: the cycle stops and everything rings out.
- **ALGO** sets the tempo, from 96 core beats a minute down to 3 (a core note every 20 seconds). Each step is
  about 3%, so the slow end is as fine as the fast. Below 12 a minute, the screen shows the seconds a beat.
- **HOME** always brings you back to the main screen. Any other page also returns to HOME after 30 seconds
  untouched, except BENCH.
- **Hold any button** for a moment, and it says what it does instead of doing it.

## 2. HOME and the screen

HOME shows the Ongkara, the sacred syllable Om in Balinese script. Its light follows the music: it glows
with the ensemble's swell, flares with the gong, and its halo widens and narrows with the tide. It never
pulses to the beat. If you prefer, SYSTEM's **HOME** row shows a bronze gong there instead, whose boss
flares with each stroke.

Round the rim is the gong cycle, one mark for each beat:

- **Gold** marks the gong.
- **Teal** marks the kemong and the gong lanang.
- **Red** marks the kempur.
- **Small grey dots** are the kajar's beats.

A bright bead appears where each core note was struck, fading over a cycle, and the travelling light shows
where the cycle is now. When the gongs are by hand, the marks dim.

Under the circle is what is playing: the mode and the gamelan (`SELISIR, SEMAR PEGULINGAN`). When a change is
waiting for the gong, it shows what is coming, in amber (`SELISIR > TEMBUNG`).

The strip at the bottom shows HOME's four knobs, and the state: PLAYING, STOPPED or ENDING, the cycle's
length, and the loop. On the right, it shows the octave, or SLEEP with the minutes left.

**HOME's knobs** are the performance:

| Knob | Does |
| --- | --- |
| K1 OMBAK | The shimmer's rate, from 0.25 to 10 Hz. Slow is a deep, slow wave; fast, a bright tremble |
| K2 DENSITY | From the core alone, to a flowing kotekan |
| K3 SPACE | How much of everything goes into the reverb |
| K4 BREATH | From a steady pulse, to beats that swell and slow into each gong |

On HOME, **SELECT** walks the tuning's modes, and **PRESETS** walks the gamelan. Either change arrives with
the next gong, as a real ensemble would change. Stopped, it changes at once.

## 3. The keys

| Keys | Do |
| --- | --- |
| White keys | Your gendèr, and the next core note |
| Black 1-5 | Gong, gong lanang, kempur, kemong, kajar |
| Black 6, 7 | FASTER, SLOWER |
| Black 8 | The ENDING |
| Black 9, 10, 11 | The kotekan, the reyong, the ornaments: in or out (lit: in) |

- **The white keys** play the tuning's tones in order, the first white key the mode's first tone. **OCT-** and
  **OCT+** move them an octave down or up.
- **Your gendèr damps as it goes,** as a player's hand does: each new note stops the one before. Hold a key, and
  it rings on while you play others, as a gendèr player's two hands do.
- **A gong key strikes when you press it.** The gongs are instruments, so you can play them whether or not the
  cycle keeps them.
- **The part keys** (9-11) bring a part in or out when you let go. A piece can begin with the core and the
  gongs, and open out.

## 4. Leading, cueing and ending

**Leading.** Play a white key while the cycle runs, and the core lands on it at the next beat. The kotekan,
reyong and ornaments spend that beat turning toward it. Play nothing, and the parts keep circling the note
they have. A melody is a few notes, one a beat or one every few beats. You don't have to keep playing.

**Taking the gongs.** **ARP** switches the gongs between the cycle and your hand (lit: the cycle keeps them).
By hand, black 1-5 are yours alone, and the beat runs on either way.

**Cueing,** as the kendang drummer does in an ensemble:

- **FASTER** (black 6) and **SLOWER** (black 7) move the tempo a step. The ensemble glides there rather than
  jumping. Hold one, and the tempo keeps moving, a step every two seconds.
- **The ENDING** (black 8) slows each beat, thins the elaboration, and closes on the gong: the next gong at
  least four beats away, or, if you set it, the one after. Then the cycle stops and everything rings out.

## 5. The holds

Hold a key or FX, and the knobs and ALGO play something else while you hold it. The screen shows the hold
after a third of a second, so playing never flickers it.

- **A gong, a part or a cue:** the change stays.
- **A white key (bends) and FX (throws):** let go, and they drift back over a couple of seconds. SAVE while
  holding FX keeps the throws out, until FX is next held and a knob turned.

| Hold | K1 | K2 | K3 | K4 | ALGO |
| --- | --- | --- | --- | --- | --- |
| A gong (black 1-5) | LEVEL | TONE (which tone of the mode it sounds) | HUM | RING | PLACES |
| FASTER / SLOWER (6, 7) | STEP (how far each press goes) | GLIDE (how fast the ensemble follows) | | | |
| ENDING (8) | AFTER (the next gong, or the one after) | SLOWING | THINNING | | |
| KOTEKAN (9) | DENSITY (as HOME's, or 2, 4, 8, 16 notes a beat) | REGISTER | TURN (where in the beat it turns to the next note) | SPARSE | Its level |
| REYONG (10) | RATE (half the kotekan, or a quarter) | REGISTER | TURN | SPARSE | Its level |
| ORNAMENTS (11) | PICKUP (how early the next note comes) | OCTAVES (the core in octaves on the beat) | REGISTER | SPARSE | Their level |
| A white key (bends on its note) | SHIMMER (into the space) | OMBAK (its own beat: still, to three times) | PLACE | SWELL | |
| FX (throws) | SHIMMER | FREEZE | DIP | HUSH | OMBAK |

- **PLACES** says where in the cycle a gong sounds: off, as made, or every other cycle. The lanang is off as
  made, unless the form has it. The kajar plays every beat, or only its strong ones.
- **SPARSE** leaves out notes at random, so a part thins to a note now and then. It never leaves out the
  landing on the core note.
- **Holding a part to shape it** doesn't switch it in or out when you let go.
- **The throws** are the gestures. FREEZE holds the space while new sound stops going into it. DIP is a
  low-pass filter turned left, and a high-pass turned right. HUSH lets the core and the elaboration fall
  away, leaving the gongs and your gendèr. OMBAK throws every beat slower or faster.

FREEZE, HUSH and holding SLOWER are the meditative gestures: let the space hold, let the ensemble fall away
to the gongs and your own notes, and let the tempo sink.

## 6. The loop

The loop plays the core for you, so your hands are free for the gendèr.

- **REC** records the core over the next cycle, from its start: up to 16 beats. REC again before it starts,
  or while it records, cancels it.
- Then it **plays as the core**, and your white keys play the gendèr freely over it.
- **PLAY** holds the loop: you lead again. PLAY again brings it back.
- **PLAY held**, or **REC and PLAY together**, clears it.

The loop is saved with everything else.

## 7. The gamelan, the tunings and the forms

**The library's gamelan.** PRESETS walks them, and LFO lists them.

| Gamelan | What it is |
| --- | --- |
| GENDER WAYANG | The gendèr quartet of the shadow play: slendro (Peliatan), gendèr for the core and the kotekan, the gongs by hand (it has none) |
| SEMAR PEGULINGAN | The seven-tone court gamelan (saih pitu, Tampak Gangsul): gangsa kotekan, gendèr ornaments (as made, at power-on) |
| ANGKLUNG | The four-tone slendro (Mas), with reyong and a norot kotekan |
| STILL | Ombak's own, for meditation, not a tradition: the gendèr and the gongs, a core note every ten seconds, long rings, no kajar, a deep space |
| GONG GEDE | Five-tone pelog (Peliatan), 32 beats, slow and breathing |
| GILAK | The eight-beat Balinese form that opens an event, in gong tuning, with nyog cag kotekan, 60 a minute |
| LADRANG | The 32-beat Javanese form on Kanyut Mesem pelog (pathet nem): the core and the gendèr's ornaments, no kotekan, 30 a minute |

**GAMELAN (EDIT)** shows the gamelan playing as a list. SELECT picks the row, and ALGO, a knob or SEL changes
it.

| Row | What it sets |
| --- | --- |
| TUNING | One of the ten measured tunings (below) |
| MODE | The mode, for tunings with modes |
| FORM | The gong form: FREE, GILAK, LANCARAN, KETAWANG or LADRANG |
| CYCLE | FREE's length: 4, 8, 16 or 32 beats. Turning it from another form goes back to FREE |
| THE GONGS | The cycle keeps them, or your hand (ARP too) |
| CORE ON | Jublag and jegogan, or gendèr |
| KOTEKAN ON | Gangsa, or gendèr |
| KOTEKAN | The style: CIRCLING, NOROT, NYOG CAG or EMPAT |
| REYONG, ORNAMENTS | In or out (black 10 and 11 too) |

**The tunings** are measured sets from real instruments, as the Scala scale archive transcribes them.

| Tuning | System |
| --- | --- |
| GENDER WAYANG, PELIATAN; GENDER WAYANG, UBUD | Slendro, five tones |
| LOW GENDER, TABANAN | Slendro (McPhee) |
| ANGKLUNG, SANGSIT | Slendro, five tones |
| ANGKLUNG, MAS | Slendro, four tones (McPhee) |
| KANYUT MESEM SLENDRO | Java, Surakarta (Surjodiningrat et al.) |
| SAIH PITU, TAMPAK GANGSUL; SAIH PITU, KSATRIA | Bali's seven-tone pelog: the modes SELISIR, TEMBUNG, SUNAREN, BARO |
| GONG, PELIATAN | Five-tone pelog (McPhee) |
| KANYUT MESEM PELOG | Java's seven-tone pelog: PATHET NEM, PATHET BARANG |

The Balinese five-tone sets name their tones *ding, dong, deng, dung, dang*. On the gendèr wayang the lowest key
is *dong*, so the three gendèr tunings run dong, deng, dung, dang, ding from the first white key, as the
instrument does. The Javanese use numbers.

SAIH PITU, TAMPAK GANGSUL is McPhee's measurement of the semar pagulingan of Banjar Tampakgangsul in Denpasar.
That ensemble disbanded in 1940; its gamelan is played today by the sekaa of Pagan Klod.

**The forms.** A form says which gong sounds where in the cycle.

- **FREE** is Ombak's own, for any length: the gong at the start, the kemong and the lanang at the half, the
  kempur at the quarters, and the kajar every beat.
- **GILAK** (8 beats): the gong on the first beat, the lanang on the fifth, the kempur on the sixth and eighth,
  and the kajar on the odd beats.
- **LANCARAN** (16), **KETAWANG** (16) and **LADRANG** (32) are Javanese structures. Ombak's own instruments
  stand in for the Javanese ones: the kempur for the kempul, the kemong for the kenong, and the kajar for the
  ketuk and kempyang (the kempyang lighter). Java counts toward the gong at the end of the cycle; on the rim,
  that gong is at the top, where each cycle ends and the next begins.

**The kotekan's styles.** Two parts, the *polos* and the *sangsih*, interlock into one line. These are Ombak's
readings of the techniques' outlines, not transcriptions:

- **CIRCLING** (as made) circles the core note, then the next one, landing on it.
- **NOROT** alternates the note and the one above it, then, partway through the beat, the next note and the one
  above that, anticipating it.
- **NYOG CAG** is one line, the parts taking turns, walking to the next note.
- **EMPAT** spreads four tones: the polos plays the lower three, the sangsih the upper two, and they strike
  together on the first.

## 8. SHAPE: the sound

**ENV** opens SHAPE, a page at a time. ENV again, or SELECT, moves to the next page. Values show Ombak's way:
levels and amounts as %, and timbre as -/+% from AS MADE.

| Page | K1 | K2 | K3 | K4 | ALGO |
| --- | --- | --- | --- | --- | --- |
| ENSEMBLE | CORE | KOTEKAN | REYONG | GONGS (their levels) | Your gendèr's level |
| GENDER | BRIGHT | RING | BLOOM (the resonator) | DAMP (your hand) | Its level |
| BARS (gangsa, jublag, jegogan) | BRIGHT | RING | MALLET | BLOOM | The core's level |
| REYONG | BRIGHT | RING | MALLET | | Its level |
| GONGS | HUM | SWELL (how slowly it blooms) | BODY (the stroke) | OMBAK (how slowly they beat) | Their level |
| TIDES | RATE (8 seconds to 4 minutes a tide) | SPACE | SWELL (louder and brighter) | OMBAK | SHAPE (even, or wandering) |
| PAN | GENDER | CORE | KOTEKAN (all the elaboration) | GONGS | RATE (0: the tide's) |
| PAN 2 | (the same four's sway) | | | | OFFSET |

**The tides** are one slow swell under everything: the space opens, the ensemble grows louder and brighter,
and the ombak quickens, then all of it settles again. RING reaches four times as made, so a gendèr note can
ring for over 20 seconds.

**A/B.** On SHAPE, **SEL** switches between two versions of everything you have shaped, from HOME's knobs to
the FX. B starts as a copy of A; shape it apart, and SEL goes back and forth. The gamelan, its form and its
kotekan style stay as they are. A/B lasts only this session.

## 9. FX, the library, SYSTEM and saving

**FX** sets the space: a long shimmering reverb. SELECT, or FX again, picks the row, and the knobs change it.

| Row | K1 | K2 | K3 | K4 |
| --- | --- | --- | --- | --- |
| SPACE | DECAY (up to 40 seconds) | SIZE | SHIMMER | TONE |
| SPACE 2 | MOTION (how much its lines move) | INTERVAL (the shimmer: an octave, a fifth or a twelfth up) | LOW CUT (on what goes in) | LEVEL (its return) |

**The library (LFO)** lists the seven gamelan, then four slots of your own: **MINE 1** to **MINE 4**.

- **SEL** on a gamelan opens it at the next gong (stopped, at once). PRESETS walks the list too.
- **SAVE** on a MINE slot keeps everything playing there: the gamelan, its shape, its loop.
- **SEL** on a MINE slot opens it, at once.

**SYSTEM (GLO)**: SELECT moves through the list, and a knob or SEL changes the row shown.

| Setting | What it does |
| --- | --- |
| HOME | THE ONGKARA, or THE GONG |
| SLEEP | Off, or 20, 40, 60 or 90 minutes (below) |
| PLAY AT POWER-ON | Starts the cycle when you switch on |
| MIDI CHANNEL | MIDI in and out: 1-16, or off (section 11) |
| FOLLOW CLOCK IN | An incoming MIDI clock leads the tempo (FOLLOWING while it does) |
| MIDI CLOCK OUT | Sends MIDI clock, Start and Stop while the cycle runs |
| MIDI NOTES OUT | Sends the keys you play on the FM-1 as notes |
| OUTPUT LEVEL | NORMAL, or LOUD (6 dB hotter) |
| USB AUDIO | MASTER (follows the volume knob), or FULL level |
| 2ND CORE | Uses the FM-1's second processor core; applies at the next power-on |
| RESET SOUND | SEL twice puts every setting of the sound back as made, and clears the loop. The gamelan, tuning and mode playing stay |
| ABOUT | The version |

**SLEEP** lets the ensemble go, as a piece ends. Five minutes before the time, it cues the ending: the
ensemble slows, thins and closes on the gong. At the time, anything still going stops and rings out. SLEEP
lasts only this session; it is not saved.

**BENCH**: press GLO again from SYSTEM. It shows what each part of the sound costs the processor, and how
many voices are sounding. If the FM-1 is working very hard, Ombak first ends quiet tails sooner and lets
fewer voices sound at once (ECON), and, only if that is not enough, holds the voices to 20 (GUARD). BENCH
counts how often each stepped in. SEL there runs a load test.

**Saving** happens by itself, once nothing has changed for two seconds and the sound has been silent for one.
Writing to the FM-1's memory briefly stops the audio, so Ombak waits until there is nothing to interrupt.
SAVE saves at once. Everything is kept: the gamelan and its shape, the loop, and the settings. Changes made
over MIDI are kept the next time something else is saved.

**Leaving Ombak.** Install another firmware the usual way; Ombak carries the update loader. If the FM-1 ever
won't take an install, hold **OCT-** and **OCT+** together for five seconds to put it in update mode.

## 10. Slow and still

To play Ombak for meditation:

1. Open **STILL** (PRESETS or the library). It has a core note every ten seconds, long rings and no kajar.
2. Play a white key now and then. The core takes it at the next beat, and the space holds it.
3. **Hold SLOWER** to sink the tempo further, as far as a note every 20 seconds.
4. Hold **FX** and turn **FREEZE** to let the space hold, or **HUSH** to let the ensemble fall away to the gongs
   and your own notes.
5. Set **SLEEP** in SYSTEM, and the ensemble ends itself on the gong when the time comes.

Every gamelan slows this far, and each gong's PLACES (hold it, then ALGO) can thin the cycle, so the gong
sounds every other cycle, or not at all.

## 11. MIDI

Connect the FM-1 by USB: a computer sees it as an audio device and a MIDI device. The TRS jack takes MIDI in
too. Everything below is on SYSTEM's **MIDI CHANNEL** (1 as made), except Start and Stop, which work on any
channel.

| MIDI | Does |
| --- | --- |
| White keys | The tones in order from middle C (C4, note 60): your gendèr, and the next core note. Velocity is the stroke |
| Black keys | In any octave: C# the gong, D# the lanang, F# the kempur, G# the kemong, A# the kajar |
| Sustain pedal (CC 64) | Lifts the damping hand: every note rings on |
| CC 20, 21, 22, 23 | OMBAK, DENSITY, SPACE, BREATH (CC 1 is DENSITY too) |
| CC 24 | The tempo |
| Program change | The gamelan (0 GENDER WAYANG to 6 LADRANG), at the next gong. Stopped, as the cycle starts |
| Start, Stop | The cycle |
| CC 120, CC 123 | Everything you are playing is damped |

**The clock.** A core beat is a quarter note: 24 MIDI clock ticks.

- **FOLLOW CLOCK IN** (on as made): when a clock arrives, its tempo leads Ombak's, and each beat is nudged so the
  cycle stays in phase with it. BREATH and the ending's slowing step aside while it leads, and ALGO says so.
  Start runs the cycle from the gong, and Stop stops it. Half a second without a tick, and Ombak goes back to its
  own tempo.
- **MIDI CLOCK OUT** (on as made): while the cycle runs on its own tempo, Ombak sends its clock, with Start and
  Stop. Its ticks follow every beat as it is played, so a DAW hears the breath and the ending slow down too.
  Ombak never sends a clock while it follows one.

**Notes out.** With MIDI NOTES OUT on, the keys you play on the FM-1 go out on the MIDI channel, mapped the way
MIDI in reads them: the white keys as the tones counted from middle C, and the gongs struck by hand as C#, D#,
F#, G# and A# above it. The cycle's own notes are not sent.

## 12. Cheat sheet

| Control | Does |
| --- | --- |
| White keys | Your gendèr; the next core note (held: bends on its note) |
| Black 1-5 | Gong, lanang, kempur, kemong, kajar (held: shape it; ALGO: its places) |
| Black 6 / 7 | Faster / slower (held: it keeps going; turn a knob: shape the cue) |
| Black 8 | The ending (held: shape it) |
| Black 9-11 | Kotekan, reyong, ornaments in or out (held: shape the part) |
| K1-K4 (HOME) | OMBAK, DENSITY, SPACE, BREATH |
| SELECT (HOME) | The mode, at the next gong |
| PRESETS | The gamelan, at the next gong |
| ALGO | Tempo (SHAPE: the page's own; GAMELAN: change the row; holds: see section 5) |
| SEQ | The cycle: start, stop |
| ARP | The gongs: the cycle, or your hand |
| REC | Loop the core from the next cycle (again: cancel) |
| PLAY | Hold the loop / bring it back; held: clear |
| REC + PLAY | Clear the loop |
| FX | The space (again: the next row); held: the throws (SHIMMER, FREEZE, DIP, HUSH; ALGO: OMBAK) |
| ENV | SHAPE (again: the next page); SEL there: A/B |
| EDIT | GAMELAN: tuning, mode, form, cycle, parts, kotekan style |
| LFO | The library (SEL: open; SAVE on MINE: keep) |
| GLO | SYSTEM, then BENCH |
| SAVE | Save now; holding FX: keep the throws |
| OCT- / OCT+ | The white keys' octave; both for 5 s: update mode |
| Any button, held | Says what it does |

## 13. The tradition, and thanks

Gamelan is a living tradition of Bali and Java, with its own teachers, ensembles and meanings. Ombak borrows
its tunings, its instruments' names and the outlines of its techniques with respect, and it is no substitute
for hearing, or learning to play, the real thing. Where Ombak makes its own choices (STILL, FREE, the kotekan
styles' simplifications), this manual says so. If you know the tradition and something here is wrong, please
tell us.

The tunings come from the Scala scale archive (Manuel Op de Coul), from Colin McPhee's *Music in Bali* (1966),
and from Surjodiningrat, Sudarjana and Susanto's measurements of Javanese gamelan (1972). Gilak's placements
come from the University of Guelph and University of Waterloo teaching resources by Dewa Made Suparta and
Maisie Sum. The Javanese structures are as Wikipedia's article on colotomy sets them out. The Ongkara is drawn
from Noto Sans Balinese (SIL Open Font License).

Ombak is free software under the GPL-3.0, built on Penumbra, X0X and Felucca. See LICENSING.md.
