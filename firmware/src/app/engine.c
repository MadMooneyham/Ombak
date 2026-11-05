/* SPDX-License-Identifier: GPL-3.0-only */
/* Ombak's audio engine; see engine.h. The cycle, the core and the elaboration are the browser prototype's
 * (prototype/index.html: beginBeat, elaborate, kotekanNote), the tides and the pan Penumbra's (its
 * engine.c), the render its two-core pattern. */
#include "engine.h"
#include "plat.h"
#include "gamelan.h"
#include "../dsp/bars.h"
#include "../dsp/fxbus.h"
#include "../dsp/master.h"
#include "../dsp/fastmath.h"

#define POOL OMB_POOL
#define SR 44100.0f
#define NONE (-32768)
#define GRID 16                                  /* the beat's grid: each part plays on its own within it */

/* ---------------------------------------------------------------- state --- */
work_t eng_w;
volatile uint8_t eng_running, eng_ending, eng_pos, eng_loop_state;
volatile int8_t eng_pend_mode = -1, eng_pend_ens = -1;
volatile int16_t eng_core = NONE, eng_next = NONE;
volatile uint32_t eng_frames, eng_beat_at, eng_beat_len, eng_gong_at, eng_cycle_no;
volatile uint8_t eng_beat_pos;
volatile float eng_bpm_now = 40.0f, eng_tide, eng_swell;
volatile float eng_pan_now[NPARTS];
volatile uint32_t eng_bead_at[BEADS];
volatile uint8_t eng_bead_pos[BEADS], eng_bead_w;
volatile uint16_t eng_level;
uint32_t eng_out_clips;

static bars_t bars POOL;
static fxbus_t fx POOL;
static master_t mst;
#define DLY_LEN 4096                             /* (the bus's delay is not used: a token line) */
static int16_t dly_buf[DLY_LEN] POOL;
static int16_t rv_buf[SV_TOTAL] POOL, rv_ps[SV_PS_LEN] POOL;
static float bus1[NPARTS][2][256] POOL, bus2[NPARTS][2][256] POOL, xs1[256] POOL, xs2[256] POOL;
static float out_l[256] POOL, out_r[256] POOL, rev[256] POOL, mono[256] POOL;
static float fx_send[2][256] POOL, wet_l[256] POOL, wet_r[256] POOL, zero[256] POOL;
static uint8_t fx_par;
_Static_assert(PLAT_TICKS_PER_BLOCK(256) == 139319u, "the block's budget in 24 MHz ticks");

/* ------------------------------------------------------------- as made --- */
static uint8_t DEF[NP];
uint8_t engine_param_def(int i) { return i >= 0 && i < NP ? DEF[i] : 0; }
static void defaults_init(void)
{
    static const uint8_t PAN_DEF[NPARTS] = {64, 54, 70, 64}, SWAY_DEF[NPARTS] = {32, 0, 0, 0};
    static const uint8_t FX_DEF[8] = {64, 76, 32, 64, 64, 0, 62, 106};
    int i, g;
    for (i = 0; i < NP; i++)
        DEF[i] = 64;
    DEF[P_OMBAK] = 99;                           /* 4.5 Hz */
    DEF[P_DENSITY] = 64;
    DEF[P_SPACE] = 57;
    DEF[P_BREATH] = 44;
    DEF[P_TEMPO] = 95;                           /* 40 a minute */
    for (i = P_L_GENDER; i <= P_L_GONG; i++)
        DEF[i] = 100;
    for (g = 0; g < G_N; g++) {
        DEF[P_GONG(g, GF_LEVEL)] = 100;
        DEF[P_GONG(g, GF_TONE)] = 2;
        DEF[P_GONG(g, GF_PLACES)] = g == G_LANANG ? 0 : 1;
    }
    DEF[P_K_SUB] = 0;
    DEF[P_K_REG] = DEF[P_R_REG] = DEF[P_O_REG] = 1;
    DEF[P_K_SPARSE] = DEF[P_R_SPARSE] = DEF[P_O_SPARSE] = 0;
    DEF[P_R_RATE] = 0;
    DEF[P_O_PICK] = 18;                          /* the pickup an eighth of the beat early */
    DEF[P_C_STEP] = 79;                          /* a third */
    DEF[P_C_GLIDE] = 35;                         /* the ensemble follows 0.3 of the way a beat */
    DEF[P_E_AFTER] = 0;
    DEF[P_T_RATE] = 67;                          /* 50 s a tide */
    DEF[P_T_SHAPE] = 38;
    DEF[P_T_SPACE] = 51;
    DEF[P_T_SWELL] = 38;
    DEF[P_T_OMBAK] = 25;
    for (i = 0; i < NPARTS; i++) {
        DEF[P_PAN + i] = PAN_DEF[i];
        DEF[P_SWAY + i] = SWAY_DEF[i];
    }
    DEF[P_S_RATE] = 0;
    DEF[P_S_OFF] = 0;
    for (i = 0; i < 8; i++)
        DEF[P_FX + i] = FX_DEF[i];
    DEF[P_FORM] = 0;
    DEF[P_K_STYLE] = KS_CIRCLE;
    DEF[P_L_KEN] = DEF[P_L_CENG] = DEF[P_L_SUL] = 100;
    DEF[P_S_REG] = 1;
}
/* a form with the lanang in it brings it in (FREE leaves it to its PLACES: off as made) */
void engine_form(int f)
{
    int i, lan = 0;
    f = (int)((unsigned)f % G_FORMS);
    eng_w.p[P_FORM] = (uint8_t)f;
    for (i = 0; i < G_FORM[f].len; i++)
        lan |= g_strike(f, G_LANANG, i, G_FORM[f].len);
    eng_w.p[P_GONG(G_LANANG, GF_PLACES)] = (uint8_t)(lan ? 1 : 0);
}

void engine_ensemble(int e)
{
    const g_ensemble_t *g = &G_ENSEMBLE[(unsigned)e % G_ENSEMBLES];
    uint8_t *p = eng_w.p;
    p[P_ENS] = (uint8_t)((unsigned)e % G_ENSEMBLES);
    p[P_TUNING] = g->tuning;
    p[P_MODE] = 0;
    p[P_CYCLE] = g->cycle;
    p[P_AUTO] = g->autog;
    p[P_CORE_G] = g->core_gender;
    p[P_KOT_G] = g->kot_gender;
    p[P_PARTS] = g->parts;
    p[P_OMBAK] = g->ombak;
    p[P_TEMPO] = g->tempo;
    p[P_BREATH] = g->breath;
    p[P_DENSITY] = g->density;
    if (g->space != 0xFF)
        p[P_SPACE] = g->space;
    p[P_GONG(G_KAJAR, GF_PLACES)] = g->kajar;
    p[P_TG_RING] = p[P_TB_RING] = g->ring;
    p[P_K_STYLE] = g->style;
    engine_form(g->form);
}

void engine_work_defaults(void)
{
    int i;
    eng_w.magic = WORK_MAGIC;
    eng_w.format = WORK_FORMAT;
    for (i = 0; i < NP; i++)
        eng_w.p[i] = DEF[i];
    engine_ensemble(1);                          /* semar pegulingan */
    eng_w.loop_len = 0;
    eng_w.loop_kept = 0;
    for (i = 0; i < LOOP_MAX; i++)
        eng_w.loop[i] = -128;
}

/* the params, as numbers */
#define PV(i) ((float)eng_w.p[i])
static float x128(int i) { return PV(i) * (1.0f / 128.0f); }               /* 64: 0.5, as made */
static float x127(int i) { return PV(i) * (1.0f / 127.0f); }
static float e2(int i) { return fm_exp2f((x128(i) - 0.5f) * 2.0f); }       /* as made 1, x2 or /2 at the ends */
static float e4(int i) { return fm_exp2f((x128(i) - 0.5f) * 4.0f); }       /* x4 or /4 (RING: slow playing's long rings) */
static float lvl(int i) { return PV(i) * 0.01f; }
static int cycle_len(void)
{
    static const uint8_t L[4] = {4, 8, 16, 32};
    const g_form_t *f = &G_FORM[eng_w.p[P_FORM] % G_FORMS];
    return f->len ? f->len : L[eng_w.p[P_CYCLE] & 3];
}
int engine_cycle_len(void) { return cycle_len(); }
static float target_bpm(void) { return 3.0f * fm_exp2f(PV(P_TEMPO) * (5.0f / 127.0f)); }

/* ------------------------------------------------------------ benchmark --- */
static uint32_t st_t[ST_N];
static struct {
    float sum[ST_N], budget;
    uint16_t peak[ST_N];
    uint32_t blocks, frames;
} acc;
static bench_t pub;
static volatile uint32_t pub_serial;
static const char *const ST_NAME[ST_N] = {"CTRL", "VOICES 1", "VOICES 2", "FX WET", "WAIT", "OUTPUT", "CORE 1", "CORE 2"};
const char *engine_stage_name(int st) { return st >= 0 && st < ST_N ? ST_NAME[st] : ""; }
uint32_t engine_bench(bench_t *out)
{
    uint32_t s, tries;
    for (tries = 0; tries < 4u; tries++) {
        s = pub_serial;
        *out = pub;
        if (s == pub_serial)
            break;
    }
    return s;
}
static void bench_block(uint32_t n)
{
    float budget = (float)PLAT_TICKS_PER_BLOCK(n);
    uint32_t k;
    for (k = 0; k < ST_N; k++) {
        uint32_t p10 = (uint32_t)((float)st_t[k] * 1000.0f / budget);
        acc.sum[k] += (float)st_t[k];
        if (p10 > acc.peak[k])
            acc.peak[k] = (uint16_t)(p10 > 65535u ? 65535u : p10);
    }
    acc.budget += budget;
    acc.blocks++;
    acc.frames += n;
    if (acc.frames >= 44100u) {
        for (k = 0; k < ST_N; k++) {
            pub.avg[k] = (uint16_t)(acc.sum[k] * 1000.0f / acc.budget + 0.5f);
            pub.peak[k] = acc.peak[k];
            acc.sum[k] = 0.0f;
            acc.peak[k] = 0;
        }
        pub.blocks = acc.blocks;
        pub.two_cores = (uint8_t)plat_cpu2_on();
        pub.voices = (uint8_t)bars_active(&bars);
        {
            static uint32_t last[2];
            for (k = 0; k < 2; k++) {
                uint32_t s = bars_sines[k];
                pub.ksines[k] = (uint16_t)((s - last[k]) / 1000u);
                last[k] = s;
            }
        }
        acc.budget = 0.0f;
        acc.blocks = acc.frames = 0;
        __asm__ volatile("" ::: "memory");
        pub_serial++;
    }
}

