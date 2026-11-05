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
    cfg.clock_out = 1;
    cfg.clock_in = 1;
    cfg.notes_out = 0;
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
    s.clock_out &= 1u;
    s.clock_in &= 1u;
    s.notes_out &= 1u;
    s.cc_out &= 1u;
    cfg = s;
    return 0;
}

int settings_save(void) { return plat_store_save(OBJ_SET, &cfg, sizeof cfg); }

void settings_apply(void)
{
    engine_midi_ch(cfg.midi_ch);
    engine_midi_opts(cfg.clock_out, cfg.clock_in, cfg.notes_out, cfg.cc_out);
    engine_out_loud(cfg.out_loud);
    engine_usb_full(cfg.usb_full);
}

/* the work: every param in its range (a loaded one), else as made */
static void work_sane(work_t *w)
{
    static const uint8_t MAX_OF[][2] = {{P_ENS, G_ENSEMBLES - 1}, {P_TUNING, G_TUNINGS - 1}, {P_MODE, 3}, {P_CYCLE, 3},
                                        {P_AUTO, 1}, {P_CORE_G, 1}, {P_KOT_G, 1}, {P_PARTS, 127}, {P_K_SUB, 4},
                                        {P_K_REG, 2}, {P_R_RATE, 1}, {P_R_REG, 2}, {P_O_REG, 2}, {P_E_AFTER, 1},
                                        {P_FORM, G_FORMS - 1}, {P_K_STYLE, G_STYLES - 1}, {P_S_REG, 2}, {P_IRON, 1}};
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

/* A work as saved, any format: magic, format, its params (format 1: NP_V1 of them, 1.0's format 2: NP_V2,
 * format 3: NP), the loop's length and kept, the loop; padded to 4 bytes. An older one is read with the
 * params it did not have as made. */
static int work_np(uint32_t format)
{
    return format == 1u ? NP_V1 : format == 2u ? NP_V2 : format == 3u ? NP_V3 : format == WORK_FORMAT ? NP : -1;
}
static uint32_t work_size(int np) { return (8u + (uint32_t)np + 2u + LOOP_MAX + 3u) & ~3u; }
_Static_assert(((8u + NP + 2u + LOOP_MAX + 3u) & ~3u) == sizeof(work_t), "the work's layout");
static int work_read(const uint8_t *b, uint32_t len, work_t *w)
{
    uint32_t magic = b[0] | b[1] << 8 | b[2] << 16 | (uint32_t)b[3] << 24;
    uint32_t format = b[4] | b[5] << 8 | b[6] << 16 | (uint32_t)b[7] << 24;
    int np = work_np(format), i;
    if (magic != WORK_MAGIC || np < 0 || len < work_size(np))
        return -1;
    w->magic = WORK_MAGIC;
    w->format = WORK_FORMAT;
    for (i = 0; i < NP; i++)
        w->p[i] = i < np ? b[8 + i] : engine_param_def(i);
    w->loop_len = b[8 + np];
    w->loop_kept = b[9 + np];
    for (i = 0; i < LOOP_MAX; i++)
        w->loop[i] = (int8_t)b[10 + np + i];
    work_sane(w);
    return 0;
}

int work_load(void)
{
    static uint8_t buf[sizeof(work_t)];
    work_t w;
    int n = plat_store_load(OBJ_WORK, buf, sizeof buf);
    if (n < 8 || work_read(buf, (uint32_t)n, &w))
        return -1;                               /* (engine_init has the defaults in place) */
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
    static uint8_t buf[sizeof mine];
    int i, n, np;
    uint32_t format, sz;
    if (mine_ready)
        return;
    mine_ready = 1;
    mine.magic = MINE_MAGIC;
    mine.format = WORK_FORMAT;
    for (i = 0; i < MINE_N; i++)
        mine.kept[i] = 0;
    n = plat_store_load(OBJ_USER, buf, sizeof buf);
    if (n < 12 || (buf[0] | buf[1] << 8 | buf[2] << 16 | (uint32_t)buf[3] << 24) != MINE_MAGIC)
        return;
    format = buf[4] | buf[5] << 8 | buf[6] << 16 | (uint32_t)buf[7] << 24;
    if ((np = work_np(format)) < 0)
        return;
    sz = work_size(np);                          /* the slots, each a work of that format, after the header */
    for (i = 0; i < MINE_N; i++) {
        uint32_t at = 8u + MINE_N + (uint32_t)i * sz;
        if (buf[8 + i] && (uint32_t)n >= at + sz && !work_read(buf + at, sz, &mine.w[i]))
            mine.kept[i] = 1;
    }
}
_Static_assert(sizeof mine == 8u + MINE_N + MINE_N * sizeof(work_t), "MINE's layout");
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
