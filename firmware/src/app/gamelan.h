/* SPDX-License-Identifier: GPL-3.0-only */
/* Ombak's gamelan: the measured tunings (sources in gamelan.c and DESIGN.md), their modes, the scale the
 * white keys play, and the five gamelan the library opens. Pure data and arithmetic: the engine and the
 * UI each keep their own scale from it. */
#pragma once
#include <stdint.h>

#define G_TUNINGS 10
#define G_ENSEMBLES 8
#define G_FORMS 5
#define G_STYLES 4
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
    uint8_t parts;                       /* bit 0 core, 1 kotekan, 2 reyong, 3 ornaments, 4 kendang, 5 ceng-ceng,
                                          * 6 suling */
    uint8_t ombak, tempo, breath, density;   /* the params' own scales (engine.h) */
    uint8_t space;                       /* 0xFF: as it is */
    uint8_t kajar;                       /* the kajar's places */
    uint8_t ring;                        /* the bars' RING (64 as made) */
    uint8_t form, style;                 /* the gong form (G_FORM), the kotekan's style */
} g_ensemble_t;
extern const g_ensemble_t G_ENSEMBLE[G_ENSEMBLES];

/* the gong forms (colotomic structures): Ombak's own (any cycle: the gong, the half, the quarters, the kajar
 * every beat) and the forms from the tradition, each its own length. A line a gong, one character a beat
 * from the gong's (place 0): 'x' a stroke, 'o' a light one, '.' none. The gongs in order: gong, lanang,
 * kempur, kemong, kajar (the forms' kempul strikes the kempur, the kenong the kemong, the ketuk and
 * kempyang the kajar, the kempli the kajar) */
typedef struct {
    const char *name;
    uint8_t len;                         /* 0: the cycle's (P_CYCLE) */
    const char *line[5];
} g_form_t;
extern const g_form_t G_FORM[G_FORMS];
int g_strike(int form, int g, int pos, int len);      /* 0 none, 1 light, 2 a stroke */
/* the cycle's places: what a place is (for the rim's marks) */
enum { PL_GONG, PL_KEMONG, PL_KEMPUR, PL_KAJAR };
int g_place(int form, int pos, int len);
/* the kotekan's styles */
enum { KS_CIRCLE, KS_NOROT, KS_NYOGCAG, KS_EMPAT };
extern const char *const G_STYLE[G_STYLES];