/* ------------------------------------------------------- overload guard --- */
/* past the CPU (a block over 92%, or 85% on average) fewer voices sound at once (the quietest give way);
 * after 3 s back under 65% they may all again. Off during the load test */
#define GUARD_ON_LAST 92u
#define GUARD_ON_AVG 85u
#define GUARD_OFF 65u
#define GUARD_CALM_FRAMES (3u * 44100u)
#define GUARD_VOICES 20
/* ECONOMY, before that: past 70% on average the quiet tails end sooner (at -60 dB, not -80) and the voices that
 * may sound at once step down, a voice every 8 blocks, to no fewer than 16 (new strokes take the quietest);
 * after 1 s under 55% they step back up. In a full mix the tails it cuts are not heard */
#define ECON_ON 70u
#define ECON_OFF 55u
#define ECON_CALM_FRAMES 44100u
#define ECON_FLOOR 16
static uint8_t econ_on;
static uint32_t econ_calm;
static int pool_cap = BARS_VOICES;
volatile uint32_t eng_econ_count;
int engine_economy(void) { return econ_on; }
static uint8_t guard_on, guard_enabled = 1;
static uint32_t guard_avg_q8, guard_calm;
volatile uint32_t eng_guard_count;
int engine_guard_active(void) { return guard_on; }
static void economy_block(void)                  /* (CTRL) the pool's cap and floor for this block */
{
    static uint32_t blk;
    int on = bars_active(&bars);
    blk++;
    if (econ_on || guard_on) {
        if (!(blk & 7u) && pool_cap > ECON_FLOOR)
            pool_cap = (on < pool_cap ? on : pool_cap) - 1;
        if (pool_cap < ECON_FLOOR)
            pool_cap = ECON_FLOOR;
    } else if (pool_cap < BARS_VOICES && !(blk & 7u)) {
        pool_cap++;
    }
    bars.cap = guard_on && pool_cap > GUARD_VOICES ? GUARD_VOICES : pool_cap;
    bars.floor = econ_on || guard_on ? 1e-3f : 1e-4f;
}
void engine_load(uint32_t pct, uint32_t frames)
{
    guard_avg_q8 = (guard_avg_q8 * 15u + pct * 256u) / 16u;
    if (!guard_enabled) {
        econ_on = 0;
    } else if (!econ_on) {
        if (guard_avg_q8 > ECON_ON * 256u) {
            econ_on = 1;
            econ_calm = 0;
            eng_econ_count++;
        }
    } else if (guard_avg_q8 >= ECON_OFF * 256u) {
        econ_calm = 0;
    } else if ((econ_calm += frames) >= ECON_CALM_FRAMES) {
        econ_on = 0;
    }
    if (!guard_enabled) {
        guard_on = 0;
        guard_calm = 0;
    } else if (!guard_on) {
        if (pct > GUARD_ON_LAST || guard_avg_q8 > GUARD_ON_AVG * 256u) {
            guard_on = 1;
            guard_calm = 0;
            eng_guard_count++;
        }
    } else if (pct >= GUARD_OFF || guard_avg_q8 >= GUARD_OFF * 256u) {
        guard_calm = 0;
    } else if ((guard_calm += frames) >= GUARD_CALM_FRAMES) {
        guard_on = 0;
    }
}

/* ------------------------------------------------------- command queue --- */
/* single producer (main loop), single consumer (audio ISR): type:4 | a:8 | b:8 | c:12 */
enum { C_RUN, C_WHITE, C_OCT, C_GONG, C_END, C_PMODE, C_PENS, C_LOOP, C_BEND, C_THROW, C_TEST, C_ANGSEL, C_CC };
#define CQ 256u
static uint32_t cq[CQ];
static volatile uint32_t cq_w, cq_r;
static void cq_put(uint32_t type, uint32_t a, uint32_t b, uint32_t c)
{
    uint32_t w = cq_w;
    if (w - cq_r >= CQ)
        return;
    cq[w % CQ] = (type << 28) | ((a & 0xFFu) << 20) | ((b & 0xFFu) << 12) | (c & 0xFFFu);
    __asm__ volatile("" ::: "memory");
    cq_w = w + 1;
}
void engine_run(int on) { cq_put(C_RUN, (uint32_t)(on != 0), 0, 0); }
void engine_white(int k, int down) { cq_put(C_WHITE, (uint32_t)k, (uint32_t)(down != 0), 0); }
void engine_oct(int oct) { cq_put(C_OCT, 0, 0, (uint32_t)(oct + 8)); }
void engine_gong(int g) { cq_put(C_GONG, (uint32_t)g, 0, 0); }
void engine_end(void) { cq_put(C_END, 0, 0, 0); }
void engine_pend_mode(int m) { cq_put(C_PMODE, (uint32_t)(m + 1), 0, 0); }
void engine_pend_ens(int e) { cq_put(C_PENS, (uint32_t)(e + 1), 0, 0); }
void engine_loop(int op) { cq_put(C_LOOP, (uint32_t)op, 0, 0); }
void engine_bend(int k, int which, int value) { cq_put(C_BEND, (uint32_t)k, (uint32_t)which, (uint32_t)value); }
void engine_throw(int i, int value) { cq_put(C_THROW, (uint32_t)i, 0, (uint32_t)(value + 128)); }
static volatile uint8_t testing;
void engine_load_test(int on) { cq_put(C_TEST, (uint32_t)(on != 0), 0, 0); }
void engine_angsel(void) { cq_put(C_ANGSEL, 0, 0, 0); }
void engine_cc_out(int cc, int v) { cq_put(C_CC, (uint32_t)cc, (uint32_t)v, 0); }
int engine_load_testing(void) { return testing; }
static uint8_t midi_ch = 1;
void engine_midi_ch(int ch) { midi_ch = (uint8_t)ch; }

/* ---------------------------------------------------------- MIDI clock --- */
/* 24 ticks a core beat. Out: while the cycle runs (and no clock is followed), the ticks spread over each beat as
 * it is played (the breath and the ending's slowing in them), Start and Stop with the cycle. In: the ticks carry
 * the time they came (usb.c, midi_uart.c: Penumbra's stamps); the tempo is the span of the last two beats' of
 * them, and each beat's length is nudged by how far the cycle is from the ticks' count, so the cycle stays in
 * phase (a beat at most half as long again or as short). Half a second without a tick: Ombak's own tempo again */
#define CLK_STAMP_HZ 5859.375f
#define CLK_SPAN 48u
static uint8_t clk_out = 1, clk_in = 1, note_send, cc_send;
void engine_midi_opts(int clock_out, int clock_in, int n_out, int c_out)
{
    clk_out = (uint8_t)(clock_out != 0);
    clk_in = (uint8_t)(clock_in != 0);
    note_send = (uint8_t)(n_out != 0);
    cc_send = (uint8_t)(c_out != 0);
}
static struct {
    uint16_t stamp[CLK_SPAN + 1u];
    uint8_t n, at;
    uint32_t since;                              /* samples since the last tick */
    float bpm, period;                           /* the clock's tempo; samples a tick */
    uint32_t count;                              /* ticks since the cycle started */
    uint8_t ext;                                 /* following now */
    float next, step;                            /* out: the next tick (samples into the block), apart */
    uint8_t left;                                /* out: ticks left in this beat */
} clk = {{0}, 0, 0, 0xFFFFFFFFu, 60.0f, 1837.5f, 0, 0, 0.0f, 0.0f, 0};
int engine_following(void) { return clk.ext; }
static void midi_send(uint32_t st, uint32_t d1, uint32_t d2)
{
    plat_midi_out((st >> 4 == 0xFu ? 0xFu : st >> 4) | st << 8 | d1 << 16 | d2 << 24);
}
static void clk_tick_in(uint32_t pkt)
{
    const uint32_t ring = CLK_SPAN + 1u;
    if (!clk_in)
        return;
    if ((pkt & 0xF0u) == 0x10u) {                /* stamped: the tempo over the span */
        uint16_t s = (uint16_t)(pkt >> 16);
        if (clk.n && (uint16_t)(s - clk.stamp[(clk.at + ring - 1u) % ring]) > (uint16_t)(CLK_STAMP_HZ / 2.0f))
            clk.n = 0;
        clk.stamp[clk.at] = s;
        clk.at = (uint8_t)((clk.at + 1u) % ring);
        if (clk.n < ring)
            clk.n++;
        if (clk.n >= 3u) {
            uint16_t first = clk.stamp[(clk.at + ring - clk.n) % ring];
            float per = (float)(uint16_t)(s - first) / (float)(clk.n - 1u) * (SR / CLK_STAMP_HZ);
            if (per > 1.0f)
                clk.period = per;
        }
    } else if (clk.since > 0u && clk.since < (uint32_t)SR) {   /* not: the smoothed spacing, to a block */
        clk.period += ((float)clk.since - clk.period) * 0.125f;
    }
    clk.bpm = fm_clampf(SR * 60.0f / (24.0f * clk.period), 3.0f, 300.0f);
    clk.since = 0;
    clk.ext = 1;
    clk.count++;
}
static void clk_block(uint32_t n)                /* the time since the last tick; gone half a second: our own */
{
    clk.since = clk.since + n < clk.since ? 0xFFFFFFFFu : clk.since + n;
    if (clk.ext && clk.since > (uint32_t)SR / 2u) {
        clk.ext = 0;
        clk.n = 0;
    }
}
static void clk_emit(float upto)                 /* out: the ticks before upto */
{
    while (clk.left && clk.next < upto) {
        midi_send(0xF8u, 0, 0);
        clk.next += clk.step;
        clk.left--;
    }
}

