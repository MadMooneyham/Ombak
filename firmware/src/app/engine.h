/* SPDX-License-Identifier: GPL-3.0-only */
/* Ombak's audio engine. engine_render() runs in the audio ISR; the main loop reaches it through the
 * command queue and the work it shapes (eng_w, a byte at a time: the ISR reads it each block), as
 * Penumbra's does.
 *
 * What it plays (DESIGN.md has it all):
 *   the cycle       a beat of 3 to 96 a minute, the gong cycle of 4, 8, 16 or 32 beats, its gongs kept
 *                   by the cycle or left to the hand; a mode or gamelan changes at the gong
 *   the core        each white key you play sounds your gendèr at once and is the next core note: the core
 *                   (jublag and jegogan, or gendèr) strikes it on the next beat
 *   the elaboration the kotekan (polos and sangsih, on a grid of 16 a beat), the reyong (norot-like) and
 *                   the gendèr's ornaments, each leaving the core note and turning toward the next
 *   the motion      TIDES (one slow swell: the space, the swell, the ombak), PAN (each part's place and
 *                   sway), the breath (the beats slowing into the gong), the cues, the ending, the loop
 *   the space       Penumbra's shimmer reverb (fxbus, its delay unused) and master stage
 * The voices (dsp/bars.c) render in two halves, one on each core. */
#pragma once
#include <stdint.h>

/* ----------------------------------------------------------------- the work --- */
/* everything that shapes the sound, a byte each (0 .. 127 unless said), saved (settings.c: OBJ_WORK) */
enum {
    /* the gamelan playing */
    P_ENS, P_TUNING, P_MODE, P_CYCLE,    /* CYCLE: 0 4 beats, 1 8, 2 16, 3 32 */
    P_AUTO,                              /* 1: the cycle keeps the gongs (ARP) */
    P_CORE_G, P_KOT_G,                   /* 1: the core / the kotekan on gendèr (else jublag / gangsa) */
    P_PARTS,                             /* bit 0 core, 1 kotekan, 2 reyong, 3 ornaments, 4 kendang, 5 ceng-ceng,
                                          * 6 suling */
    /* HOME's knobs and the tempo */
    P_OMBAK,                             /* 0.25 .. 10 Hz: 0.25 x 40^(p/127) */
    P_DENSITY, P_SPACE, P_BREATH,
    P_TEMPO,                             /* 3 .. 96 core beats a minute: 3 x 32^(p/127) */
    /* ENSEMBLE: the levels (100: as made) */
    P_L_GENDER, P_L_CORE, P_L_KOT, P_L_REY, P_L_ORN, P_L_GONG,
    /* the families' timbre (64: as made) */
    P_TG_BRIGHT, P_TG_RING, P_TG_BLOOM, P_TG_DAMP,                  /* GENDÈR */
    P_TB_BRIGHT, P_TB_RING, P_TB_MALLET, P_TB_BLOOM,                /* BARS */
    P_TR_BRIGHT, P_TR_RING, P_TR_MALLET,                            /* REYONG */
    P_TO_HUM, P_TO_SWELL, P_TO_BODY, P_TO_OMBAK,                    /* GONGS */
    /* each gong, held: LEVEL (100), TONE (0..4: -2..+2), HUM, RING (64), PLACES (0 off, 1 as made, 2 every other) */
    P_GONGS, P_GONGS_END = P_GONGS + 5 * 5 - 1,
    /* the parts, held: the kotekan's DENSITY (0 HOME's, 1..4: 2 4 8 16 a beat), REGISTER (0..2), TURN, SPARSE;
     * the reyong's RATE (0 half, 1 a quarter), REGISTER, TURN, SPARSE; the ornaments' PICKUP, OCTAVES, REGISTER,
     * SPARSE */
    P_K_SUB, P_K_REG, P_K_TURN, P_K_SPARSE,
    P_R_RATE, P_R_REG, P_R_TURN, P_R_SPARSE,
    P_O_PICK, P_O_OCT, P_O_REG, P_O_SPARSE,
    /* the cues, held: STEP, GLIDE; the ending: AFTER (0 the next gong, 1 the one after), SLOWING, THINNING */
    P_C_STEP, P_C_GLIDE, P_E_AFTER, P_E_SLOW, P_E_THIN,
    /* TIDES: RATE (8 s .. 4 min), SHAPE, SPACE, SWELL, OMBAK */
    P_T_RATE, P_T_SHAPE, P_T_SPACE, P_T_SWELL, P_T_OMBAK,
    /* PAN: each part's place (64 the middle) and sway; RATE (0 the tide), OFFSET */
    P_PAN, P_PAN_END = P_PAN + 3, P_SWAY, P_SWAY_END = P_SWAY + 3, P_S_RATE, P_S_OFF,
    /* FX: DECAY, SIZE, SHIMMER, TONE, MOTION, INTERVAL (0..2), LOW CUT, LEVEL */
    P_FX, P_FX_END = P_FX + 7,
    /* (format 2) the gong form (G_FORM: 0 FREE, the cycle's length CYCLE's), the kotekan's style (G_STYLE) */
    P_FORM, P_K_STYLE,
    /* (format 3) DRUMS: the kendang's and the ceng-ceng's levels (100 as made), BUSY, SLAP (64 as made);
     * SULING: its level, BREATH, VIBRATO, REGISTER (0..2), GLIDE */
    P_L_KEN, P_L_CENG, P_D_BUSY, P_D_SLAP,
    P_L_SUL, P_S_BREATH, P_S_VIB, P_S_REG, P_S_GLIDE,
    NP
};
#define NP_V1 P_FORM                     /* the params a format 1 work has */
#define NP_V2 P_L_KEN                    /* a format 2's */
enum { G_GONG, G_LANANG, G_KEMPUR, G_KEMONG, G_KAJAR, G_N };
enum { GF_LEVEL, GF_TONE, GF_HUM, GF_RING, GF_PLACES, GF_N };
#define P_GONG(g, f) (P_GONGS + (g) * GF_N + (f))
enum { PART_GENDER, PART_CORE, PART_KOT, PART_GONGS, NPARTS };
enum { PB_CORE = 1, PB_KOT = 2, PB_REY = 4, PB_ORN = 8, PB_KEN = 16, PB_CENG = 32, PB_SUL = 64 };

