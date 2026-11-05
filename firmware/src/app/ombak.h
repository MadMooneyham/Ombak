/* SPDX-License-Identifier: GPL-3.0-only */
/* Ombak: the settings, the work saved, and the app's entry points.
 * Included by the unity build after gfx.c, so drawing helpers are in scope. */
#pragma once
#include <stdint.h>
#include "plat.h"
#include "engine.h"
#include "gamelan.h"

#ifndef OMB_VERSION                      /* a release build passes its own (tools/build.py --release) */
#define OMB_VERSION "DEV"
#endif
#define FW_NAME "OMBAK"                  /* the name on the screen and in the reader's HELLO answer */

#define SET_MAGIC 0x3153424Fu            /* "OBS1": another firmware's settings load as defaults */
#define SET_FORMAT 1u
typedef struct {
    uint32_t magic, format;
    uint8_t midi_ch;                     /* 1-16, 0 off (MIDI in: engine.c midi_in) */
    uint8_t one_core;                    /* 1: the second core stays off (applies at the next power-on) */
    uint8_t out_loud;                    /* 1: the output 6 dB hotter */
    uint8_t usb_full;                    /* 1: USB audio at full level, MASTER for the FM-1's own outputs */
    uint8_t autoplay;                    /* 1: the cycle starts at power-on */
    uint8_t home;                        /* HOME's heart: 0 the Ongkara, 1 the gong */
    uint8_t clock_out;                   /* 1: MIDI clock and Start / Stop out while the cycle runs */
    uint8_t clock_in;                    /* 1: an incoming MIDI clock leads the tempo */
    uint8_t notes_out;                   /* 1: the FM-1's keys go out as MIDI notes */
    uint8_t rsv[23];
} settings_t;

extern settings_t cfg;
void settings_defaults(void);
int settings_load(void);                 /* 0 = loaded; else the defaults are in place */
int settings_save(void);                 /* 0 = saved */
void settings_apply(void);
int work_load(void);                     /* the work (eng_w) saved: 0 = loaded */
int work_save(void);
/* MINE: four works of your own in the library (OBJ_USER), kept with SAVE there */
#define MINE_N 4
int mine_kept(int i);                    /* 1: slot i has a work */
int mine_open(int i);                    /* slot i into eng_w: 0 = done */
int mine_keep(int i);                    /* eng_w into slot i (and the flash): 0 = done */

void ui_init(void);
void ui_start(void);                     /* once, after boot: PLAY AT POWER-ON */
void ui_frame(void);
void ui_input_only(void);
void ui_say(const char *a, const char *b);