/* ---------------------------------------------------------- the scale --- */
static g_scale_t sc;
static int sc_t = -1, sc_m = -1;
static void scale_check(void)
{
    int t = eng_w.p[P_TUNING] % G_TUNINGS, m = g_modes(t) ? eng_w.p[P_MODE] % g_modes(t) : 0;
    if (t != sc_t || m != sc_m) {
        g_scale(t, m, &sc);
        sc_t = t;
        sc_m = m;
    }
}
static float pitch(int i) { return g_pitch(&sc, i); }
static int fold(int i, float lo) { return g_fold(&sc, i, lo); }

/* ------------------------------------------------------------ the motion --- */
/* TIDES (Penumbra's): an even sine and a wandering one (a new height every half tide, eased into),
 * blended by SHAPE, the same every time (a fixed seed); its past kept, so a part can follow it behind */
static float tide_ph, tide_wph, tide_from, tide_to, tide_now, sway_ph;
static uint32_t tide_rng = 0x7A1DEu;
#define TH_PER 64
#define TH_N 128
static float tide_hist[TH_N];
static uint32_t tide_steps;
static int tide_s;
static float tide_rand(void)
{
    tide_rng = tide_rng * 1664525u + 1013904223u;
    return (float)(tide_rng >> 8) * (2.0f / 16777216.0f) - 1.0f;
}
static float sway_period(int rate) { return 60.0f * fm_exp2f((float)(rate - 1) * (-6.9068906f / 126.0f)); }
static void tide_tick(uint32_t n)
{
    float period = 8.0f * fm_exp2f(PV(P_T_RATE) * (5.0f / 127.0f)), inc = (float)n / (SR * period);
    float w = x127(P_T_SHAPE), ease;
    if (eng_w.p[P_S_RATE]) {
        sway_ph += (float)n / (SR * sway_period(eng_w.p[P_S_RATE]));
        if (sway_ph >= 1.0f)
            sway_ph -= 1.0f;
    }
    tide_ph += inc;
    if (tide_ph >= 1.0f)
        tide_ph -= 1.0f;
    tide_wph += 2.0f * inc;
    if (tide_wph >= 1.0f) {
        tide_wph -= 1.0f;
        tide_from = tide_to;
        tide_to = tide_rand();
    }
    ease = 0.5f - 0.5f * fm_sin_turns(0.25f - 0.5f * tide_wph);
    tide_now = (1.0f - w) * fm_sin_turns(tide_ph) + w * (tide_from + (tide_to - tide_from) * ease);
    {
        int s = (int)(tide_ph * (float)TH_PER), k = (s - tide_s + TH_PER) % TH_PER;
        for (; k; k--)
            tide_hist[++tide_steps % TH_N] = tide_now;
        tide_s = s;
    }
    eng_tide = tide_now;
}
static float tide_ago(float lag)
{
    float x;
    int32_t i;
    if (lag <= 0.0f)
        return tide_now;
    x = (float)tide_steps + (tide_ph * (float)TH_PER - (float)tide_s) - lag * (float)TH_PER;
    if (x < 0.0f)
        return 0.0f;
    i = (int32_t)x;
    x -= (float)i;
    return tide_hist[(uint32_t)i % TH_N] * (1.0f - x) +
           ((uint32_t)i + 1u > tide_steps ? tide_now : tide_hist[((uint32_t)i + 1u) % TH_N]) * x;
}

/* the throws (FX held), and what the tide does to the sound now */
static int16_t thr[TH_N];
static float bright_now = 1.0f, swell_now = 1.0f, ombak_now = 4.5f;
static float ombak_hz(void) { return 0.25f * fm_exp2f(PV(P_OMBAK) * (5.3219281f / 127.0f)); }   /* 0.25 x 40^(p/127) */

/* the effects: the FX page, the throws over it; set when they move */
static int16_t fx_cache[8], dip_cache = -999;
static void fx_put(int k, int v, int ext, int param)
{
    if (fx_cache[k] == v)
        return;
    fx_cache[k] = (int16_t)v;
    if (ext)
        fxbus_ext_set(&fx, param, v);
    else
        fxbus_set(&fx, param, v);
}
static int c127(float x) { return x < 0.0f ? 0 : x > 127.0f ? 127 : (int)(x + 0.5f); }
static float send_k, part_g[NPARTS];
static void motion(uint32_t n)
{
    const float tv = tide_now, freeze = (float)thr[TH_FREEZE] * (1.0f / 127.0f);
    const uint8_t *p = eng_w.p;
    int k, swaying = 0;
    tide_tick(n);
    bright_now = 1.0f + tv * x127(P_T_SWELL) * 0.6f;   /* (struck bronze swells louder and brighter together) */
    swell_now = 1.0f + tv * x127(P_T_SWELL) * 0.35f;
    ombak_now = ombak_hz() * fm_exp2f((float)thr[TH_OMBAK] * (2.0f / 127.0f)) * (1.0f + tv * x127(P_T_OMBAK) * 0.5f);
    send_k = x127(P_SPACE) * 0.9f * (1.0f + tv * x127(P_T_SPACE) * 0.5f) * (1.0f - 0.9f * freeze);
    {   /* PAN: each part where it is put, its sway on the tide (or a rate of its own), each swaying part
         * following the one before by OFFSET */
        const float off = x127(P_S_OFF) * 0.5f;
        for (k = 0; k < NPARTS; k++) {
            float x = ((float)p[P_PAN + k] - 64.0f) * (1.0f / 63.0f), sw = x127(P_SWAY + k);
            if (p[P_SWAY + k]) {
                float lag = (float)swaying++ * off;
                x += sw * (p[P_S_RATE] ? fm_sin_turns(sway_ph - lag) : tide_ago(lag));
            }
            eng_pan_now[k] = fm_clampf(x, -1.0f, 1.0f);
        }
    }
    {   /* HUSH: the core and the elaboration fall away (the gongs and your gendèr stay) */
        float h = 1.0f - (float)thr[TH_HUSH] * (1.0f / 127.0f);
        part_g[PART_GENDER] = part_g[PART_GONGS] = 1.0f;
        part_g[PART_CORE] = part_g[PART_KOT] = h;
    }
    {   /* the reverb: the FX page, FREEZE holding it (its decay all the way, the sends shut), SHIMMER thrown */
        float dec = PV(P_FX + 0);
        fx_put(0, c127(dec + (127.0f - dec) * freeze), 0, FX_RV_DECAY);
        fx_put(1, p[P_FX + 1], 1, FXX_RV_SIZE);
        fx_put(2, c127(PV(P_FX + 2) + (float)thr[TH_SHIMMER]), 1, FXX_RV_SHIMMER);
        fx_put(3, p[P_FX + 3], 0, FX_RV_TONE);
        fx_put(4, p[P_FX + 4], 1, FXX_RV_MOD);
        fx_put(5, p[P_FX + 5] % 3u, 1, FXX_RV_INTERVAL);
        fx_put(6, p[P_FX + 6], 0, FX_RV_HPF);
        fx_put(7, p[P_FX + 7], 0, FX_RV_LEVEL);
    }
    {
        int d = thr[TH_DIP], x = d > -3 && d < 3 ? 64 : d < 0 ? 59 + d * 59 / 127 : 69 + d * 58 / 127;
        if (x != dip_cache) {
            dip_cache = (int16_t)x;
            master_dip(&mst, x);
        }
    }
}