#define WORK_MAGIC 0x3157424Fu           /* "OBW1" */
#define WORK_FORMAT 3u
#define LOOP_MAX 16
typedef struct {
    uint32_t magic, format;
    uint8_t p[NP];
    uint8_t loop_len, loop_kept;         /* the core's loop (kept: it has notes) */
    int8_t loop[LOOP_MAX];               /* its notes (-128: none) */
} work_t;
extern work_t eng_w;
uint8_t engine_param_def(int i);         /* as made */
void engine_work_defaults(void);
void engine_ensemble(int e);             /* (main loop, stopped) a gamelan's settings into eng_w */
void engine_form(int f);                 /* the gong form (and the lanang with it) */
void engine_angsel(void);                /* an angsel: the next beat breaks, the ensemble accenting together */
void engine_cc_out(int cc, int v);       /* MIDI CC OUT: a control change (128 + n: program change n) */

/* -------------------------------------------------------------- what it is doing --- */
extern volatile uint8_t eng_running, eng_ending, eng_pos, eng_loop_state;
enum { LS_OFF, LS_ARM, LS_REC, LS_PLAY, LS_HELD };
extern volatile int8_t eng_pend_mode, eng_pend_ens;   /* arriving at the next gong (-1: none) */
extern volatile int16_t eng_core, eng_next;           /* the core note now, the next (-32768: none) */
extern volatile uint32_t eng_frames;                  /* samples since power-on */
extern volatile uint32_t eng_beat_at, eng_beat_len;   /* the beat now: its start, its length (0: the last) */
extern volatile uint8_t eng_beat_pos;                 /* the beat now: its place in the cycle */
extern volatile uint32_t eng_gong_at;                 /* the last gong's stroke */
extern volatile uint32_t eng_cycle_no;
extern volatile float eng_bpm_now, eng_tide, eng_swell;   /* the tempo now; the tide -1..1; HOME's swell 0..1 */
extern volatile float eng_pan_now[NPARTS];            /* each part's place now, -1 .. 1 */
#define BEADS 32
extern volatile uint32_t eng_bead_at[BEADS];          /* each core note's beat: when, and where in the cycle */
extern volatile uint8_t eng_bead_pos[BEADS], eng_bead_w;
extern volatile uint16_t eng_level;                   /* the output's peak, Q15 */
extern uint32_t eng_out_clips;
int engine_cycle_len(void);
int engine_voices(void);                              /* voices sounding */

/* ----------------------------------------------------------- the hands (main loop) --- */
void engine_run(int on);                 /* SEQ: the cycle starts (with the gong) or stops (it rings out) */
void engine_white(int k, int down);      /* a white key, 0..15, with the octave (engine_oct) */
void engine_oct(int oct);
void engine_gong(int g);                 /* a gong by hand */
void engine_end(void);                   /* the ending (FASTER and SLOWER move P_TEMPO: the cycle glides to it) */
void engine_pend_mode(int m);            /* the mode at the next gong (stopped: now) */
void engine_pend_ens(int e);             /* the gamelan at the next gong (stopped: main loop applies it) */
enum { LOOP_REC, LOOP_PLAY, LOOP_CLEAR };
void engine_loop(int op);
/* a white key held: its note's bends, SHIMMER 0..127, OMBAK 0..127 (42 as made: x1), PLACE (64 middle), SWELL (80: as made) */
void engine_bend(int k, int which, int value);
/* the throws (FX held): SHIMMER, FREEZE, HUSH 0..127; DIP -127 .. 127; OMBAK -127 .. 127 (x 4^(v/127)) */
enum { TH_SHIMMER, TH_FREEZE, TH_DIP, TH_HUSH, TH_OMBAK, TH_N };
void engine_throw(int i, int value);
void engine_load_test(int on);
int engine_load_testing(void);
void engine_out_loud(int on);
void engine_usb_full(int on);
void engine_midi_ch(int ch);
void engine_midi_opts(int clock_out, int clock_in, int notes_out, int cc_out);
int engine_following(void);              /* 1: an incoming MIDI clock leads the tempo now */
uint32_t engine_quiet_ms(void);

/* --------------------------------------------------------------- the benchmark --- */
enum { ST_CTRL, ST_VOICES1, ST_VOICES2, ST_FXWET, ST_WAIT, ST_OUT, ST_CORE1, ST_CORE2, ST_N };
typedef struct {
    uint16_t avg[ST_N], peak[ST_N];
    uint32_t blocks;
    uint8_t two_cores, voices;
    uint16_t ksines[2];                  /* the voices' sine evaluations that second, thousands: core 1's, core 2's */
} bench_t;
uint32_t engine_bench(bench_t *out);
const char *engine_stage_name(int st);
void engine_load(uint32_t pct, uint32_t frames);
int engine_guard_active(void);
extern volatile uint32_t eng_guard_count;
int engine_economy(void);                /* economy: under load, the quiet tails end sooner, fewer voices at once */
extern volatile uint32_t eng_econ_count;

void engine_init(void);
void engine_render(int32_t *out_lr, uint32_t n);
void engine_volume(int32_t *out_lr, uint32_t n);
