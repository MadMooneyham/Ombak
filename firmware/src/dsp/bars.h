/* SPDX-License-Identifier: GPL-3.0-only */
/* Ombak's voices: the gamelan's bars, kettles and gongs, all FM, from one pool.
 *
 * A stroke is a few sine carriers at inharmonic ratios (the bar's or the gong's modes), each with an
 * optional sine modulator whose index falls fast (the mallet's brightness), a band of noise for the
 * mallet, and for the gongs a hum: a sine at the fundamental that swells after the stroke and fades
 * slowly. Each carrier rings down exponentially; the slow-attack ones are the resonators' bloom.
 *
 * Every instrument is a pair, pengumbang and pengisep, tuned beat Hz apart. Two equal sines at f - b/2
 * and f + b/2 sum to 2 sin(2 pi f t) cos(pi b t): one tone whose loudness beats. So a stroke is rendered
 * once and multiplied by the beating, which is exact for the carriers and costs half of two copies.
 * (The prototype placed the two a little apart in the field; here they sound from one place.)
 *
 * The pool's voices are rendered in two halves, one on each core (engine.c), each into its own part
 * buses. A voice is stolen, quietest first, when the pool is full. */
#pragma once
#include <stdint.h>

#define BARS_SR 44100.0f
#define BARS_VOICES 48
#define BARS_OPS 6                       /* five carriers and a gong's hum */
#define BARS_PARTS 4                     /* the buses: your gendèr, the core, the elaboration, the gongs */

enum { IN_GENDER, IN_GANGSA, IN_JUBLAG, IN_JEGOGAN, IN_REYONG, IN_GONG, IN_KEMPUR, IN_KEMONG, IN_KAJAR,
       IN_WADON, IN_LANANG, IN_CENG, IN_N };     /* the kendang's two drums (wadon low, lanang high), the ceng-ceng */
#define BARS_IS_GONG(i) ((i) >= IN_GONG && (i) <= IN_KAJAR)

/* phases are fixed point: a turn is 2^32, so they wrap by themselves and the sine table's index and fraction
 * are their bits (no float-int conversions, no wrapping tests, in the loop that runs most) */
typedef struct {
    uint32_t ph, inc, inc0;              /* inc0 without the gong's glide */
    float amp, k;                        /* the level now, its fall a sample */
    float a, astep;                      /* the level it rises to, a sample's rise (0: risen) */
    uint32_t att;                        /* samples of the rise left */
    float bamp, bk, bstep;               /* a resonator's bloom on the same partial: its own level, fall, rise */
    uint32_t batt;                       /* (sharing the carrier's sine: one evaluation for both) */
    uint32_t mph, minc, minc0;           /* the modulator */
    float mi, mk;                        /* its index (turns of phase, x 2^30), its fall */
} bars_op_t;

typedef struct {
    uint8_t on, part, nops, hum;         /* hum: the last op is the gong's hum (amp - amp2 swells) */
    uint16_t gen;                        /* the stroke's serial (a held handle is stale once it moves on) */
    uint32_t delay;                      /* samples before it sounds (its place in the block) */
    uint32_t damp_in;                    /* samples until the damping hand (0xFFFFFFFF: none) */
    float damp_k, gain;                  /* the damping's fall a sample; the voice's gain (1, falling once damped) */
    float gl, gr, xsend;                 /* its place within its part; its own extra send (a white key's SHIMMER) */
    uint32_t bph, binc;                  /* the ombak's beating: cos of bph, bph += b / 2 / SR turns */
    float bcur, bstep;                   /* (it moves slowly: evaluated each 16 samples, a line between) */
    float glide, gk;                     /* the gong's pitch, from (1 + glide) down to 1 */
    float hamp2, hk2;                    /* the hum's fast exponential (subtracted: the swell) */
    float nz, nzk, nlp, nbp, nf, nq;     /* the mallet: its level, fall; the band-pass's state and coefficients */
    float level;                         /* how loud it is now (for stealing), set each block */
    bars_op_t op[BARS_OPS];
} bars_voice_t;

typedef struct {
    bars_voice_t v[BARS_VOICES];
    uint32_t rng;
    uint16_t gen;
    int cap;                             /* the most that sound at once (economy and the overload guard lower it) */
    float floor;                         /* below this a partial, and a voice, has rung out (1e-4: -80 dB; economy
                                          * raises it, so the quiet tails end sooner) */
} bars_t;

/* a stroke: what to play, and the shaping that SHAPE and the holds put on it */
typedef struct {
    uint8_t inst, part;
    float hz, vel, pan;                  /* pan -1 .. 1, within the part */
    uint32_t delay;                      /* samples into the block */
    float beat;                          /* the ombak, Hz (the instrument's own share is applied here) */
    float bright, ring, bloom, body, mallet;   /* 1: as made */
    float hum, swell;                    /* the gong's hum: its level, how slowly it blooms (1: as made) */
    float gong_ring;                     /* a gong's own RING (1: as made) */
} bars_strike_t;

typedef struct { int16_t i; uint16_t gen; } bars_handle_t;   /* a voice, while it is that stroke */

void bars_init(bars_t *b);
bars_handle_t bars_strike(bars_t *b, const bars_strike_t *s);
/* the damping hand on a stroke: from `in` samples on, it falls with time constant tau seconds */
void bars_damp(bars_t *b, bars_handle_t h, uint32_t in, float tau);
int bars_alive(const bars_t *b, bars_handle_t h);
/* a held note's bends (a white key held): its own beat, place, gain and extra send, glided by the caller */
void bars_bend(bars_t *b, bars_handle_t h, float beat, float pan, float gain, float xsend);
/* the voices [lo, hi) into the part buses (bus[part][0 L, 1 R], each n samples, added to) and xs (the extra
 * send, mono); returns how many sounded */
int bars_render(bars_t *b, int lo, int hi, float (*bus)[2][256], float *xs, int n);
int bars_active(const bars_t *b);
/* the work: sine evaluations so far, by the half of the pool that made them (0: voices from BARS_VOICES / 2, the
 * first core's; 1: below it, the second's) */
extern volatile uint32_t bars_sines[2];
const char *bars_inst_name(int inst);