/* ------------------------------------------------------------ the voices --- */
static uint32_t rng = 0x2545F491u;
static float rnd01(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return (float)(rng >> 8) * (1.0f / 16777216.0f);
}
/* a stroke, with SHAPE's timbre for its family, the tide's brightness, the ombak now */
static bars_handle_t play(int inst, int part, float hz, float vel, float pan, uint32_t delay, int gong)
{
    bars_strike_t s;
    s.inst = (uint8_t)inst;
    s.part = (uint8_t)part;
    s.hz = hz;
    s.vel = vel;
    s.pan = pan;
    s.delay = delay;
    s.beat = ombak_now;
    s.bright = bright_now;
    s.ring = s.bloom = s.body = s.mallet = s.hum = s.swell = s.gong_ring = 1.0f;
    if (inst == IN_GENDER) {
        s.bright *= e2(P_TG_BRIGHT);
        s.ring = e4(P_TG_RING);
        s.bloom = x128(P_TG_BLOOM) * 2.0f;
    } else if (inst == IN_REYONG) {
        s.bright *= e2(P_TR_BRIGHT);
        s.ring = e4(P_TR_RING);
        s.mallet = x128(P_TR_MALLET) * 2.0f;
    } else if (inst < IN_GONG) {                 /* the bars: gangsa, jublag, jegogan */
        s.bright *= e2(P_TB_BRIGHT);
        s.ring = e4(P_TB_RING);
        s.mallet = x128(P_TB_MALLET) * 2.0f;
        s.bloom = x128(P_TB_BLOOM) * 2.0f;
    } else {                                     /* the gongs: their family's, and each one's own */
        s.bright = 1.0f;
        s.beat *= e2(P_TO_OMBAK);
        s.body = x128(P_TO_BODY) * 2.0f;
        s.hum = x128(P_TO_HUM) * 2.0f * x128(P_GONG(gong, GF_HUM)) * 2.0f;
        s.swell = e2(P_TO_SWELL);
        s.gong_ring = e4(P_GONG(gong, GF_RING));
    }
    return bars_strike(&bars, &s);
}
/* the damped parts' last strokes */
enum { H_CORE, H_JEG, H_POLOS, H_SANGSIH, H_REY, H_ORN, H_GEM1, H_GEM2, H_N };
static bars_handle_t held[H_N];
static void strike_h(int slot, int inst, int part, float hz, float vel, float pan, uint32_t delay, uint32_t damp_after)
{
    if (damp_after != 0xFFFFFFFFu)
        bars_damp(&bars, held[slot], delay + damp_after, 0.06f);
    held[slot] = play(inst, part, hz, vel, pan, delay, 0);
}

/* the gongs: each one's tone (its hold's TONE moves it), where the cycle strikes them */
static int gong_idx(int g)
{
    int t = (int)eng_w.p[P_GONG(g, GF_TONE)] - 2;
    switch (g) {
    case G_GONG: return fold(t, 44.0f);
    case G_LANANG: return fold((int)eng_w.p[P_GONG(G_GONG, GF_TONE)] - 2, 44.0f) + 1 + t;
    case G_KEMPUR: return fold((sc.n > 3 ? 3 : sc.n - 1) + t, 88.0f);
    case G_KEMONG: return fold(2 + t, 280.0f);
    default: return fold(sc.n - 1 + t, 620.0f);
    }
}
static const uint8_t GONG_INST[G_N] = {IN_GONG, IN_GONG, IN_KEMPUR, IN_KEMONG, IN_KAJAR};
static const float GONG_PAN[G_N] = {-0.1f, 0.15f, 0.0f, 0.0f, 0.0f};
static void gong(int g, uint32_t delay, float vel)
{
    play(GONG_INST[g], PART_GONGS, pitch(gong_idx(g)), vel * lvl(P_L_GONG) * lvl(P_GONG(g, GF_LEVEL)), GONG_PAN[g], delay, g);
    if (g == G_GONG)
        eng_gong_at = eng_frames + delay;
}
/* what the cycle strikes at a place: each gong's PLACES (off, as made, every other cycle; the lanang, off as
 * made, answers the gong at the half; the kajar every beat or every other) */
static int keep(int g, uint32_t cyc)
{
    int pl = eng_w.p[P_GONG(g, GF_PLACES)];
    return pl == 1 || (pl == 2 && !(cyc & 1u));
}
static void cycle_strikes(int pos, int len, uint32_t cyc, uint32_t delay)
{
    const int form = eng_w.p[P_FORM] % G_FORMS;
    int g, kj = eng_w.p[P_GONG(G_KAJAR, GF_PLACES)], s;
    for (g = 0; g < G_KAJAR; g++)
        if (g_strike(form, g, pos, len) && keep(g, cyc))
            gong(g, delay, 0.9f);
    s = g_strike(form, G_KAJAR, pos, len);       /* (every other: the strokes, not the light ones) */
    if (s && (kj == 1 || (kj == 2 && s == 2)))
        gong(G_KAJAR, delay, s == 2 ? 0.85f : 0.6f);
}

/* ---------------------------------------------------------- the cycle --- */
static struct {
    float next_off;                              /* samples into this block of the next grid step */
    float dur;                                   /* this beat's, samples */
    float dyn, fade;                             /* the breath's swell, the ending's thinning */
    int j;                                       /* the grid step in the beat */
    uint32_t beat_no;
    float end_mul;
    int end_beats, end_gongs;
    uint8_t angsel_pend, angsel;                 /* an angsel asked for; this beat is one */
} cy;
static int16_t core_c = NONE, core_n = NONE;
static int8_t oct;
static uint8_t loop_i;
static int next_core(void)
{
    if (eng_loop_state == LS_PLAY && eng_w.loop_len)
        return eng_w.loop[loop_i % eng_w.loop_len] == -128 ? core_c : eng_w.loop[loop_i % eng_w.loop_len];
    return core_n != NONE ? core_n : core_c;
}

static void begin_beat(uint32_t off)
{
    int pos, len, final = 0;
    float p, d;
    if (eng_pos >= cycle_len())                  /* a shorter cycle or form chosen: on to its gong */
        eng_pos = 0;
    pos = eng_pos;
    if (pos == 0 && (eng_pend_ens >= 0 || eng_pend_mode >= 0)) {   /* a new gamelan or mode arrives with the gong */
        if (eng_pend_ens >= 0)
            engine_ensemble(eng_pend_ens);
        else
            eng_w.p[P_MODE] = (uint8_t)eng_pend_mode;
        eng_pend_ens = eng_pend_mode = -1;
        scale_check();
    }
    len = cycle_len();
    if (pos == 0 && cy.beat_no)
        eng_cycle_no++;
    /* the core note for this beat */
    if (eng_loop_state == LS_PLAY && eng_w.loop_len) {
        int8_t x = eng_w.loop[loop_i % eng_w.loop_len];
        if (x != -128)
            core_c = x;
        loop_i++;
    } else if (core_n != NONE) {
        core_c = core_n;
        core_n = NONE;
    }
    if (eng_loop_state == LS_ARM && pos % (len < LOOP_MAX ? len : LOOP_MAX) == 0) {
        eng_loop_state = LS_REC;
        eng_w.loop_len = 0;
    }
    if (eng_loop_state == LS_REC) {
        eng_w.loop[eng_w.loop_len++] = (int8_t)(core_c == NONE ? -128 : core_c);
        if (eng_w.loop_len >= (len < LOOP_MAX ? len : LOOP_MAX)) {
            eng_loop_state = LS_PLAY;
            eng_w.loop_kept = 1;
            loop_i = 0;
        }
    }
    /* the tempo glides toward its target (the cues' GLIDE), breathes across the cycle, slows for the ending */
    eng_bpm_now += (target_bpm() - eng_bpm_now) * (0.03f + 0.97f * x127(P_C_GLIDE));
    if (eng_ending) {
        cy.end_beats++;
        cy.end_mul = fm_minf(cy.end_mul * (1.0f + x128(P_E_SLOW) * 0.18f), 2.5f);   /* (at most 2.5 times as slow) */
        if (pos == 0 && cy.end_beats > 4)        /* closing on the next gong at least four beats away (or the one after) */
            cy.end_gongs++;
        final = cy.end_gongs > (int)eng_w.p[P_E_AFTER];
    }
    p = (float)(pos + 1) / (float)len;
    d = 60.0f / fm_maxf(eng_bpm_now, 2.0f) * (1.0f + x127(P_BREATH) * 0.9f * p * p * p * p) * cy.end_mul;
    if (clk.ext) {                               /* a clock leads: its tempo, the beat nudged into phase with it */
        float err = (float)clk.count - 1.0f + fm_minf((float)clk.since / clk.period, 1.0f) - 24.0f * (float)cy.beat_no;
        eng_bpm_now = clk.bpm;
        d = clk.period * fm_clampf(24.0f - 0.5f * err, 12.0f, 36.0f) / SR;
    }
    cy.dur = d * SR;
    if (clk_out && !clk.ext) {                   /* our clock out: this beat's ticks */
        clk.next = (float)off;
        clk.step = cy.dur / 24.0f;
        clk.left = final ? 1u : 24u;
    }
    cy.dyn = (1.0f + x127(P_BREATH) * (0.5f * fm_sin_turns(0.5f * p) - 0.25f)) * swell_now;
    cy.fade = eng_ending ? fm_maxf(0.2f, 1.0f - (float)cy.end_beats * x128(P_E_THIN) * 0.16f) : 1.0f;
    cy.angsel = cy.angsel_pend && !final;
    cy.angsel_pend = 0;
    /* the cycle's instruments at their places (the ending's last: the gong) */
    if (final)
        gong(G_GONG, off, 0.9f);
    else if (eng_w.p[P_AUTO])
        cycle_strikes(pos, len, eng_cycle_no, off);
    /* the core */
    if (core_c != NONE && (eng_w.p[P_PARTS] & PB_CORE)) {
        float v = cy.dyn * lvl(P_L_CORE);
        if (eng_w.p[P_CORE_G])
            strike_h(H_CORE, IN_GENDER, PART_CORE, pitch(fold(core_c, 150.0f)), 0.8f * v, -0.2f, off, 2205u);
        else {
            strike_h(H_CORE, IN_JUBLAG, PART_CORE, pitch(fold(core_c, 200.0f)), 0.85f * v, -0.15f, off, 2205u);
            if (!(pos & 1) || final)             /* the jegogan takes every other core note */
                strike_h(H_JEG, IN_JEGOGAN, PART_CORE, pitch(fold(core_c, 100.0f)), 0.8f * v, 0.1f, off, 2205u);
        }
    }
    if (core_c != NONE) {
        uint32_t w = eng_bead_w % BEADS;
        eng_bead_at[w] = eng_frames + off;
        eng_bead_pos[w] = (uint8_t)pos;
        eng_bead_w = (uint8_t)(w + 1);
    }
    eng_core = core_c;
    eng_beat_pos = (uint8_t)pos;
    eng_beat_at = eng_frames + off;
    eng_beat_len = final ? 0u : (uint32_t)cy.dur;
    if (final) {
        if (core_c != NONE)
            play(IN_GENDER, PART_KOT, pitch(fold(core_c, 300.0f)), 0.4f, 0.3f, off, 0);
        eng_running = 0;
        eng_ending = 0;
        cy.end_mul = 1.0f;
        eng_pos = 0;
        clk_emit((float)off + 1.0f);
        if (clk_out && !clk.ext)
            midi_send(0xFCu, 0, 0);
    }
}

