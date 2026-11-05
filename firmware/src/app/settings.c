/* SPDX-License-Identifier: GPL-3.0-only */
/* Ombak's settings (OBJ_SET) and the work playing (OBJ_WORK: everything shaped, the loop), with defaults
 * when one is missing or another firmware's. Part of the unity build (and the host's). */
#include "ombak.h"

settings_t cfg;
_Static_assert(sizeof(settings_t) == 40u, "the settings' size");
_Static_assert(sizeof(work_t) <= PLAT_STORE_MAX, "the work fits one object");

void settings_defaults(void)
{
    uint8_t *z = (uint8_t *)&cfg;
    uint32_t i;
    for (i = 0; i < sizeof cfg; i++)
        z[i] = 0;
    cfg.magic = SET_MAGIC;
    cfg.format = SET_FORMAT;
    cfg.midi_ch = 1;
    cfg.autoplay = 0;                            /* (Ombak waits for you: SEQ starts the cycle) */
    cfg.home = 0;
}

int settings_load(void)
{
    settings_t s;
    int n = plat_store_load(OBJ_SET, &s, sizeof s);
    if (n != (int)sizeof s || s.magic != SET_MAGIC || s.format != SET_FORMAT) {
        settings_defaults();                     /* none, or another firmware's */
        return -1;
    }
    if (s.midi_ch > 16)
        s.midi_ch = 1;
    s.home &= 1u;
    cfg = s;
    return 0;
}

int settings_save(void) { return plat_store_save(OBJ_SET, &cfg, sizeof cfg); }

void settings_apply(void)
{
    engine_midi_ch(cfg.midi_ch);
    engine_out_loud(cfg.out_loud);
    engine_usb_full(cfg.usb_full);
}

/* the work: every param in its range (a loaded one), else as made */
static void work_sane(work_t *w)
{
    static const uint8_t MAX_OF[][2] = {{P_ENS, G_ENSEMBLES - 1}, {P_TUNING, G_TUNINGS - 1}, {P_MODE, 3}, {P_CYCLE, 3},
                                        {P_AUTO, 1}, {P_CORE_G, 1}, {P_KOT_G, 1}, {P_PARTS, 15}, {P_K_SUB, 4},
                                        {P_K_REG, 2}, {P_R_RATE, 1}, {P_R_REG, 2}, {P_O_REG, 2}, {P_E_AFTER, 1},
                                        {P_FORM, G_FORMS - 1}, {P_K_STYLE, G_STYLES - 1}};
    int i, g;
    for (i = 0; i < NP; i++)
        w->p[i] &= 0x7Fu;
    for (i = 0; i < (int)(sizeof MAX_OF / sizeof MAX_OF[0]); i++)
        if (w->p[MAX_OF[i][0]] > MAX_OF[i][1])
            w->p[MAX_OF[i][0]] = engine_param_def(MAX_OF[i][0]);
    for (g = 0; g < G_N; g++) {
        if (w->p[P_GONG(g, GF_TONE)] > 4)
            w->p[P_GONG(g, GF_TONE)] = 2;
        if (w->p[P_GONG(g, GF_PLACES)] > 2)
            w->p[P_GONG(g, GF_PLACES)] = engine_param_def(P_GONG(g, GF_PLACES));
    }
    if (w->p[P_FX + 5] > 2)
        w->p[P_FX + 5] = 0;
    if (w->loop_len > LOOP_MAX)
        w->loop_len = 0;
    w->loop_kept = w->loop_kept && w->loop_len;
}

/* format 1 (0.1 .. 0.1.3): without the form and the style */
typedef struct {
    uint32_t magic, format;
    uint8_t p[NP_V1];
    uint8_t loop_len, loop_kept;
    int8_t loop[LOOP_MAX];
} work_v1_t;

int work_load(void)
{
    static union { work_t w; work_v1_t v1; } u;
    work_t w;
    int n = plat_store_load(OBJ_WORK, &u, sizeof u), i;
    if (n == (int)sizeof u.v1 && u.v1.magic == WORK_MAGIC && u.v1.format == 1u) {
        w.magic = WORK_MAGIC;                    /* an older work: its own, the new params as made */
        w.format = WORK_FORMAT;
        for (i = 0; i < NP; i++)
            w.p[i] = i < NP_V1 ? u.v1.p[i] : engine_param_def(i);
        w.loop_len = u.v1.loop_len;
        w.loop_kept = u.v1.loop_kept;
        for (i = 0; i < LOOP_MAX; i++)
            w.loop[i] = u.v1.loop[i];
    } else if (n == (int)sizeof u.w && u.w.magic == WORK_MAGIC && u.w.format == WORK_FORMAT) {
        w = u.w;
    } else {
        return -1;                               /* (engine_init has the defaults in place) */
    }
    work_sane(&w);
    eng_w = w;
    return 0;
}

int work_save(void) { return plat_store_save(OBJ_WORK, &eng_w, sizeof eng_w); }

/* MINE: the four slots, in RAM from the first use (one object: OBJ_USER) */
#define MINE_MAGIC 0x314D424Fu           /* "OBM1" */
static struct {
    uint32_t magic, format;
    uint8_t kept[MINE_N];
    work_t w[MINE_N];
} mine;
_Static_assert(sizeof mine <= PLAT_STORE_MAX, "MINE fits one object");
static uint8_t mine_ready;
static void mine_load(void)
{
    int i, n;
    if (mine_ready)
        return;
    mine_ready = 1;
    n = plat_store_load(OBJ_USER, &mine, sizeof mine);
    if (n != (int)sizeof mine || mine.magic != MINE_MAGIC || mine.format != WORK_FORMAT) {
        mine.magic = MINE_MAGIC;
        mine.format = WORK_FORMAT;
        for (i = 0; i < MINE_N; i++)
            mine.kept[i] = 0;
    }
    for (i = 0; i < MINE_N; i++)
        if (mine.kept[i] && (mine.w[i].magic != WORK_MAGIC || mine.w[i].format != WORK_FORMAT))
            mine.kept[i] = 0;
}
int mine_kept(int i)
{
    mine_load();
    return i >= 0 && i < MINE_N && mine.kept[i];
}
int mine_open(int i)
{
    work_t w;
    if (!mine_kept(i))
        return -1;
    w = mine.w[i];
    work_sane(&w);
    eng_w = w;
    return 0;
}
int mine_keep(int i)
{
    mine_load();
    if (i < 0 || i >= MINE_N)
        return -1;
    mine.w[i] = eng_w;
    mine.kept[i] = 1;
    return plat_store_save(OBJ_USER, &mine, sizeof mine);
}
