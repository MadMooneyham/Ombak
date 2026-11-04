/* SPDX-License-Identifier: GPL-3.0-only */
/* Ombak's gamelan: the measured tunings (sources in gamelan.c and DESIGN.md), their modes, the scale the
 * white keys play, and the five gamelan the library opens. Pure data and arithmetic: the engine and the
 * UI each keep their own scale from it. */
#pragma once
#include <stdint.h>

#define G_TUNINGS 10
#define G_ENSEMBLES 5
#define G_MAXN 7

enum { FAM_SLENDRO, FAM_PELOG };
typedef struct {
    const char *name;                    /* as the screen shows it */
    uint8_t fam, n;                      /* slendro / pelog; the tones a set (4, 5 or 7) */
    uint8_t modes;                       /* 0: none (a five- or four-tone set), 1: Bali's saih pitu, 2: Java's pathet */
    float hz;                            /* the set's first tone */
    float cents[G_MAXN];                 /* each tone above it */
    const char *const *names;            /* (sets without modes) the tones' names */
} g_tuning_t;
extern const g_tuning_t G_TUNING[G_TUNINGS];

typedef struct {
    uint8_t n;                           /* tones an octave */
    float base;                          /* the first white key, Hz (130 .. 260, or 105 .. 210 with four tones) */
    float ratio[G_MAXN];                 /* each tone above the first */
    const char *name[G_MAXN];
} g_scale_t;
int g_modes(int tuning);                 /* how many (0: none) */
const char *g_mode_name(int tuning, int mode);
void g_scale(int tuning, int mode, g_scale_t *s);
float g_pitch(const g_scale_t *s, int idx);           /* a tone, any octave: idx 0 the first white key */
int g_fold(const g_scale_t *s, int idx, float lo);    /* that tone's lowest at or above lo Hz */
int g_near(const g_scale_t *s, int idx, int ref);     /* that tone's nearest to ref */
const char *g_name(const g_scale_t *s, int idx);

/* the gamelan the library opens (and PRESETS walks): what each sets */
typedef struct {
    const char *name;
    uint8_t tuning, cycle;               /* cycle: 0 4 beats, 1 8, 2 16, 3 32 */
    uint8_t autog, core_gender, kot_gender;
    uint8_t parts;                       /* bit 0 core, 1 kotekan, 2 reyong, 3 ornaments */
    uint8_t ombak, tempo, breath, density;   /* the params' own scales (engine.h) */
    uint8_t space;                       /* 0xFF: as it is */
    uint8_t kajar;                       /* the kajar's places */
    uint8_t ring;                        /* the bars' RING (64 as made) */
} g_ensemble_t;
extern const g_ensemble_t G_ENSEMBLE[G_ENSEMBLES];

/* the cycle's places: what a place is (for the rim's marks), and what the cycle strikes there */
enum { PL_GONG, PL_KEMONG, PL_KEMPUR, PL_KAJAR };
int g_place(int pos, int len);