/* the interlocking parts: leave the core note, turn toward the next before it arrives. TURN is where in the
 * beat they turn (a quarter of the way to three quarters; as made, halfway) */
static float turn_at(int i) { return 0.25f + 0.5f * x127(i); }
static int kotekan_note(int kc, int kn, int j, int sub)
{
    static const int8_t F_UP[3] = {0, 1, -1}, F_DN[3] = {0, -1, 1};
    int up;
    if (j == 0)
        return kc;
    up = kn > kc ? 1 : kn < kc ? 0 : !(cy.beat_no & 1u);
    if ((float)j < (float)sub * turn_at(P_K_TURN))
        return kc + (up ? F_UP : F_DN)[j % 3];   /* after the core note, circling it */
    return kn + (up ? F_DN : F_UP)[(sub - j) % 3];   /* before the next, circling it (counted back from its arrival) */
}
static int kot_sub(void)
{
    static const uint8_t SUB[5] = {0, 2, 4, 8, 16};
    int s = eng_w.p[P_K_SUB] % 5u;
    float dn = x127(P_DENSITY);
    return s ? SUB[s] : dn < 0.08f ? 0 : dn < 0.55f ? 4 : 8;
}
static int sparse(int j, int i) { return j != 0 && rnd01() < x127(i) * 0.85f; }   /* (never the landing) */

/* THE DRUMS. The kendang: two drums, wadon (low) and lanang (high), interlocking as the kotekan's two parts do,
 * the wadon's open DAG on the strong places, the lanang's TUT between and its PAK (a slap) answering; two
 * figures, one a beat, in turn. The ceng-ceng: small cymbals, the beat in four, the first stroke open. Both
 * thin as the beat slows (a beat over 2.4 s: two strokes; over 4 s: one), and BUSY thins or fills them. They
 * sit with the elaboration (HUSH lets them fall away too). */
static void drum(int inst, float hz, float vel, float slap, float pan, uint32_t delay)
{
    bars_strike_t s;
    s.inst = (uint8_t)inst;
    s.part = PART_KOT;
    s.hz = hz * (0.985f + 0.03f * rnd01());
    s.vel = vel * (0.88f + 0.2f * rnd01());
    s.pan = pan;
    s.delay = delay;
    s.beat = 0.0f;
    s.bright = bright_now;
    s.ring = s.bloom = s.body = s.hum = s.swell = s.gong_ring = 1.0f;
    s.mallet = slap * x128(P_D_SLAP) * 2.0f;
    bars_strike(&bars, &s);
}
static int drum_per(void)                        /* the drums' strokes a beat */
{
    float secs = cy.dur / SR, busy = x127(P_D_BUSY);
    return secs > 4.0f ? 1 : secs > 2.4f ? 2 : busy < 0.3f ? 4 : 8;
}
static void drums(uint32_t off, int j, float lv)
{
    /* 0 none, 1 dug (soft), 3 dag (open, accented); the lanang's 1 tut, 2 pak (slap). Eight a beat */
    static const uint8_t WAD[2][8] = {{3, 0, 0, 1, 0, 0, 3, 0}, {3, 0, 0, 1, 0, 1, 0, 0}};
    static const uint8_t LAN[2][8] = {{0, 1, 0, 0, 2, 0, 0, 1}, {0, 1, 0, 0, 2, 0, 1, 0}};
    const int parts = eng_w.p[P_PARTS], per = drum_per(), step = GRID / per, fig = (int)(cy.beat_no & 1u);
    if (j % step)
        return;
    if (parts & PB_KEN) {
        int q = (j / step) * 8 / per, w = WAD[fig][q], l = LAN[fig][q];
        float v = lv * lvl(P_L_KEN);
        if (per <= 2 && j == 0)
            w = 3;                               /* (slow: the wadon keeps the beat) */
        if (w)
            drum(IN_WADON, 88.0f, (w == 3 ? 0.95f : 0.45f) * v, w == 3 ? 0.7f : 0.4f, -0.12f, off);
        if (l && per >= 4)
            drum(IN_LANANG, l == 2 ? 196.0f : 148.0f, (l == 2 ? 0.55f : 0.6f) * v, l == 2 ? 3.0f : 0.6f, 0.12f,
                 off + (uint32_t)(rnd01() * 180.0f));
    }
    if ((parts & PB_CENG) && (per >= 4 ? !(j % (GRID / 4)) : 1))
        drum(IN_CENG, 1100.0f, (j == 0 ? 0.9f : 0.5f) * lv * lvl(P_L_CENG), j == 0 ? 1.0f : 0.55f, 0.3f, off);
}

/* ANGSEL (SEL on HOME, or MIDI): the beat after it breaks. The ensemble strikes together, accented, on the beat
 * and again late in it (syncopated), the drums and the ceng-ceng with it, and leaves the rest of the beat
 * silent; then the music goes on. */
static void angsel_strokes(uint32_t off, int j, int c, float lv)
{
    int parts = eng_w.p[P_PARTS];
    float v = (j == 0 ? 1.0f : 0.85f) * lv;
    if (j != 0 && j != 10)
        return;
    if (parts & PB_KOT) {
        int k = fold(c, eng_w.p[P_KOT_G] ? 250.0f : 300.0f);
        strike_h(H_POLOS, eng_w.p[P_KOT_G] ? IN_GENDER : IN_GANGSA, PART_KOT, pitch(k), 0.85f * v * lvl(P_L_KOT), -0.4f, off,
                 2205u);
        strike_h(H_SANGSIH, eng_w.p[P_KOT_G] ? IN_GENDER : IN_GANGSA, PART_KOT, pitch(k + sc.n), 0.8f * v * lvl(P_L_KOT),
                 0.4f, off, 2205u);
    }
    if (parts & PB_REY) {
        int r = fold(c, 480.0f);
        strike_h(H_REY, IN_REYONG, PART_KOT, pitch(r), 0.8f * v * lvl(P_L_REY), 0.35f, off, 4410u);
        play(IN_REYONG, PART_KOT, pitch(r + 2), 0.6f * v * lvl(P_L_REY), 0.45f, off, 0);
    }
    if (parts & PB_KEN) {
        drum(IN_WADON, 88.0f, 1.0f * v * lvl(P_L_KEN), 0.8f, -0.12f, off);
        drum(IN_LANANG, 196.0f, 0.7f * v * lvl(P_L_KEN), 3.0f, 0.12f, off);
    }
    if (parts & PB_CENG)
        drum(IN_CENG, 1100.0f, 1.0f * v * lvl(P_L_CENG), 1.3f, 0.3f, off);
}

static void elaborate_parts(uint32_t off, int j, int c, int nx);
static void elaborate(uint32_t off, int j, int c, int nx)
{
    if (cy.angsel) {
        angsel_strokes(off, j, c, cy.dyn * cy.fade);
        return;
    }
    drums(off, j, cy.dyn * cy.fade);
    elaborate_parts(off, j, c, nx);
}
static void elaborate_parts(uint32_t off, int j, int c, int nx)
{
    const float lv = cy.dyn * cy.fade, dens = x127(P_DENSITY);
    const int ks = kot_sub(), parts = eng_w.p[P_PARTS], step = ks ? GRID / ks : 0;
    float sub_s = cy.dur / (float)GRID;
    int rs;
    if ((parts & PB_KOT) && ks && j % step == 0) {
        int jk = j / step, kc, kn, pn = NONE, sn = NONE, turned, t, k;
        float lo = (eng_w.p[P_KOT_G] ? 250.0f : 300.0f) * fm_exp2f((float)((int)eng_w.p[P_K_REG] - 1));
        float v = (0.5f + dens * 0.3f) * lv * (0.85f + 0.2f * rnd01()) * lvl(P_L_KOT);
        const int inst = eng_w.p[P_KOT_G] ? IN_GENDER : IN_GANGSA;
        const uint32_t damp = (uint32_t)(sub_s * (float)step * 0.9f);
        kc = fold(c, lo);
        kn = g_near(&sc, nx, kc);
        if (pitch(kn) < lo * 0.94f)
            kn += sc.n;
        if (pitch(kn) > lo * 2.6f)
            kn -= sc.n;
        turned = jk && (float)jk >= (float)ks * turn_at(P_K_TURN);
        t = turned ? kn : kc;
        switch (eng_w.p[P_K_STYLE] % G_STYLES) {
        case KS_NOROT:                           /* the note and the one above it, in turn; then the next and its */
            if (jk & 1)
                sn = t + 1;
            else
                pn = t;
            break;
        case KS_NYOGCAG: {                       /* one line, the parts in turn, walking to the next note */
            int d = kn - kc, s = d > 0 ? 1 : d < 0 ? -1 : (cy.beat_no & 1u ? 1 : -1), ad = d * s, lead = ks - jk, n;
            n = jk == 0 ? kc : lead <= ad ? kn - s * lead : ((lead - ad) & 1) ? kc - s : kc;
            if (jk & 1)
                sn = n;
            else
                pn = n;
            break;
        }
        case KS_EMPAT:                           /* four tones: the polos the lower three, the sangsih the upper two,
                                                    together on the first */
            k = jk & 3;
            pn = k == 0 || k == 3 ? t : k == 1 ? t - 1 : NONE;
            sn = k == 0 ? t + 2 : k == 2 ? t + 1 : NONE;
            break;
        default:                                 /* circling (as made) */
            if (jk & 1)
                sn = kotekan_note(kc, kn, jk, ks);
            else
                pn = kotekan_note(kc, kn, jk, ks);
            break;
        }
        if (pn != NONE && !sparse(j, P_K_SPARSE))
            strike_h(H_POLOS, inst, PART_KOT, pitch(pn), v, -0.4f, off, damp);
        if (sn != NONE && !sparse(j, P_K_SPARSE))
            strike_h(H_SANGSIH, inst, PART_KOT, pitch(sn), v * (pn != NONE ? 0.8f : 0.92f), 0.4f,
                     off + (uint32_t)(rnd01() * 260.0f), damp);
    }
    rs = ks >= 4 && dens > 0.2f ? ks / (eng_w.p[P_R_RATE] ? 4 : 2) : 0;   /* the reyong: half the kotekan, or a quarter */
    if ((parts & PB_REY) && rs >= 1 && j % (GRID / rs) == 0) {
        int jj = j / (GRID / rs), half = (int)((float)rs * turn_at(P_R_TURN) + 0.5f), rc, rn, note;
        if (half < 1)
            half = 1;
        rc = fold(c, 480.0f * fm_exp2f((float)((int)eng_w.p[P_R_REG] - 1)));
        rn = g_near(&sc, nx, rc);
        note = rs == 1 ? rc : jj < half ? rc + (jj & 1) : rn + ((jj - half) & 1);
        if (!sparse(j, P_R_SPARSE))
            strike_h(H_REY, IN_REYONG, PART_KOT, pitch(note), 0.6f * lv * (0.85f + 0.2f * rnd01()) * lvl(P_L_REY), 0.35f,
                     off, (uint32_t)(sub_s * (float)(GRID / rs) * 1.8f));
    }
    if ((parts & PB_ORN) && dens > 0.12f) {       /* the next note as a pickup (PICKUP: how early), the core in octaves */
        float lo = 150.0f * fm_exp2f((float)((int)eng_w.p[P_O_REG] - 1));
        int pick = GRID - (int)((float)GRID * (1.0f / 16.0f + x127(P_O_PICK) * 7.0f / 16.0f) + 0.5f);
        if (pick >= GRID)
            pick = GRID - 1;
        /* (each damps the one before as it strikes, as a gendèr player's hands do: they do not pile up) */
        if (j == pick && !sparse(j, P_O_SPARSE))
            strike_h(H_ORN, IN_GENDER, PART_KOT, pitch(fold(nx, lo * 2.0f)), 0.32f * lv * lvl(P_L_ORN), 0.3f, off, 2205u);
        if (j == 0 && eng_w.p[P_O_OCT] > 1) {
            int g = fold(c, lo);
            float v = 0.22f * lv * x128(P_O_OCT) * 2.0f * lvl(P_L_ORN);
            strike_h(H_GEM1, IN_GENDER, PART_KOT, pitch(g), v, 0.25f, off + 176u, 2205u);
            strike_h(H_GEM2, IN_GENDER, PART_KOT, pitch(g + sc.n), v, 0.35f, off + 176u, 2205u);
        }
    }
}

static void schedule(uint32_t n)
{
    if (!eng_running)
        return;
    while (eng_running && cy.next_off < (float)n) {
        uint32_t off = (uint32_t)cy.next_off;
        if (cy.j == 0) {
            clk_emit((float)off);
            begin_beat(off);
        }
        if (!eng_running)
            break;
        if (core_c != NONE)
            elaborate(off, cy.j, core_c, next_core());
        cy.j++;
        cy.next_off += cy.dur / (float)GRID;
        if (cy.j >= GRID) {
            cy.j = 0;
            eng_pos = (uint8_t)((eng_pos + 1) % cycle_len());
            cy.beat_no++;
        }
    }
    cy.next_off -= (float)n;
    clk_emit((float)n);
    clk.next -= (float)n;
    eng_next = (int16_t)(eng_running ? next_core() : NONE);
}

/* ------------------------------------------------------------ the hands --- */
static bars_handle_t white_h[32];               /* the FM-1's white keys 0..15, MIDI's notes 16..31 */
static uint32_t white_down;
static int8_t white_last = -1;
static uint8_t sustain;                          /* MIDI's pedal: no damping hand */
static uint8_t bend[16][4];
static void bend_apply(int k)
{
    const uint8_t *b = bend[k];
    bars_bend(&bars, white_h[k], ombak_now * (float)b[1] * (1.0f / 42.0f), ((float)b[2] - 64.0f) * (1.0f / 63.0f),
              (float)b[3] * (1.0f / 80.0f), (float)b[0] * (1.5f / 127.0f));
}
/* a key (slot k) plays tone i: the gendèr at once, and the next core note */
static void tone(int k, int i, float vel)
{
    /* the damping hand: the note before is stopped as this one sounds, unless its key is still held (two hands) */
    if (white_last >= 0 && !sustain && !(white_down >> white_last & 1u))
        bars_damp(&bars, white_h[white_last], 4410u, 0.03f * fm_exp2f(x128(P_TG_DAMP) * 8.0f));
    white_h[k] = play(IN_GENDER, PART_GENDER, pitch(i), 0.9f * vel * lvl(P_L_GENDER), 0.0f, 0, 0);
    if (k < 16) {
        bend[k][0] = 0;
        bend[k][1] = 42;
        bend[k][2] = 64;
        bend[k][3] = 80;
    }
    white_down |= 1u << k;
    white_last = (int8_t)k;
    if (eng_loop_state != LS_PLAY) {
        if (eng_running)
            core_n = (int16_t)i;
        else
            core_c = eng_core = (int16_t)i;
    }
}
/* MIDI NOTES OUT: the FM-1's keys as MIDI in takes them (a tone the white key counted from middle C; the gongs
 * C#, D#, F#, G#, A# above it) */
static uint8_t key_note[16];
static uint8_t tone_note(int i)
{
    static const uint8_t SEMI[7] = {0, 2, 4, 5, 7, 9, 11};
    int o = i >= 0 ? i / 7 : -((-i + 6) / 7), n = 60 + 12 * o + SEMI[i - 7 * o];
    return (uint8_t)(n < 0 ? 0 : n > 127 ? 127 : n);
}
static void note_out(int on, uint32_t note, uint32_t vel)
{
    if (note_send && midi_ch)
        midi_send((on ? 0x90u : 0x80u) | (midi_ch - 1u), note, on ? vel : 0u);
}
static void white(int k, int down)
{
    if (k < 0 || k > 15)
        return;
    if (!down) {
        white_down &= ~(1u << k);
        note_out(0, key_note[k], 0);
    } else {
        int i = k + oct * sc.n;
        tone(k, i, 1.0f);
        key_note[k] = tone_note(i);
        note_out(1, key_note[k], 100);
    }
}

/* MIDI in, on SYSTEM's channel: a white key (any octave: middle C the first tone, each white key the next tone)
 * plays as the FM-1's do, its velocity the stroke's; a black key strikes a gong (C# the gong, D# the lanang, F#
 * the kempur, G# the kemong, A# the kajar); the pedal holds every note (no damping hand); CC 20..23 HOME's
 * OMBAK, DENSITY, SPACE, BREATH (and CC 1 DENSITY), CC 24 the tempo; a program change the gamelan at the next
 * gong; Start and Stop the cycle (any channel). */
static void run(int on);
static void midi_in(uint32_t pkt)
{
    static const int8_t WHITE_OF[12] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
    static const int8_t GONG_OF[12] = {-1, G_GONG, -1, G_LANANG, -1, -1, G_KEMPUR, -1, G_KEMONG, -1, G_KAJAR, -1};
    uint32_t s = pkt >> 8 & 0xFFu, d1 = pkt >> 16 & 0xFFu, d2 = pkt >> 24 & 0xFFu;
    if (s == 0xF8) {
        clk_tick_in(pkt);
        return;
    }
    if (s == 0xFA || s == 0xFB) {                /* (with FOLLOW CLOCK IN, its clock is expected: none echoed) */
        if (clk_in) {
            clk.ext = 1;
            clk.since = 0;
        }
        run(1);
        return;
    }
    if (s == 0xFC) {
        run(0);
        return;
    }
    if (!midi_ch || s < 0x80 || s >= 0xF0 || (s & 15u) + 1u != midi_ch)
        return;
    d1 &= 0x7Fu;
    d2 &= 0x7Fu;
    if ((s & 0xF0u) == 0x90u && !d2)
        s = 0x80u;                               /* a note on at 0 is a note off */
    switch (s & 0xF0u) {
    case 0x90: {
        int pc = (int)(d1 % 12u), o = (int)(d1 / 12u) - 5;
        if (GONG_OF[pc] >= 0)
            gong(GONG_OF[pc], 0, 0.4f + 0.6f * (float)d2 / 127.0f);
        else
            tone(16 + (int)(d1 & 15u), o * 7 + WHITE_OF[pc], 0.3f + 0.7f * (float)d2 / 127.0f);
        break;
    }
    case 0x80:
        white_down &= ~(1u << (16u + (d1 & 15u)));
        break;
    case 0xB0:
        switch (d1) {
        case 1: case 21: eng_w.p[P_DENSITY] = (uint8_t)d2; break;
        case 20: eng_w.p[P_OMBAK] = (uint8_t)d2; break;
        case 22: eng_w.p[P_SPACE] = (uint8_t)d2; break;
        case 23: eng_w.p[P_BREATH] = (uint8_t)d2; break;
        case 24: eng_w.p[P_TEMPO] = (uint8_t)d2; break;
        case 25: if (d2 >= 64u && eng_running && !eng_ending) cy.angsel_pend = 1; break;   /* an angsel */
        case 64: sustain = d2 >= 64; break;
        case 120: case 123:                      /* all sound / notes off: the hands lift and damp */
            sustain = 0;
            white_down = 0;
            for (s = 0; s < 32u; s++)
                bars_damp(&bars, white_h[s], 0u, 0.03f);
            break;
        default: break;
        }
        break;
    case 0xC0:
        eng_pend_ens = (int8_t)(d1 % G_ENSEMBLES == eng_w.p[P_ENS] && eng_running ? -1 : (int)(d1 % G_ENSEMBLES));
        break;
    default: break;
    }
}

static void run(int on)
{
    if (on && !eng_running) {
        eng_pos = 0;
        cy.beat_no = 0;
        cy.j = 0;
        cy.next_off = 0.0f;
        cy.end_mul = 1.0f;
        eng_cycle_no = 0;
        eng_ending = 0;
        eng_bpm_now = target_bpm();
        loop_i = 0;
        clk.count = 0;
        clk.left = 0;
        if (clk_out && !clk.ext)
            midi_send(0xFAu, 0, 0);
        eng_running = 1;
    } else if (!on) {
        if (eng_running && clk_out && !clk.ext)
            midi_send(0xFCu, 0, 0);
        clk.left = 0;
        eng_running = 0;
        eng_ending = 0;
        cy.end_mul = 1.0f;
    }
}
static void loop_op(int op)
{
    switch (op) {
    case LOOP_REC:                               /* the next cycle from its start (again: cancel) */
        if (eng_loop_state == LS_ARM || eng_loop_state == LS_REC)
            eng_loop_state = eng_w.loop_kept ? LS_HELD : LS_OFF;
        else
            eng_loop_state = LS_ARM;
        break;
    case LOOP_PLAY:                              /* held (you lead again), or back */
        if (eng_loop_state == LS_PLAY)
            eng_loop_state = LS_HELD;
        else if (eng_loop_state == LS_HELD || (eng_loop_state == LS_OFF && eng_w.loop_kept)) {
            eng_loop_state = LS_PLAY;
            loop_i = (uint8_t)(eng_w.loop_len ? eng_pos % eng_w.loop_len : 0);
        }
        break;
    default:
        eng_loop_state = LS_OFF;
        eng_w.loop_kept = 0;
        eng_w.loop_len = 0;
        break;
    }
}

/* the load test: the heaviest the voices get: a long gendèr note into the pool every block, until all of it
 * sounds; the cycle's own on top */
static uint32_t test_i;
static void test_tick(void)
{
    if (!testing)
        return;
    play(IN_GENDER, (int)(test_i % NPARTS), pitch((int)(test_i * 7u % 15u)), 0.05f, 0.0f, 0, 0);
    test_i++;
}

static void drain(void)
{
    uint32_t r = cq_r, pkt;
    int guard = 64;
    while (r != cq_w) {
        uint32_t c = cq[r % CQ], type = c >> 28, a = (c >> 20) & 0xFFu, b = (c >> 12) & 0xFFu, v = c & 0xFFFu;
        r++;
        switch (type) {
        case C_RUN: run((int)a); break;
        case C_WHITE: white((int)a, (int)b); break;
        case C_OCT: oct = (int8_t)((int)v - 8); break;
        case C_GONG:
            if (a < G_N) {
                static const uint8_t GONG_NOTE[G_N] = {61, 63, 66, 68, 70};
                gong((int)a, 0, a == G_KAJAR ? 0.9f : 1.0f);
                note_out(1, GONG_NOTE[a], 100);
                note_out(0, GONG_NOTE[a], 0);
            }
            break;
        case C_END: if (eng_running && !eng_ending) { eng_ending = 1; cy.end_beats = cy.end_gongs = 0; } break;
        case C_PMODE: eng_pend_mode = (int8_t)((int)a - 1); break;
        case C_PENS: eng_pend_ens = (int8_t)((int)a - 1); break;
        case C_LOOP: loop_op((int)a); break;
        case C_BEND:
            if (a < 16u && b < 4u) {
                bend[a][b] = (uint8_t)(v > 127u ? 127u : v);
                bend_apply((int)a);
            }
            break;
        case C_THROW:
            if (a < TH_N) {
                int x = (int)v - 128;
                thr[a] = (int16_t)(x < -127 ? -127 : x > 127 ? 127 : x);
            }
            break;
        case C_TEST:
            testing = (uint8_t)a;
            guard_enabled = !a;
            break;
        case C_ANGSEL: if (eng_running && !eng_ending) cy.angsel_pend = 1; break;
        case C_CC:                               /* MIDI CC OUT (the main loop's: sent from here, one sender) */
            if (cc_send && midi_ch)
                midi_send(a >= 128u ? 0xC0u | (midi_ch - 1u) : 0xB0u | (midi_ch - 1u), a >= 128u ? a - 128u : a,
                          a >= 128u ? 0u : (b > 127u ? 127u : b));
            break;
        default: break;
        }
    }
    cq_r = r;
    while (guard-- && plat_midi_in(&pkt))
        midi_in(pkt);
}

/* ------------------------------------------------------------------ output --- */
static volatile float out_scale = 4194303.0f, out_lim = 2.0f;
void engine_out_loud(int on)
{
    out_scale = on ? 8388607.0f : 4194303.0f;
    out_lim = on ? 1.0f : 2.0f;
}
static volatile int usb_full;
static float vol_after = -1.0f;
void engine_usb_full(int on) { usb_full = on; }
static volatile uint32_t eng_quiet_frames;
uint32_t engine_quiet_ms(void) { return eng_quiet_frames / 44u; }
void engine_volume(int32_t *out_lr, uint32_t n)
{
    float vol = (float)plat_master() / 4096.0f, v = vol_after < 0.0f ? vol : vol_after;
    uint32_t i;
    if (!usb_full) {
        vol_after = -1.0f;
        return;
    }
    for (i = 0; i < n; i++) {
        if (v != vol) {
            v += (vol - v) * 0.0022651f;
            if (fm_fabsf(vol - v) < 1e-5f)
                v = vol;
        }
        out_lr[2u * i] = (int32_t)((float)out_lr[2u * i] * v);
        out_lr[2u * i + 1u] = (int32_t)((float)out_lr[2u * i + 1u] * v);
    }
    vol_after = v;
}
int engine_voices(void) { return bars_active(&bars); }

/* ------------------------------------------------------------------- mix --- */
typedef struct { float cur, tgt; } glide_t;
#define GLIDE_A 0.0022651f                       /* ~10 ms */
static void glide_set(glide_t *g, float t)
{
    g->tgt = t;
    if (g->cur < 0.0f)
        g->cur = t;
}
__attribute__((noinline)) static void add_glide(float *dst, const float *src, glide_t *g, uint32_t n)
{
    uint32_t i;
    float c = g->cur, t = g->tgt;
    if (c == t) {
        if (c != 0.0f)
            for (i = 0; i < n; i++)
                dst[i] += src[i] * c;
        return;
    }
    for (i = 0; i < n; i++) {
        c += (t - c) * GLIDE_A;
        dst[i] += src[i] * c;
    }
    if (fm_fabsf(t - c) < 1e-5f)
        c = t;
    g->cur = c;
}
static glide_t gl_l[NPARTS] = {{-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}, gl_r[NPARTS] = {{-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}};
static glide_t gl_s[NPARTS] = {{-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}, gl_x = {-1, 0};

/* --------------------------------------------------------------- suling --- */
/* The suling, the bamboo flute: one long tone on the core note (REGISTER: from about 440 Hz, an octave down or
 * up), gliding to each new one (GLIDE), a vibrato that grows in after each arrival (VIBRATO), the breath's hiss
 * (BREATH). Balinese suling players breathe circularly, so the tone never has to break; it swells in as the cycle
 * starts and fades as it stops. Its own few sines a sample, on the first core, with your gendèr. */
static struct {
    float ph, hz, amp, since, vph;               /* phase (turns), Hz now, level now, seconds on this note, vibrato */
    float nlp, nbp;                              /* the breath's band-pass */
    int note;
} sul = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -32768};
static void suling_block(float *L, float *R, uint32_t n)
{
    const int on = (eng_w.p[P_PARTS] & PB_SUL) && eng_running && core_c != NONE;
    const float dt = 1.0f / SR, vib = x127(P_S_VIB) * 0.012f, breath = x127(P_S_BREATH) * 0.12f;
    const float gk = 1.0f - fm_expf(-dt / (0.02f + 0.5f * x127(P_S_GLIDE) * x127(P_S_GLIDE)));
    float to = 0.0f, target = 0.0f, ak, nf;
    uint32_t i;
    if (on) {
        int k = fold(core_c, 220.0f * fm_exp2f((float)eng_w.p[P_S_REG]));
        if (k != sul.note) {
            sul.note = k;
            sul.since = 0.0f;
        }
        to = pitch(k);
        target = 0.13f * lvl(P_L_SUL) * swell_now;
        if (sul.hz < 1.0f)
            sul.hz = to;
    }
    if (sul.amp < 1e-5f && target <= 0.0f)
        return;
    ak = 1.0f - fm_expf(-dt / (target > sul.amp ? 0.35f : 1.6f));
    nf = 2.0f * fm_sin_turns(fm_minf(sul.hz * 2.0f, 6000.0f) / SR * 0.5f);
    for (i = 0; i < n; i++) {
        float depth = vib * fm_minf(sul.since * 2.0f, 1.0f), f, s, hp;
        if (on)
            sul.hz += (to - sul.hz) * gk;
        f = sul.hz * (1.0f + depth * fm_sin_turns(sul.vph));
        sul.vph += 5.2f * dt;
        if (sul.vph >= 1.0f)
            sul.vph -= 1.0f;
        sul.ph += f * dt;
        if (sul.ph >= 1.0f)
            sul.ph -= 1.0f;
        s = fm_sin_turns(sul.ph) + 0.22f * fm_sin_turns(2.0f * sul.ph) + 0.06f * fm_sin_turns(3.0f * sul.ph);
        hp = rnd01() * 2.0f - 1.0f - sul.nlp - 0.7f * sul.nbp;   /* the breath: noise about the second partial */
        sul.nbp += nf * hp;
        sul.nlp += nf * sul.nbp;
        s = (s + breath * sul.nbp) * sul.amp;     /* (HUSH and the mix's place: the gendèr's part, after) */
        sul.amp += (target - sul.amp) * ak;
        L[i] += s;
        R[i] += s;
    }
    sul.since += (float)n * dt;
    sul.nlp = fm_flush(sul.nlp);
    sul.nbp = fm_flush(sul.nbp);
    if (sul.amp < 1e-5f && !on) {
        sul.amp = 0.0f;
        sul.hz = 0.0f;
        sul.note = -32768;
    }
}

/* ---------------------------------------------------------------- render --- */
/* core 2: the first half of the voices into its buses; core 1 the rest, and the reverb's return of the
 * block before, meanwhile */
static int sounded2;
static void core2_job(uint32_t arg)
{
    uint32_t n = arg & 0x1FFu, t0 = plat_ticks24(), p, i;
    for (p = 0; p < NPARTS; p++)
        for (i = 0; i < n; i++)
            bus2[p][0][i] = bus2[p][1][i] = 0.0f;
    for (i = 0; i < n; i++)
        xs2[i] = 0.0f;
    sounded2 = bars_render(&bars, 0, BARS_VOICES / 2, bus2, xs2, (int)n);
    st_t[ST_VOICES2] = plat_ticks24() - t0;
    st_t[ST_CORE2] = st_t[ST_VOICES2];
}

/* HOME's swell: the output's loudness in dB, eased (in ~0.4 s, out ~1.5 s), against where it settled over
 * the last few seconds, so the light answers the music growing and falling */
static float sw_level, sw_settled;
static void swell_meter(const float *l, const float *r, uint32_t n)
{
    float s = 0.0f, db, lv, dt = (float)n / SR;
    uint32_t i;
    for (i = 0; i < n; i++)
        s += l[i] * l[i] + r[i] * r[i];
    db = 10.0f * fm_log2f(s / (float)(2u * n) + 1e-12f) * 0.30103f;
    lv = fm_clampf((db + 60.0f) / 45.0f, 0.0f, 1.0f);
    sw_level += (lv - sw_level) * (1.0f - fm_expf(-dt / (lv > sw_level ? 0.4f : 1.5f)));
    sw_settled += (sw_level - sw_settled) * (1.0f - fm_expf(-dt / 4.0f));
    eng_swell = fm_clampf(0.35f * sw_level + 1.2f * (sw_level - sw_settled), 0.0f, 1.0f);
}

void engine_render(int32_t *out_lr, uint32_t n)
{
    uint32_t t0 = plat_ticks24(), t, i, p, k;
    float vol = usb_full ? 1.0f : (float)plat_master() / 4096.0f, pk = 0.0f;
    int on2;
    if (n > 256u)
        n = 256u;
    for (k = 0; k < ST_N; k++)
        st_t[k] = 0;
    /* CTRL: commands, the motion, the cycle's strokes for this block */
    economy_block();
    scale_check();
    clk_block(n);
    drain();
    motion(n);
    if (!eng_running)
        eng_bpm_now = target_bpm();
    schedule(n);
    test_tick();
    t = plat_ticks24();
    st_t[ST_CTRL] = t - t0;
    on2 = plat_cpu2_run(core2_job, n);
    t = plat_ticks24();
    for (p = 0; p < NPARTS; p++)
        for (i = 0; i < n; i++)
            bus1[p][0][i] = bus1[p][1][i] = 0.0f;
    for (i = 0; i < n; i++)
        xs1[i] = 0.0f;
    bars_render(&bars, BARS_VOICES / 2, BARS_VOICES, bus1, xs1, (int)n);
    suling_block(bus1[PART_GENDER][0], bus1[PART_GENDER][1], n);
    st_t[ST_VOICES1] = plat_ticks24() - t;
    t = plat_ticks24();
    fxbus_wet_st(&fx, fx_send[fx_par ^ 1u], zero, wet_l, wet_r, (int)n);
    st_t[ST_FXWET] = plat_ticks24() - t;
    t = plat_ticks24();
    if (!on2 || plat_cpu2_wait())
        core2_job(n);
    else
        st_t[ST_WAIT] = plat_ticks24() - t;
    t = plat_ticks24();
    /* MIX: each part by its place and level into the output and the reverb's send */
    for (i = 0; i < n; i++)
        out_l[i] = out_r[i] = rev[i] = 0.0f;
    for (p = 0; p < NPARTS; p++) {
        float gl, gr, x = (eng_pan_now[p] + 1.0f) * 0.25f, g = part_g[p];
        float *bl = bus1[p][0], *br = bus1[p][1];
        gl = fm_sin_turns(x + 0.25f) * 1.41421356f * g;
        gr = fm_sin_turns(x) * 1.41421356f * g;
        glide_set(&gl_l[p], gl);
        glide_set(&gl_r[p], gr);
        glide_set(&gl_s[p], send_k * g);
        for (i = 0; i < n; i++) {
            bl[i] += bus2[p][0][i];
            br[i] += bus2[p][1][i];
            mono[i] = (bl[i] + br[i]) * 0.5f;
        }
        add_glide(out_l, bl, &gl_l[p], n);
        add_glide(out_r, br, &gl_r[p], n);
        add_glide(rev, mono, &gl_s[p], n);
    }
    for (i = 0; i < n; i++)                      /* a white key's SHIMMER: its own way into the space */
        xs1[i] += xs2[i];
    glide_set(&gl_x, 1.0f - (float)thr[TH_FREEZE] * (0.9f / 127.0f));
    add_glide(rev, xs1, &gl_x, n);
    {
        float *s = fx_send[fx_par];
        for (i = 0; i < n; i++) {
            s[i] = rev[i];                       /* for the next block */
            out_l[i] += wet_l[i];
            out_r[i] += wet_r[i];
        }
        for (i = n; i < 256u; i++)
            s[i] = 0.0f;
    }
    fx_par ^= 1u;
    master_process_st(&mst, out_l, out_r, (int)n, vol);
    swell_meter(out_l, out_r, n);
    for (i = 0; i < n; i++) {
        float l = out_l[i], r = out_r[i];
        eng_out_clips += (fm_fabsf(l) > out_lim) + (fm_fabsf(r) > out_lim);
        l = l > out_lim ? out_lim : l < -out_lim ? -out_lim : l;
        r = r > out_lim ? out_lim : r < -out_lim ? -out_lim : r;
        out_lr[2u * i] = (int32_t)(l * out_scale);
        out_lr[2u * i + 1u] = (int32_t)(r * out_scale);
        pk = fm_maxf(pk, fm_maxf(fm_fabsf(l), fm_fabsf(r)));
    }
    eng_quiet_frames = pk < 2.5e-4f ? (eng_quiet_frames + n > 0x7FFFFFFFu ? 0x7FFFFFFFu : eng_quiet_frames + n) : 0;
    {
        uint32_t q = (uint32_t)(fm_minf(pk, 1.0f) * 32767.0f);
        if (q > eng_level)
            eng_level = (uint16_t)q;
    }
    st_t[ST_OUT] = plat_ticks24() - t;
    st_t[ST_CORE1] = plat_ticks24() - t0;
    if (!on2)
        st_t[ST_CORE2] = 0;
    eng_frames += n;
    bench_block(n);
}

void engine_init(void)
{
    int i;
    defaults_init();
    engine_work_defaults();
    bars_init(&bars);
    fxbus_init(&fx, dly_buf, DLY_LEN);
    fxbus_reverb_buffers(&fx, rv_buf, rv_ps);
    fxbus_set(&fx, FX_DL_LEVEL, 0);              /* (the delay: unused) */
    master_init(&mst);
    for (i = 0; i < 8; i++)
        fx_cache[i] = -1;
    for (i = 0; i < 16; i++) {
        white_h[i].i = -1;
        white_h[i].gen = 0;
    }
    for (i = 0; i < H_N; i++)
        held[i].i = -1;
    cy.end_mul = 1.0f;
    eng_bpm_now = target_bpm();
    scale_check();
}
