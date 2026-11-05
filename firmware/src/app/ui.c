/* SPDX-License-Identifier: GPL-3.0-only */
/* Ombak's user interface (DESIGN.md: "Performance and the interface", "The holds"). HOME, where you play,
 * and a page one press away for everything else; Penumbra's buttons, encoders and screen.
 *
 *   HOME     the Ongkara (or, set in SYSTEM, the gong), its light the music's swell, the gong's flare and the
 *            tide; the gong cycle round its rim, a bead for each core note, the light travelling. KNOBS:
 *            OMBAK, DENSITY, SPACE, BREATH. SELECT: the mode, at the next gong. PRESETS: the gamelan, at the
 *            next gong. ALGO: the tempo
 *   SHAPE    ENV: ENSEMBLE, GENDÈR, BARS, REYONG, GONGS, TIDES, PAN, PAN 2 (ENV again or SELECT: the next);
 *            four knobs each, ALGO the page's own
 *   FX       SPACE, SPACE 2: Penumbra's reverb (SELECT: the row)
 *   GAMELAN  EDIT: the gamelan playing, a list (SELECT: the row, ALGO, a knob or SEL: change it)
 *   LIBRARY  LFO: the seven gamelan (SEL: open, at the next gong), then MINE 1-4 (SAVE: keep the work there;
 *            SEL: open it)
 *   SYSTEM   GLO: the settings; GLO again: BENCH
 * Buttons act when let go; held, they say what they do. SEQ the cycle, ARP the gongs (the cycle or the
 * hand), REC / PLAY the loop (PLAY held, or REC with PLAY: clear), OCT- / OCT+, SAVE.
 * Keys: white, your gendèr and the next core note; black 1-5 the gongs, 6-8 FASTER, SLOWER, the ENDING,
 * 9-11 the kotekan, reyong and ornaments in and out.
 * Holds: a gong (LEVEL, TONE, HUM, RING; ALGO its PLACES), FASTER / SLOWER (STEP, GLIDE; held, the tempo
 * keeps moving), the ENDING (AFTER, SLOWING, THINNING), a part (DENSITY or RATE or PICKUP, REGISTER, TURN or
 * OCTAVES, SPARSE; ALGO its level; it toggles when let go only if the knobs were not turned), a white key
 * (BENDS on its note: SHIMMER, OMBAK, PLACE, SWELL), FX (THROWS: SHIMMER, FREEZE, DIP, HUSH; ALGO the
 * OMBAK). Bends and throws drift back when let go; SAVE while holding FX keeps the throws.
 * The screen is three bands (header, main, knob strip), each drawn into Felucca's canvas and blitted only
 * where its pixels changed. */
#include "ombak.h"
#include "../dsp/fastmath.h"
#include "ongkara.h"

/* ================================================================ model === */
static const int8_t KEY_WHITE[NKEYS] = {0, -1, 1, -1, 2, -1, 3, 4, -1, 5, -1, 6, 7, -1, 8, -1, 9, -1, 10, 11, -1, 12, -1, 13, 14, -1, 15};
static const int8_t KEY_BLACK[NKEYS] = {-1, 0, -1, 1, -1, 2, -1, -1, 3, -1, 4, -1, -1, 5, -1, 6, -1, 7, -1, -1, 8, -1, 9, -1, -1, 10, -1};
static uint8_t BLACK_KEY[11], WHITE_KEY[16];
enum { BK_FASTER = 5, BK_SLOWER = 6, BK_ENDING = 7, BK_KOT = 8, BK_REY = 9, BK_ORN = 10 };

enum { V_HOME, V_SHAPE, V_EDIT, V_FX, V_LIB, V_SYSTEM, V_BENCH, NVIEWS };
static const char *const VIEW_N[NVIEWS] = {"HOME", "SHAPE", "GAMELAN", "FX", "LIBRARY", "SYSTEM", "BENCH"};

#define HELP_MS 600u
#define HOLD_MS 300u                       /* a key held this long is a hold: its knobs show */
#define IDLE_HOME_MS 30000u
#define BACK_S 2.0f                        /* bends and throws let go drift back over this */

enum { L_PAGE, L_GONG, L_CUE, L_END, L_PART, L_BEND, L_THROW };

typedef struct {
    uint8_t view, page, fxrow, erow, lrow, row;
    int8_t oct;
    uint32_t btn, keys;
    uint32_t press_t[NB], helped, used;
    uint32_t kdown_t[NKEYS];               /* when each key went down */
    char msg[28];
    uint32_t msg_until, frame;
    uint8_t dirty;
    uint32_t act_t, touch_t;
    int8_t hold_bk;                        /* the black key held (the latest), -1 none */
    uint8_t hold_turned;                   /* its knobs moved */
    int8_t hold_w;                         /* the white key held (the latest), -1 none */
    float bend[16][4];                     /* each white key's bends: SHIMMER, OMBAK, PLACE, SWELL (as sent: 0, 42, 64, 80) */
    uint8_t bend_back[16];
    float thr[TH_N];                       /* the throws: SHIMMER, FREEZE, DIP, HUSH, OMBAK */
    int16_t thr_sent[TH_N];
    uint8_t thr_kept, thr_back;
    float tempo_acc;                       /* a held cue's tempo moving, between its steps */
    int8_t last_pend_mode, last_pend_ens;
    uint8_t last_loop;
    uint8_t rs_ask;
    uint32_t rs_until;
    uint8_t mine;                          /* the MINE slot opened or kept (1..4), 0 none */
    uint8_t ab_on, ab_b;                   /* A/B in use; B playing */
    uint8_t sleep, sleep_step;             /* SLEEP's choice (this session's); how far its letting go has got */
    uint32_t sleep_at;                     /* when it is all still */
} ui_t;
static ui_t ui;
/* SLEEP (Penumbra's): in so many minutes Ombak lets go: in the last five it cues the ending (the ensemble slows,
 * thins and closes on the gong), and at the end anything still going stops and rings out */
#define SLEEPS 5
static const uint8_t SLEEP_MIN[SLEEPS] = {0, 20, 40, 60, 90};
#define SLEEP_LAST_MS 300000u
static int sleep_left_ms(void) { return (int)(ui.sleep_at - plat_ms()); }

/* ============================================================ text helpers === */
static char *put_s(char *b, const char *s)
{
    while (*s)
        *b++ = *s++;
    *b = 0;
    return b;
}
static char *put_i(char *b, int v)
{
    char t[12];
    int n = 0, neg = v < 0;
    uint32_t u = (uint32_t)(neg ? -v : v);
    do {
        t[n++] = (char)('0' + u % 10u);
        u /= 10u;
    } while (u);
    if (neg)
        *b++ = '-';
    while (n)
        *b++ = t[--n];
    *b = 0;
    return b;
}
static char *put_d1(char *b, uint32_t x10)
{
    b = put_i(b, (int)(x10 / 10u));
    *b++ = '.';
    *b++ = (char)('0' + x10 % 10u);
    *b = 0;
    return b;
}
static char *put_sgn(char *b, int v) { return put_i(put_s(b, v > 0 ? "+" : ""), v); }
static int pct_of(int x) { return (x * 100 + 63) / 127; }
static char *put_pct(char *b, int x) { return put_s(put_i(b, pct_of(x)), "%"); }
static char *put_secs(char *b, float s)          /* "6.3 S", "14 S", "2.1 MIN" */
{
    if (s >= 60.0f)
        return put_s(put_d1(b, (uint32_t)(s / 6.0f + 0.5f)), " MIN");
    if (s < 10.0f)
        return put_s(put_d1(b, (uint32_t)(s * 10.0f + 0.5f)), " S");
    return put_s(put_i(b, (int)(s + 0.5f)), " S");
}
static int clamp_i(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static int wrap_i(int v, int n) { return ((v % n) + n) % n; }
static int round_i(float x) { return (int)(x + (x < 0.0f ? -0.5f : 0.5f)); }

static void say_as(const char *a, const char *b, uint32_t flags)
{
    const char *p;
    uint32_t n = 0;
    for (p = a; *p && n < sizeof ui.msg - 1u; p++)
        ui.msg[n++] = *p;
    for (p = b; p && *p && n < sizeof ui.msg - 1u; p++)
        ui.msg[n++] = *p;
    ui.msg[n] = 0;
    ui.msg_until = plat_ms() + 1300u;
    ann_say(a, b, flags);
}
static void say(const char *a, const char *b) { say_as(a, b, ANN_NOW); }
static void warn(const char *a, const char *b) { say_as(a, b, ANN_NOW | ANN_KEEP); }
static void changed(void)
{
    ui.dirty = 1;
    ui.act_t = plat_ms();
}

/* ================================================================ the scale === */
static g_scale_t usc;                             /* the UI's own (the keys' names) */
static void scale_now(void)
{
    int t = eng_w.p[P_TUNING] % G_TUNINGS, m = g_modes(t) ? eng_w.p[P_MODE] % g_modes(t) : 0;
    g_scale(t, m, &usc);
}
static float tempo_bpm(int p) { return 3.0f * fm_exp2f((float)p * (5.0f / 127.0f)); }
static char *put_tempo(char *b, float bpm)        /* "40 BPM", or slow: "10.0 S A BEAT" */
{
    if (bpm < 12.0f)
        return put_s(put_d1(b, (uint32_t)(600.0f / bpm + 0.5f)), " S A BEAT");
    return put_s(put_i(b, (int)(bpm + 0.5f)), " BPM");
}

/* ================================================================ the knobs === */
/* what a knob turns: a param of the work (or, rel, the held gong's field), shown its own way */
enum { KD_PCT, KD_LVL, KD_ASMADE, KD_OMBAK, KD_PAN, KD_SWAY, KD_TRATE, KD_SRATE, KD_SHAPE, KD_OFFSET, KD_DECAY,
       KD_SIZE, KD_INTERVAL, KD_LOWCUT, KD_SUB, KD_REG, KD_RRATE, KD_TURN, KD_PICK, KD_STEP, KD_GLIDE, KD_AFTER,
       KD_TONE, KD_PLACES };
typedef struct { const char *name; uint8_t p, kind, rel; } knob_t;
static const uint8_t KD_MAX[] = {[KD_INTERVAL] = 2, [KD_SUB] = 4, [KD_REG] = 2, [KD_RRATE] = 1, [KD_AFTER] = 1,
                                 [KD_TONE] = 4, [KD_PLACES] = 2, [KD_PLACES + 1] = 0};
static int kd_max(int kind) { return kind < (int)sizeof KD_MAX && KD_MAX[kind] ? KD_MAX[kind] : 127; }
static int knob_p(const knob_t *k, int gong) { return k->rel ? P_GONG(gong, k->p) : k->p; }
static void knob_text(const knob_t *k, int gong, char *b)
{
    static const char *const INTV[3] = {"OCTAVE", "FIFTH", "TWELFTH"}, *const REG[3] = {"LOW", "AS MADE", "HIGH"};
    static const char *const SUB[5] = {"AS HOME", "2 A BEAT", "4 A BEAT", "8 A BEAT", "16 A BEAT"};
    static const char *const PLACES[3] = {"OFF", "AS MADE", "EVERY OTHER"}, *const PL_LAN[3] = {"OFF", "AT THE HALF", "HALF, OTHER"};
    int v = eng_w.p[knob_p(k, gong)];
    float x = (float)v / 127.0f;
    switch (k->kind) {
    case KD_LVL: put_s(put_i(b, v), "%"); break;
    case KD_ASMADE: {
        int o = round_i(((float)v / 128.0f - 0.5f) * 200.0f);
        if (o == 0)
            put_s(b, "AS MADE");
        else
            put_s(put_sgn(b, o), "%");
        break;
    }
    case KD_OMBAK: put_s(put_d1(b, (uint32_t)(2.5f * fm_exp2f((float)v * (5.3219281f / 127.0f)) + 0.5f)), " HZ"); break;
    case KD_PAN: {
        int o = round_i(((float)v - 64.0f) * (100.0f / 63.0f));
        if (o == 0)
            put_s(b, "MIDDLE");
        else
            put_s(put_i(put_s(b, o < 0 ? "L " : "R "), o < 0 ? -o : o), "%");
        break;
    }
    case KD_SWAY: if (!v) put_s(b, "STILL"); else put_pct(b, v); break;
    case KD_TRATE: put_secs(b, 8.0f * fm_exp2f((float)v * (5.0f / 127.0f))); break;
    case KD_SRATE: if (!v) put_s(b, "THE TIDE"); else put_secs(b, 60.0f * fm_exp2f((float)(v - 1) * (-6.9068906f / 126.0f))); break;
    case KD_SHAPE: put_s(b, v < 6 ? "EVEN" : v > 121 ? "WANDERING" : ""); if (v >= 6 && v <= 121) put_pct(b, v); break;
    case KD_OFFSET: put_s(b, v < 3 ? "IN SYNC" : v > 124 ? "MIRRORED" : ""); if (v >= 3 && v <= 124) put_pct(b, v); break;
    case KD_DECAY: put_secs(b, fm_exp2f(5.32f * x)); break;
    case KD_SIZE: put_s(put_d1(b, (uint32_t)((0.5f + x) * 10.0f + 0.5f)), "X"); break;
    case KD_INTERVAL: put_s(b, INTV[v % 3]); break;
    case KD_LOWCUT: put_s(put_i(b, (int)(30.0f * fm_exp2f(x * 4.7369656f) + 0.5f)), " HZ"); break;
    case KD_SUB: put_s(b, SUB[v % 5]); break;
    case KD_REG: put_s(b, REG[v % 3]); break;
    case KD_RRATE: put_s(b, v ? "A QUARTER" : "HALF"); break;
    case KD_TURN: if (v == 64) put_s(b, "HALFWAY"); else put_s(put_i(b, (int)((0.25f + 0.5f * x) * 100.0f + 0.5f)), "%"); break;
    case KD_PICK: put_s(put_i(b, (int)((1.0f / 16.0f + x * 7.0f / 16.0f) * 100.0f + 0.5f)), "% EARLY"); break;
    case KD_STEP: put_s(put_i(b, (int)((0.05f + 0.45f * x) * 100.0f + 0.5f)), "%"); break;
    case KD_GLIDE: put_s(put_i(b, (int)((0.03f + 0.97f * x) * 100.0f + 0.5f)), "%"); break;
    case KD_AFTER: put_s(b, v ? "GONG AFTER" : "NEXT GONG"); break;
    case KD_TONE:                                 /* steps along the mode from the tone as made */
        if (v == 2)
            put_s(b, "AS MADE");
        else
            put_s(put_sgn(b, v - 2), v - 2 == 1 || v - 2 == -1 ? " TONE" : " TONES");
        break;
    case KD_PLACES: put_s(b, (gong == G_LANANG ? PL_LAN : PLACES)[v % 3]); break;
    default: put_pct(b, v); break;
    }
}
static void knob_turn_p(const knob_t *k, int gong, int e)
{
    int p = knob_p(k, gong), mx = kd_max(k->kind), v = eng_w.p[p];
    v = mx < 127 ? clamp_i(v + (e > 0 ? 1 : -1), 0, mx) : clamp_i(v + 2 * e, 0, mx);
    eng_w.p[p] = (uint8_t)v;
    changed();
}

typedef struct { const char *name; knob_t k[4]; knob_t algo; } page_t;
#define NOKNOB {0, 0, 0, 0}
static const knob_t MACRO[4] = {{"OMBAK", P_OMBAK, KD_OMBAK, 0}, {"DENSITY", P_DENSITY, KD_PCT, 0},
                                {"SPACE", P_SPACE, KD_PCT, 0}, {"BREATH", P_BREATH, KD_PCT, 0}};
#define NPG 8
static const page_t PAGE[NPG] = {
    {"ENSEMBLE", {{"CORE", P_L_CORE, KD_LVL, 0}, {"KOTEKAN", P_L_KOT, KD_LVL, 0}, {"REYONG", P_L_REY, KD_LVL, 0},
                  {"GONGS", P_L_GONG, KD_LVL, 0}}, {"GENDER", P_L_GENDER, KD_LVL, 0}},
    {"GENDER", {{"BRIGHT", P_TG_BRIGHT, KD_ASMADE, 0}, {"RING", P_TG_RING, KD_ASMADE, 0}, {"BLOOM", P_TG_BLOOM, KD_ASMADE, 0},
                {"DAMP", P_TG_DAMP, KD_ASMADE, 0}}, {"LEVEL", P_L_GENDER, KD_LVL, 0}},
    {"BARS", {{"BRIGHT", P_TB_BRIGHT, KD_ASMADE, 0}, {"RING", P_TB_RING, KD_ASMADE, 0}, {"MALLET", P_TB_MALLET, KD_ASMADE, 0},
              {"BLOOM", P_TB_BLOOM, KD_ASMADE, 0}}, {"CORE", P_L_CORE, KD_LVL, 0}},
    {"REYONG", {{"BRIGHT", P_TR_BRIGHT, KD_ASMADE, 0}, {"RING", P_TR_RING, KD_ASMADE, 0}, {"MALLET", P_TR_MALLET, KD_ASMADE, 0},
                NOKNOB}, {"LEVEL", P_L_REY, KD_LVL, 0}},
    {"GONGS", {{"HUM", P_TO_HUM, KD_ASMADE, 0}, {"SWELL", P_TO_SWELL, KD_ASMADE, 0}, {"BODY", P_TO_BODY, KD_ASMADE, 0},
               {"OMBAK", P_TO_OMBAK, KD_ASMADE, 0}}, {"LEVEL", P_L_GONG, KD_LVL, 0}},
    {"TIDES", {{"RATE", P_T_RATE, KD_TRATE, 0}, {"SPACE", P_T_SPACE, KD_PCT, 0}, {"SWELL", P_T_SWELL, KD_PCT, 0},
               {"OMBAK", P_T_OMBAK, KD_PCT, 0}}, {"SHAPE", P_T_SHAPE, KD_SHAPE, 0}},
    {"PAN", {{"GENDER", P_PAN + PART_GENDER, KD_PAN, 0}, {"CORE", P_PAN + PART_CORE, KD_PAN, 0},
             {"KOTEKAN", P_PAN + PART_KOT, KD_PAN, 0}, {"GONGS", P_PAN + PART_GONGS, KD_PAN, 0}}, {"RATE", P_S_RATE, KD_SRATE, 0}},
    {"PAN 2", {{"GENDER", P_SWAY + PART_GENDER, KD_SWAY, 0}, {"CORE", P_SWAY + PART_CORE, KD_SWAY, 0},
               {"KOTEKAN", P_SWAY + PART_KOT, KD_SWAY, 0}, {"GONGS", P_SWAY + PART_GONGS, KD_SWAY, 0}},
     {"OFFSET", P_S_OFF, KD_OFFSET, 0}},
};
#define NFXROWS 2
static const page_t FXROW[NFXROWS] = {
    {"SPACE", {{"DECAY", P_FX + 0, KD_DECAY, 0}, {"SIZE", P_FX + 1, KD_SIZE, 0}, {"SHIMMER", P_FX + 2, KD_PCT, 0},
               {"TONE", P_FX + 3, KD_PCT, 0}}, NOKNOB},
    {"SPACE 2", {{"MOTION", P_FX + 4, KD_PCT, 0}, {"INTERVAL", P_FX + 5, KD_INTERVAL, 0}, {"LOW CUT", P_FX + 6, KD_LOWCUT, 0},
                 {"LEVEL", P_FX + 7, KD_PCT, 0}}, NOKNOB},
};
/* the holds' knobs */
static const page_t GONG_HOLD = {"", {{"LEVEL", GF_LEVEL, KD_LVL, 1}, {"TONE", GF_TONE, KD_TONE, 1}, {"HUM", GF_HUM, KD_ASMADE, 1},
                                      {"RING", GF_RING, KD_ASMADE, 1}}, {"PLACES", GF_PLACES, KD_PLACES, 1}};
static const page_t CUE_HOLD = {"", {{"STEP", P_C_STEP, KD_STEP, 0}, {"GLIDE", P_C_GLIDE, KD_GLIDE, 0}, NOKNOB, NOKNOB}, NOKNOB};
static const page_t END_HOLD = {"ENDING", {{"AFTER", P_E_AFTER, KD_AFTER, 0}, {"SLOWING", P_E_SLOW, KD_ASMADE, 0},
                                           {"THINNING", P_E_THIN, KD_ASMADE, 0}, NOKNOB}, NOKNOB};
static const page_t PART_HOLD[3] = {
    {"KOTEKAN", {{"DENSITY", P_K_SUB, KD_SUB, 0}, {"REGISTER", P_K_REG, KD_REG, 0}, {"TURN", P_K_TURN, KD_TURN, 0},
                 {"SPARSE", P_K_SPARSE, KD_PCT, 0}}, {"LEVEL", P_L_KOT, KD_LVL, 0}},
    {"REYONG", {{"RATE", P_R_RATE, KD_RRATE, 0}, {"REGISTER", P_R_REG, KD_REG, 0}, {"TURN", P_R_TURN, KD_TURN, 0},
                {"SPARSE", P_R_SPARSE, KD_PCT, 0}}, {"LEVEL", P_L_REY, KD_LVL, 0}},
    {"ORNAMENTS", {{"PICKUP", P_O_PICK, KD_PICK, 0}, {"OCTAVES", P_O_OCT, KD_ASMADE, 0}, {"REGISTER", P_O_REG, KD_REG, 0},
                   {"SPARSE", P_O_SPARSE, KD_PCT, 0}}, {"LEVEL", P_L_ORN, KD_LVL, 0}},
};
static const char *const GONG_N[G_N] = {"GONG", "GONG LANANG", "KEMPUR", "KEMONG", "KAJAR"};
static const char *const BEND_N[4] = {"SHIMMER", "OMBAK", "PLACE", "SWELL"};
static const char *const THR_N[TH_N] = {"SHIMMER", "FREEZE", "DIP", "HUSH", "OMBAK"};
static const uint8_t BEND_AS[4] = {0, 42, 64, 80};

/* what the controls play now: the page's, or a hold's */
static int layer(void)
{
    uint32_t now = plat_ms();
    if (ui.btn >> B_FX & 1u)
        return L_THROW;
    if (ui.hold_bk >= 0 && now - ui.kdown_t[BLACK_KEY[ui.hold_bk]] >= HOLD_MS) {
        int k = ui.hold_bk;
        return k < G_N ? L_GONG : k <= BK_SLOWER ? L_CUE : k == BK_ENDING ? L_END : L_PART;
    }
    if (ui.hold_w >= 0 && now - ui.kdown_t[WHITE_KEY[ui.hold_w]] >= HOLD_MS)
        return L_BEND;
    return L_PAGE;
}
static const page_t *hold_page(int l)
{
    switch (l) {
    case L_GONG: return &GONG_HOLD;
    case L_CUE: return &CUE_HOLD;
    case L_END: return &END_HOLD;
    case L_PART: return &PART_HOLD[ui.hold_bk - BK_KOT];
    default: return 0;
    }
}
static const knob_t *page_knobs(void)
{
    return ui.view == V_SHAPE ? PAGE[ui.page % NPG].k : ui.view == V_FX ? FXROW[ui.fxrow % NFXROWS].k : ui.view == V_HOME ? MACRO : 0;
}

/* ================================================================ the moves === */
static void thr_text(int i, char *b)
{
    float v = ui.thr[i];
    if (i == TH_DIP) {
        if (v > -3.0f && v < 3.0f)
            put_s(b, "OPEN");
        else
            put_pct(put_s(b, v < 0.0f ? "LP " : "HP "), round_i(v < 0.0f ? -v : v));
    } else if (i == TH_OMBAK) {
        put_s(b, "X");
        put_d1(b + 1, (uint32_t)(fm_exp2f(v * (2.0f / 127.0f)) * 10.0f + 0.5f));
    } else {
        put_pct(b, round_i(v));
    }
}
static void bend_text(int w, int k, char *b)
{
    float v = ui.bend[w][k];
    switch (k) {
    case 1:
        if (v < 1.0f)
            put_s(b, "STILL");
        else
            put_d1(put_s(b, "X"), (uint32_t)(v * (10.0f / 42.0f) + 0.5f));
        break;
    case 2: {
        int o = round_i((v - 64.0f) * (100.0f / 63.0f));
        if (!o)
            put_s(b, "MIDDLE");
        else
            put_s(put_i(put_s(b, o < 0 ? "L " : "R "), o < 0 ? -o : o), "%");
        break;
    }
    case 3: put_s(put_i(b, (int)(v * 100.0f / 80.0f + 0.5f)), "%"); break;
    default: put_pct(b, round_i(v)); break;
    }
}
/* the moves to the engine when they change; let go, they drift back over BACK_S from all the way */
static void moves_tick(float dt)
{
    int i, w;
    float step = 127.0f * dt / BACK_S;
    if (ui.thr_back) {
        int moving = 0;
        for (i = 0; i < TH_N; i++) {
            float *v = &ui.thr[i];
            *v = *v > 0.0f ? fm_maxf(*v - step, 0.0f) : fm_minf(*v + step, 0.0f);
            moving |= *v != 0.0f;
        }
        ui.thr_back = (uint8_t)moving;
    }
    for (i = 0; i < TH_N; i++) {
        int v = round_i(ui.thr[i]);
        if (v != ui.thr_sent[i]) {
            ui.thr_sent[i] = (int16_t)v;
            engine_throw(i, v);
        }
    }
    for (w = 0; w < 16; w++)
        if (ui.bend_back[w]) {
            int moving = 0;
            for (i = 0; i < 4; i++) {
                float *v = &ui.bend[w][i], t = (float)BEND_AS[i];
                *v = *v > t ? fm_maxf(*v - step, t) : fm_minf(*v + step, t);
                moving |= *v != t;
                engine_bend(w, i, round_i(*v));
            }
            ui.bend_back[w] = (uint8_t)moving;
        }
}
static int any_thrown(void)
{
    int i;
    for (i = 0; i < TH_N; i++)
        if (ui.thr[i] != 0.0f)
            return 1;
    return 0;
}

/* ================================================================ the gamelan === */
static int pend_mode_or_mode(void) { return eng_pend_mode >= 0 ? eng_pend_mode : eng_w.p[P_MODE]; }
static void mode_turn(int e)                      /* SELECT on HOME: the modes, arriving with the gong */
{
    int t = eng_w.p[P_TUNING] % G_TUNINGS, n = g_modes(t), m;
    if (!n) {
        warn("THIS TUNING HAS NO MODES", 0);
        return;
    }
    m = wrap_i(pend_mode_or_mode() + (e > 0 ? 1 : -1), n);
    if (!eng_running) {
        eng_w.p[P_MODE] = (uint8_t)m;
        changed();
        say(g_mode_name(t, m), 0);
    } else {
        engine_pend_mode(m == eng_w.p[P_MODE] ? -1 : m);
        say(g_mode_name(t, m), m == eng_w.p[P_MODE] ? "" : " AT THE GONG");
    }
}
static void gamelan_go(int e)                     /* PRESETS, LIBRARY's SEL: a gamelan, arriving with the gong */
{
    e = wrap_i(e, G_ENSEMBLES);
    ui.mine = 0;
    if (!eng_running) {
        engine_ensemble(e);
        changed();
        say(G_ENSEMBLE[e].name, 0);
    } else {
        engine_pend_ens(e == eng_w.p[P_ENS] ? -1 : e);
        say(G_ENSEMBLE[e].name, e == eng_w.p[P_ENS] ? "" : " AT THE GONG");
    }
}
static void cue(int dir)                          /* FASTER / SLOWER: a STEP (the cycle glides to it) */
{
    float step = 0.05f + 0.45f * (float)eng_w.p[P_C_STEP] / 127.0f;
    int d = round_i(127.0f / 5.0f * fm_log2f(1.0f + step));
    char t[20];
    eng_w.p[P_TEMPO] = (uint8_t)clamp_i(eng_w.p[P_TEMPO] + dir * (d ? d : 1), 0, 127);
    changed();
    put_tempo(t, tempo_bpm(eng_w.p[P_TEMPO]));
    say(dir > 0 ? "FASTER " : "SLOWER ", t);
}

/* GAMELAN (EDIT): the rows */
enum { ER_TUNING, ER_MODE, ER_FORM, ER_CYCLE, ER_GONGS, ER_CORE, ER_KOT, ER_STYLE, ER_REY, ER_ORN, NER };
static const char *const ER_N[NER] = {"TUNING", "MODE", "FORM", "CYCLE", "THE GONGS", "CORE ON", "KOTEKAN ON",
                                      "KOTEKAN", "REYONG", "ORNAMENTS"};
static void er_text(int r, char *b)
{
    const uint8_t *p = eng_w.p;
    switch (r) {
    case ER_TUNING: put_s(b, G_TUNING[p[P_TUNING] % G_TUNINGS].name); break;
    case ER_MODE: put_s(b, g_mode_name(p[P_TUNING], p[P_MODE])); break;
    case ER_FORM: put_s(b, G_FORM[p[P_FORM] % G_FORMS].name); break;
    case ER_CYCLE: put_s(put_i(b, engine_cycle_len()), " BEATS"); break;
    case ER_GONGS: put_s(b, p[P_AUTO] ? "THE CYCLE" : "BY HAND"); break;
    case ER_CORE: put_s(b, p[P_CORE_G] ? "GENDER" : "JUBLAG"); break;
    case ER_KOT: put_s(b, p[P_KOT_G] ? "GENDER" : "GANGSA"); break;
    case ER_STYLE: put_s(b, G_STYLE[p[P_K_STYLE] % G_STYLES]); break;
    case ER_REY: put_s(b, p[P_PARTS] & PB_REY ? "IN" : "OUT"); break;
    default: put_s(b, p[P_PARTS] & PB_ORN ? "IN" : "OUT"); break;
    }
}
static void er_turn(int r, int e)
{
    uint8_t *p = eng_w.p;
    int d = e > 0 ? 1 : -1;
    char v[32];
    switch (r) {
    case ER_TUNING: p[P_TUNING] = (uint8_t)wrap_i(p[P_TUNING] + d, G_TUNINGS); p[P_MODE] = 0; break;
    case ER_MODE: if (g_modes(p[P_TUNING])) p[P_MODE] = (uint8_t)wrap_i(p[P_MODE] + d, g_modes(p[P_TUNING])); break;
    case ER_FORM: engine_form(wrap_i(p[P_FORM] + d, G_FORMS)); break;
    case ER_CYCLE:                                /* (from a form: FREE, at the form's length, then a step) */
        if (p[P_FORM] % G_FORMS) {
            int len = engine_cycle_len();
            p[P_CYCLE] = (uint8_t)(len >= 32 ? 3 : len >= 16 ? 2 : len >= 8 ? 1 : 0);
            engine_form(0);
        }
        p[P_CYCLE] = (uint8_t)wrap_i(p[P_CYCLE] + d, 4);
        break;
    case ER_STYLE: p[P_K_STYLE] = (uint8_t)wrap_i(p[P_K_STYLE] + d, G_STYLES); break;
    case ER_GONGS: p[P_AUTO] ^= 1u; break;
    case ER_CORE: p[P_CORE_G] ^= 1u; break;
    case ER_KOT: p[P_KOT_G] ^= 1u; break;
    case ER_REY: p[P_PARTS] ^= PB_REY; break;
    default: p[P_PARTS] ^= PB_ORN; break;
    }
    changed();
    er_text(r, v);
    say(v, 0);
}

/* SYSTEM */
enum { R_HOME, R_SLEEP, R_AUTOPLAY, R_MIDI, R_OUTLOUD, R_USBFULL, R_CORES, R_RESET, R_ABOUT, NROWS };
static const char *const ROW_N[NROWS] = {"HOME", "SLEEP", "PLAY AT POWER-ON", "MIDI CHANNEL", "OUTPUT LEVEL",
                                         "USB AUDIO", "2ND CORE", "RESET SOUND", "ABOUT"};
static void row_text(int r, char *b)
{
    switch (r) {
    case R_HOME: put_s(b, cfg.home ? "THE GONG" : "THE ONGKARA"); break;
    case R_AUTOPLAY: put_s(b, cfg.autoplay ? "ON" : "OFF"); break;
    case R_MIDI: if (cfg.midi_ch) put_i(b, cfg.midi_ch); else put_s(b, "OFF"); break;
    case R_SLEEP:
        if (ui.sleep)
            put_s(put_i(b, SLEEP_MIN[ui.sleep % SLEEPS]), " MIN");
        else
            put_s(b, "OFF");
        break;
    case R_OUTLOUD: put_s(b, cfg.out_loud ? "LOUD" : "NORMAL"); break;
    case R_USBFULL: put_s(b, cfg.usb_full ? "FULL" : "MASTER"); break;
    case R_CORES: put_s(b, cfg.one_core ? "OFF" : "ON"); break;
    case R_RESET: put_s(b, ui.rs_ask && (int32_t)(ui.rs_until - plat_ms()) > 0 ? "SEL AGAIN" : "SEL TWICE"); break;
    default: put_s(b, OMB_VERSION); break;
    }
}
static void row_set(int r, int e)                 /* SEL (e 1) or a knob turned */
{
    char v[24];
    switch (r) {
    case R_HOME: cfg.home ^= 1u; break;
    case R_AUTOPLAY: cfg.autoplay ^= 1u; break;
    case R_MIDI: cfg.midi_ch = (uint8_t)wrap_i(cfg.midi_ch + (e < 0 ? -1 : 1), 17); break;   /* 0 off, 1 .. 16 */
    case R_SLEEP:                                 /* (from now; not saved) */
        ui.sleep = (uint8_t)wrap_i(ui.sleep + (e < 0 ? -1 : 1), SLEEPS);
        ui.sleep_at = plat_ms() + (uint32_t)SLEEP_MIN[ui.sleep] * 60000u;
        ui.sleep_step = 0;
        row_text(r, v);
        say("SLEEP ", v);
        return;
    case R_OUTLOUD: cfg.out_loud ^= 1u; break;
    case R_USBFULL: cfg.usb_full ^= 1u; break;
    case R_CORES: cfg.one_core ^= 1u; break;
    case R_RESET:
        if (ui.rs_ask && (int32_t)(ui.rs_until - plat_ms()) > 0) {
            int e = eng_w.p[P_ENS], t = eng_w.p[P_TUNING], m = eng_w.p[P_MODE];
            engine_work_defaults();               /* every param as made; the gamelan playing stays */
            engine_ensemble(e);
            eng_w.p[P_TUNING] = (uint8_t)t;
            eng_w.p[P_MODE] = (uint8_t)m;
            ui.rs_ask = 0;
            changed();
            say("THE SOUND AS MADE", 0);
            return;
        }
        ui.rs_ask = 1;
        ui.rs_until = plat_ms() + 3000u;
        warn("SEL AGAIN TO RESET", 0);
        return;
    default: return;
    }
    settings_apply();
    changed();
    row_text(r, v);
    say(ROW_N[r], 0);
}

/* LIBRARY: the gamelan, then MINE (four works of your own: SAVE there keeps the work playing, SEL opens it) */
#define NLIB (G_ENSEMBLES + MINE_N)
static void lib_name(int r, char *b)
{
    if (r < G_ENSEMBLES)
        put_s(b, G_ENSEMBLE[r].name);
    else
        put_i(put_s(b, "MINE "), r - G_ENSEMBLES + 1);
}
static void lib_open(int r)
{
    char n[24];
    if (r < G_ENSEMBLES) {
        ui.mine = 0;
        gamelan_go(r);
        return;
    }
    lib_name(r, n);
    if (mine_open(r - G_ENSEMBLES)) {
        warn("EMPTY: SAVE KEEPS THE WORK HERE", 0);
        return;
    }
    ui.mine = (uint8_t)(r - G_ENSEMBLES + 1);
    ui.ab_on = 0;                                 /* (A/B starts again from what was opened) */
    engine_pend_ens(-1);
    engine_pend_mode(-1);
    changed();
    say(n, 0);
}

/* A/B (SHAPE's SEL): two shapes of the gamelan playing, this session's: everything shaped (HOME's knobs to the FX),
 * not the gamelan, its form or its kotekan; B starts as a copy of A */
static uint8_t ab_other[P_FX_END + 1];
static void ab_swap(void)
{
    int i;
    if (!ui.ab_on) {
        for (i = P_OMBAK; i <= P_FX_END; i++)
            ab_other[i] = eng_w.p[i];
        ui.ab_on = 1;
    }
    for (i = P_OMBAK; i <= P_FX_END; i++) {
        uint8_t t = eng_w.p[i];
        eng_w.p[i] = ab_other[i];
        ab_other[i] = t;
    }
    ui.ab_b ^= 1u;
    changed();
    say(ui.ab_b ? "B" : "A", ui.ab_b ? ": SEL BACK TO A" : ": SEL TO B");
}

/* ================================================================ saving === */
#define SAVE_QUIET_MS 2000u
static void save_now(void)
{
    int a = settings_save(), b = work_save();
    ui.dirty = 0;
    if (a || b)
        warn("SAVE FAILED", 0);
}
/* a save waits until nothing has changed for 2 s and the sound has been silent for 1 s: writing the flash stops
 * the audio for a moment (storage.c), which would be heard as a glitch over anything still ringing */
static void autosave(void)
{
    if (ui.dirty && plat_ms() - ui.act_t >= SAVE_QUIET_MS && engine_quiet_ms() >= 1000u)
        save_now();
}

/* ================================================================ buttons === */
static const char *const HELP[NB] = {
    [B_FX] = "FX. HELD: THROWS", [B_SEL] = "THE PAGE'S ACTION. SHAPE: A/B", [B_ENV] = "SHAPE", [B_LFO] = "LIBRARY",
    [B_EDIT] = "GAMELAN", [B_GLO] = "SYSTEM, BENCH", [B_HOME] = "HOME", [B_SAVE] = "SAVE. LIBRARY: INTO MINE. HOLDING FX: KEEP",
    [B_ARP] = "THE GONGS: CYCLE OR HAND", [B_SEQ] = "THE CYCLE: START, STOP", [B_REC] = "LOOP THE CORE",
    [B_OCTDN] = "WHITE KEYS DOWN", [B_OCTUP] = "WHITE KEYS UP"};
static void set_view(int v)
{
    ui.view = (uint8_t)v;
    ann_say(VIEW_N[v], 0, ANN_NOW);
}
static void button(int i)
{
    switch (i) {
    case B_HOME: set_view(V_HOME); break;
    case B_ENV:
        if (ui.view == V_SHAPE) {
            ui.page = (uint8_t)((ui.page + 1) % NPG);
            say(PAGE[ui.page].name, 0);
        } else
            set_view(V_SHAPE);
        break;
    case B_FX:
        if (ui.view == V_FX)
            ui.fxrow = (uint8_t)((ui.fxrow + 1) % NFXROWS);
        set_view(V_FX);
        break;
    case B_EDIT: set_view(V_EDIT); break;
    case B_LFO: set_view(V_LIB); break;
    case B_GLO: set_view(ui.view == V_SYSTEM ? V_BENCH : V_SYSTEM); break;
    case B_SEQ:
        engine_run(!eng_running);
        say(eng_running ? "STOP: IT RINGS OUT" : "THE CYCLE", 0);
        break;
    case B_ARP:
        eng_w.p[P_AUTO] ^= 1u;
        changed();
        say(eng_w.p[P_AUTO] ? "THE CYCLE KEEPS THE GONGS" : "THE GONGS BY HAND", 0);
        break;
    case B_REC:
        engine_loop(LOOP_REC);
        say(eng_loop_state == LS_ARM || eng_loop_state == LS_REC ? "RECORDING CANCELLED" : "LOOP: FROM THE NEXT CYCLE", 0);
        break;
    case B_PLAY:
        if (eng_loop_state == LS_OFF && !eng_w.loop_kept)
            warn("NO LOOP YET: REC", 0);
        else {
            engine_loop(LOOP_PLAY);
            say(eng_loop_state == LS_PLAY ? "LOOP HELD: YOU LEAD" : "THE LOOP PLAYS", 0);
        }
        break;
    case B_SAVE:
        if (ui.btn >> B_FX & 1u) {                /* holding FX: the throws stay out */
            ui.thr_kept = 1;
            ui.used |= 1u << B_FX;
            say("THROWS KEPT", 0);
        } else if (ui.view == V_LIB && ui.lrow >= G_ENSEMBLES) {   /* LIBRARY on MINE: kept there */
            char n[24];
            lib_name(ui.lrow, n);
            if (mine_keep(ui.lrow - G_ENSEMBLES))
                warn("SAVE FAILED", 0);
            else {
                ui.mine = (uint8_t)(ui.lrow - G_ENSEMBLES + 1);
                say("KEPT IN ", n);
            }
        } else {
            save_now();
            say("SAVED", 0);
        }
        break;
    case B_OCTDN:
    case B_OCTUP: {
        char t[8];
        ui.oct = (int8_t)clamp_i(ui.oct + (i == B_OCTUP ? 1 : -1), -1, 1);
        engine_oct(ui.oct);
        put_sgn(t, ui.oct);
        say("OCTAVE ", t);
        break;
    }
    case B_SEL:
        if (ui.view == V_SYSTEM)
            row_set(ui.row, 1);
        else if (ui.view == V_EDIT)
            er_turn(ui.erow, 1);
        else if (ui.view == V_LIB)
            lib_open(ui.lrow);
        else if (ui.view == V_SHAPE)
            ab_swap();
        else if (ui.view == V_BENCH) {
            int on = !engine_load_testing();
            engine_load_test(on);
            say(on ? "LOAD TEST ON" : "LOAD TEST OFF", 0);
        }
        break;
    default: break;
    }
}

static void select_turn(int e)
{
    switch (ui.view) {
    case V_HOME: mode_turn(e); break;
    case V_SHAPE: ui.page = (uint8_t)wrap_i(ui.page + e, NPG); ann_say(PAGE[ui.page].name, 0, ANN_NOW); break;
    case V_FX: ui.fxrow = (uint8_t)wrap_i(ui.fxrow + e, NFXROWS); ann_say(FXROW[ui.fxrow].name, 0, ANN_NOW); break;
    case V_EDIT: ui.erow = (uint8_t)clamp_i(ui.erow + e, 0, NER - 1); ann_say(ER_N[ui.erow], 0, ANN_NOW); break;
    case V_LIB: {
        char n[24];
        ui.lrow = (uint8_t)clamp_i(ui.lrow + e, 0, NLIB - 1);
        lib_name(ui.lrow, n);
        ann_say(n, 0, ANN_NOW);
        break;
    }
    case V_SYSTEM: ui.row = (uint8_t)clamp_i(ui.row + e, 0, NROWS - 1); ann_say(ROW_N[ui.row], 0, ANN_NOW); break;
    default: break;
    }
}

static void say_knob(const knob_t *k, int gong)
{
    char t[40], v[20];
    knob_text(k, gong, v);
    put_s(put_s(put_s(t, k->name), " "), v);
    say(t, 0);
}
static void knob_turn(int k, int e)
{
    int l = layer();
    if (l == L_THROW) {
        float *v = &ui.thr[k];
        char t[32], x[16];
        *v = fm_clampf(*v + 2.0f * (float)e, k == 2 ? -127.0f : 0.0f, 127.0f);
        ui.used |= 1u << B_FX;
        ui.thr_kept = 0;
        thr_text(k, x);
        put_s(put_s(put_s(t, THR_N[k]), " "), x);
        say(t, 0);
        return;
    }
    if (l == L_BEND) {
        float *v = &ui.bend[ui.hold_w][k];
        char t[32], x[16];
        *v = fm_clampf(*v + 2.0f * (float)e, 0.0f, 127.0f);
        engine_bend(ui.hold_w, k, round_i(*v));
        bend_text(ui.hold_w, k, x);
        put_s(put_s(put_s(t, BEND_N[k]), " "), x);
        say(t, 0);
        return;
    }
    if (l != L_PAGE) {
        const page_t *pg = hold_page(l);
        if (pg && pg->k[k].name) {
            knob_turn_p(&pg->k[k], ui.hold_bk, e);
            ui.hold_turned = 1;
            say_knob(&pg->k[k], ui.hold_bk);
        }
        return;
    }
    if (ui.view == V_SYSTEM) {
        row_set(ui.row, e);
        return;
    }
    if (ui.view == V_EDIT) {
        er_turn(ui.erow, e);
        return;
    }
    {
        const knob_t *ks = page_knobs();
        if (ks && ks[k].name) {
            knob_turn_p(&ks[k], 0, e);
            say_knob(&ks[k], 0);
        }
    }
}
static void algo_turn(int e)
{
    int l = layer();
    char t[24];
    if (l == L_THROW) {
        ui.thr[TH_OMBAK] = fm_clampf(ui.thr[TH_OMBAK] + 4.0f * (float)e, -127.0f, 127.0f);
        ui.used |= 1u << B_FX;
        thr_text(TH_OMBAK, t);
        say("OMBAK ", t);
        return;
    }
    if (l != L_PAGE && l != L_BEND) {
        const page_t *pg = hold_page(l);
        if (pg && pg->algo.name) {
            knob_turn_p(&pg->algo, ui.hold_bk, e);
            ui.hold_turned = 1;
            say_knob(&pg->algo, ui.hold_bk);
        }
        return;
    }
    if (ui.view == V_SHAPE) {
        knob_turn_p(&PAGE[ui.page].algo, 0, e);
        say_knob(&PAGE[ui.page].algo, 0);
    } else if (ui.view == V_EDIT) {
        er_turn(ui.erow, e);
    } else {                                      /* the tempo, in steps of 2.8% (the slow end as fine as the fast) */
        eng_w.p[P_TEMPO] = (uint8_t)clamp_i(eng_w.p[P_TEMPO] + e, 0, 127);
        changed();
        put_tempo(t, tempo_bpm(eng_w.p[P_TEMPO]));
        say("TEMPO ", t);
    }
}

/* ================================================================ input === */
static void input(void)
{
    uint32_t b = plat_buttons(), k = plat_keys(), now = plat_ms();
    uint32_t down = b & ~ui.btn, up = ui.btn & ~b, kd = k & ~ui.keys, ku = ui.keys & ~k;
    int i, e;
    if (b || k || down || up)
        ui.touch_t = now;
    ui.btn = b;
    ui.keys = k;
    for (i = 0; i < NKEYS; i++) {
        int w = KEY_WHITE[i], bk = KEY_BLACK[i];
        if (kd >> i & 1u) {
            ui.kdown_t[i] = now;
            if (w >= 0) {
                engine_white(w, 1);
                ui.hold_w = (int8_t)w;
                ui.bend_back[w] = 0;
                {
                    int j;
                    for (j = 0; j < 4; j++)
                        ui.bend[w][j] = (float)BEND_AS[j];
                }
            } else if (bk >= 0) {
                ui.hold_bk = (int8_t)bk;
                ui.hold_turned = 0;
                if (bk < G_N)
                    engine_gong(bk);
                else if (bk == BK_FASTER || bk == BK_SLOWER) {
                    cue(bk == BK_FASTER ? 1 : -1);
                    ui.tempo_acc = 0.0f;
                } else if (bk == BK_ENDING) {
                    if (eng_running) {
                        engine_end();
                        say(eng_w.p[P_E_AFTER] ? "ENDING: THE GONG AFTER NEXT" : "ENDING: AT THE NEXT GONG", 0);
                    } else
                        warn("NOT PLAYING", 0);
                }
            }
        }
        if (ku >> i & 1u) {
            if (w >= 0) {
                engine_white(w, 0);
                if (now - ui.kdown_t[i] >= HOLD_MS)
                    ui.bend_back[w] = 1;          /* (its bends drift back) */
                if (ui.hold_w == w)
                    ui.hold_w = -1;
            } else if (bk >= 0) {
                if (bk >= BK_KOT && !(ui.hold_bk == bk && ui.hold_turned)) {   /* a part: in or out, unless shaped */
                    static const uint8_t BIT[3] = {PB_KOT, PB_REY, PB_ORN};
                    static const char *const NM[3] = {"KOTEKAN", "REYONG", "ORNAMENTS"};
                    eng_w.p[P_PARTS] ^= BIT[bk - BK_KOT];
                    changed();
                    say(NM[bk - BK_KOT], eng_w.p[P_PARTS] & BIT[bk - BK_KOT] ? " IN" : " OUT");
                }
                if (ui.hold_bk == bk)
                    ui.hold_bk = -1;
            }
        }
    }
    for (i = 0; i < NB; i++) {
        uint32_t bit = 1u << i;
        if (down & bit) {
            const uint32_t both = 1u << B_REC | 1u << B_PLAY;
            ui.press_t[i] = now;
            ui.helped &= ~bit;
            ui.used &= ~bit;
            if ((i == B_REC || i == B_PLAY) && (b & both) == both) {
                engine_loop(LOOP_CLEAR);
                ui.used |= both;
                say("LOOP CLEARED", 0);
            }
            if (i == B_FX)
                ui.thr_back = 0;
        } else if ((b & bit) && !(ui.helped & bit) && !(ui.used & bit) && i != B_PLAY && i != B_FX &&
                   now - ui.press_t[i] >= HELP_MS) {
            ui.helped |= bit;
            if (HELP[i])
                say(HELP[i], 0);
        }
        if (up & bit) {
            if (i == B_PLAY && !(ui.used & bit) && now - ui.press_t[i] >= HELP_MS) {
                engine_loop(LOOP_CLEAR);          /* PLAY held: the loop gone */
                say("LOOP CLEARED", 0);
            } else if (!(ui.helped & bit) && !(ui.used & bit)) {
                button(i);
            }
            if (i == B_FX && (ui.used & bit) && !ui.thr_kept)
                ui.thr_back = 1;                  /* FX let go: the throws drift back */
            ui.used &= ~bit;
        }
    }
    if ((e = plat_enc(EN_SELECT)) != 0) {
        ui.touch_t = now;
        select_turn(e);
    }
    if ((e = plat_enc(EN_ALGO)) != 0) {
        ui.touch_t = now;
        algo_turn(e);
    }
    if ((e = plat_enc(EN_PRESET)) != 0) {
        ui.touch_t = now;
        if (ui.view == V_LIB)
            select_turn(e);
        else
            gamelan_go((eng_pend_ens >= 0 ? eng_pend_ens : eng_w.p[P_ENS]) + (e > 0 ? 1 : -1));
    }
    for (i = EN_K1; i <= EN_K4; i++)
        if ((e = plat_enc(i)) != 0) {
            ui.touch_t = now;
            knob_turn(i - EN_K1, e);
        }
    if (ui.view != V_HOME && ui.view != V_BENCH && now - ui.touch_t >= IDLE_HOME_MS)
        set_view(V_HOME);
}

/* a cue held: the tempo keeps moving, a STEP every two seconds */
static void cue_tick(float dt)
{
    int l = layer();
    if (l == L_CUE && !ui.hold_turned) {
        float step = 0.05f + 0.45f * (float)eng_w.p[P_C_STEP] / 127.0f;
        ui.tempo_acc += (ui.hold_bk == BK_FASTER ? 1.0f : -1.0f) * 127.0f / 5.0f * fm_log2f(1.0f + step) * dt * 0.5f;
        while (ui.tempo_acc >= 1.0f || ui.tempo_acc <= -1.0f) {
            int d = ui.tempo_acc > 0.0f ? 1 : -1;
            eng_w.p[P_TEMPO] = (uint8_t)clamp_i(eng_w.p[P_TEMPO] + d, 0, 127);
            ui.tempo_acc -= (float)d;
            changed();
        }
    }
}

/* ================================================================== drawing === */
#define HDR_H 20
#define MAIN_Y 20
#define MAIN_H 138
#define KNOB_Y 158
#define KNOB_H 82

static uint32_t blit_hash[3];
static uint32_t cv_hash(void)
{
    uint32_t i, h = 2166136261u, n = cv_w * cv_h;
    for (i = 0; i < n; i++)
        h = (h ^ cv_px[i]) * 16777619u;
    return h ^ (cv_w << 16) ^ cv_h;
}
static void cv_commit(int region, uint32_t x, uint32_t y)
{
    uint32_t h = cv_hash();
    if (h != blit_hash[region]) {
        blit_hash[region] = h;
        cv_blit(x, y);
    }
}
/* the main area sends only the rectangle that changed (Penumbra's: the panel shows no tearing line) */
#define MAIN_COLB (240 / 8)
static uint32_t main_rowh[MAIN_H], main_colh[MAIN_COLB];
static void cv_commit_rect(int region, uint32_t x, uint32_t y)
{
    uint32_t rowh[MAIN_H], colh[MAIN_COLB], r, c, r0 = MAIN_H, r1 = 0, c0 = MAIN_COLB, c1 = 0, w, h, k;
    for (c = 0; c < MAIN_COLB; c++)
        colh[c] = 2166136261u;
    for (r = 0; r < MAIN_H; r++) {
        const uint16_t *q = cv_px + r * 240u;
        uint32_t hr = 2166136261u;
        for (c = 0; c < MAIN_COLB; c++) {
            uint32_t hc = colh[c];
            for (k = 0; k < 8u; k++) {
                hr = (hr ^ q[k]) * 16777619u;
                hc = (hc ^ q[k]) * 16777619u;
            }
            colh[c] = hc;
            q += 8;
        }
        rowh[r] = hr;
    }
    if (!blit_hash[region]) {
        r0 = c0 = 0;
        r1 = MAIN_H - 1;
        c1 = MAIN_COLB - 1;
    } else {
        for (r = 0; r < MAIN_H; r++)
            if (rowh[r] != main_rowh[r]) {
                if (r0 == MAIN_H)
                    r0 = r;
                r1 = r;
            }
        for (c = 0; c < MAIN_COLB; c++)
            if (colh[c] != main_colh[c]) {
                if (c0 == MAIN_COLB)
                    c0 = c;
                c1 = c;
            }
        if (r0 == MAIN_H || c0 == MAIN_COLB)
            return;
    }
    blit_hash[region] = 1;
    for (r = 0; r < MAIN_H; r++)
        main_rowh[r] = rowh[r];
    for (c = 0; c < MAIN_COLB; c++)
        main_colh[c] = colh[c];
    w = (c1 - c0 + 1u) * 8u;
    h = r1 - r0 + 1u;
    for (r = 0; r < h; r++) {
        const uint16_t *src = cv_px + (r0 + r) * 240u + c0 * 8u;
        uint16_t *dst = cv_px + r * w;
        for (k = 0; k < w; k++)
            dst[k] = src[k];
    }
    lcd_blit(x + c0 * 8u, y + r0, w, h, cv_px);
}

static uint16_t blend(uint16_t a, uint16_t b, int t)
{
    int r = ((a >> 11) * (16 - t) + (b >> 11) * t) >> 4, g = (((a >> 5) & 63) * (16 - t) + ((b >> 5) & 63) * t) >> 4,
        bl = ((a & 31) * (16 - t) + (b & 31) * t) >> 4;
    return (uint16_t)((r << 11) | (g << 5) | bl);
}
static uint16_t dim(uint16_t c, int t) { return blend(C_BG, c, t); }
#define C_RED RGB(255, 90, 80)

typedef struct { const felucca_font_t *f; int8_t top, bot; } capm_t;
static capm_t capm[6];
static const capm_t *caps(const felucca_font_t *f)
{
    int i;
    for (i = 0; i < 6 && capm[i].f; i++)
        if (capm[i].f == f)
            return &capm[i];
    if (i == 6)
        i = 5;
    {
        uint32_t gi = ((uint32_t)'H' <= f->last ? 'H' : '0') - f->first, w = f->bw[gi], bpr = (w + 1u) / 2u, x, y;
        const uint8_t *gd = f->data + f->off[gi];
        int top = -1, bot = 0;
        for (y = 0; y < f->h; y++)
            for (x = 0; x < w; x++) {
                uint32_t a = gd[y * bpr + x / 2u];
                a = (x & 1u) ? (a & 15u) : (a >> 4);
                if (a >= 8u) {
                    if (top < 0)
                        top = (int)y;
                    bot = (int)y;
                }
            }
        capm[i].f = f;
        capm[i].top = (int8_t)(top < 0 ? 0 : top);
        capm[i].bot = (int8_t)bot;
    }
    return &capm[i];
}
static int32_t vc(const felucca_font_t *f, int32_t y0, int32_t h)
{
    const capm_t *m = caps(f);
    return y0 + (h - (m->bot - m->top + 1) + 1) / 2 - m->top;
}
static void text_c(int32_t cx, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    cv_text(cx - text_w(f, s) / 2, y, f, s, c);
}
static void text_r(int32_t rx, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    cv_text(rx - text_w(f, s), y, f, s, c);
}
static void px_blend(int32_t x, int32_t y, uint16_t c, uint32_t a)
{
    uint16_t *q;
    uint32_t bg;
    y += cv_oy;
    if ((uint32_t)x >= cv_w || y < cv_y0 || y >= cv_y1 || !a)
        return;
    q = &cv_px[(uint32_t)y * cv_w + (uint32_t)x];
    if (a >= 16u) {
        *q = swap16(c);
        return;
    }
    bg = swap16(*q);
    *q = swap16((uint32_t)(((((c >> 11) * a + (bg >> 11) * (16u - a)) >> 4) << 11) |
                           (((((c >> 5) & 63u) * a + ((bg >> 5) & 63u) * (16u - a)) >> 4) << 5) |
                           (((c & 31u) * a + (bg & 31u) * (16u - a)) >> 4)));
}
static void disc(float cx, float cy, float r, uint16_t c)
{
    int32_t x0 = (int32_t)fm_floorf(cx - r - 1.0f), x1 = (int32_t)(cx + r + 1.0f);
    int32_t y0 = (int32_t)fm_floorf(cy - r - 1.0f), y1 = (int32_t)(cy + r + 1.0f), x, y;
    for (y = y0; y <= y1; y++)
        for (x = x0; x <= x1; x++) {
            float dx = (float)x + 0.5f - cx, dy = (float)y + 0.5f - cy, d = fm_sqrtf(dx * dx + dy * dy);
            float a = fm_clampf(r - d + 0.5f, 0.0f, 1.0f);
            px_blend(x, y, c, (uint32_t)(a * 16.0f + 0.5f));
        }
}
static void ring(float cx, float cy, float r, float w, uint16_t c)   /* a circle's line, w wide */
{
    int32_t x0 = (int32_t)fm_floorf(cx - r - w - 1.0f), x1 = (int32_t)(cx + r + w + 1.0f);
    int32_t y0 = (int32_t)fm_floorf(cy - r - w - 1.0f), y1 = (int32_t)(cy + r + w + 1.0f), x, y;
    for (y = y0; y <= y1; y++)
        for (x = x0; x <= x1; x++) {
            float dx = (float)x + 0.5f - cx, dy = (float)y + 0.5f - cy, d = fm_fabsf(fm_sqrtf(dx * dx + dy * dy) - r);
            float a = fm_clampf(w * 0.5f - d + 0.5f, 0.0f, 1.0f);
            px_blend(x, y, c, (uint32_t)(a * 16.0f + 0.5f));
        }
}
/* a 4-bit bitmap (the logo's, the Ongkara's), its coverage x k/16 in colour c */
static void bitmap(const uint8_t *px, int w, int h, int x0, int y0, uint16_t c, uint32_t k16)
{
    int x, y;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            uint32_t a = px[y * ((w + 1) / 2) + x / 2];
            a = (x & 1) ? (a & 15u) : (a >> 4);
            a = (a + (a == 15u)) * k16 / 16u;
            if (a)
                px_blend(x0 + x, y0 + y, c, a > 16u ? 16u : a);
        }
}

/* ------------------------------------------------------------------ header --- */
static void draw_header(void)
{
    char b[24];
    int l = layer();
    cv_begin(240, HDR_H, C_BG);
    cv_rect(0, HDR_H - 1, 240, 1, C_LINE);
    if (ui.msg_until && (int32_t)(ui.msg_until - plat_ms()) > 0) {
        cv_rect(0, 0, 240, HDR_H - 1, dim(C_AMB, 4));
        cv_rect(0, HDR_H - 1, 240, 1, C_AMB);
        bitmap(LOGO_T_PX, LOGO_T_W, LOGO_T_H, 3, 1, C_WHITE, 16u);
        text_c(122, vc(&FONT_B, 0, HDR_H - 1), &FONT_B, ui.msg, C_WHITE);
        cv_commit(0, 0, 0);
        return;
    }
    ui.msg_until = 0;
    bitmap(LOGO_T_PX, LOGO_T_W, LOGO_T_H, 3, 1, C_HI, 16u);
    {
        const char *t = ui.view == V_HOME ? FW_NAME : ui.view == V_SHAPE ? PAGE[ui.page].name :
                        ui.view == V_FX ? FXROW[ui.fxrow].name : VIEW_N[ui.view];
        if (l == L_GONG)
            t = GONG_N[ui.hold_bk];
        else if (l == L_CUE)
            t = ui.hold_bk == BK_FASTER ? "FASTER" : "SLOWER";
        else if (l != L_PAGE && l != L_BEND && l != L_THROW)
            t = hold_page(l)->name;
        else if (l == L_THROW)
            t = "THROWS";
        else if (l == L_BEND) {
            put_s(put_s(b, "YOUR "), g_name(&usc, ui.hold_w + ui.oct * usc.n));
            t = b;
        }
        cv_text(8 + LOGO_T_W, vc(&FONT_B, 1, 17), &FONT_B, t, C_HI);
    }
    if (ui.dirty)
        cv_rect(150, 8, 3, 3, C_AMB);             /* not saved yet */
    {
        char tt[20];
        put_tempo(tt, eng_running ? eng_bpm_now : tempo_bpm(eng_w.p[P_TEMPO]));
        text_r(232, vc(&FONT_XS, 1, 17), &FONT_XS, tt, eng_running ? C_HI : C_GRAY);
    }
    cv_commit(0, 0, 0);
}

/* -------------------------------------------------------- HOME: the Ongkara --- */
#define HC_X 120
#define HC_Y 62
#define HC_R 44.0f
#define HC_RIM 53.0f
/* a place on the rim: the gong at the top, the cycle clockwise */
static float rim_x(float pos, int len) { return (float)HC_X + HC_RIM * fm_sin_turns(pos / (float)len); }
static float rim_y(float pos, int len) { return (float)HC_Y - HC_RIM * fm_sin_turns(pos / (float)len + 0.25f); }
static float gong_glow(void)                       /* the stroke's flare, then the hum swelling and fading */
{
    uint32_t d = eng_frames - eng_gong_at;
    float t;
    if (!eng_gong_at || d > 44100u * 60u)
        return 0.0f;
    t = (float)d / 44100.0f;
    return 0.7f * fm_expf(-t / 0.25f) + 0.9f * (1.0f - fm_expf(-t / 0.45f)) * fm_expf(-t / 7.0f);
}
static void draw_home(void)
{
    const int len = engine_cycle_len();
    const float tide = eng_tide;
    char b[48];
    int p, i;
    float glow, hot;
    /* the tide: a halo that widens and narrows */
    {
        float halo = HC_R + 12.0f + tide * 5.0f;
        disc((float)HC_X, (float)HC_Y, halo, dim(C_AMB, 2));
        disc((float)HC_X, (float)HC_Y, HC_R + 6.0f + tide * 3.0f, dim(C_AMB, 3));
    }
    if (!cfg.home) {                              /* the Ongkara: its light the music's swell, never the tempo */
        glow = fm_clampf(0.3f + 0.2f * tide + 1.1f * eng_swell + 0.8f * gong_glow(), 0.0f, 1.8f);
        hot = fm_minf(glow, 1.0f);
        disc((float)HC_X, (float)HC_Y, HC_R, RGB(20, 15, 10));
        ring((float)HC_X, (float)HC_Y, HC_R, 1.2f, RGB(58, 44, 29));
        bitmap(ONG_GLOW_PX, ONG_GLOW_W, ONG_GLOW_H, HC_X - ONG_GLOW_W / 2, HC_Y - ONG_GLOW_H / 2, RGB(255, 170, 80),
               (uint32_t)(4.0f + 10.0f * hot));
        bitmap(ONG_PX, ONG_W, ONG_H, HC_X - ONG_W / 2, HC_Y - ONG_H / 2,
               blend(RGB(232, 190, 120), RGB(255, 246, 228), (int)(fm_clampf(glow - 0.6f, 0.0f, 1.0f) * 16.0f)), 16u);
    } else {                                      /* the gong: its boss flares and swells with the hum */
        glow = fm_clampf(0.25f + gong_glow(), 0.0f, 1.3f);
        disc((float)HC_X, (float)HC_Y, HC_R, RGB(52, 37, 22));
        ring((float)HC_X, (float)HC_Y, HC_R, 2.5f, RGB(128, 96, 58));
        ring((float)HC_X, (float)HC_Y, HC_R * 0.6f, 1.2f, RGB(61, 43, 26));
        disc((float)HC_X, (float)HC_Y, 20.0f, blend(RGB(90, 64, 36), RGB(255, 222, 150), (int)(fm_minf(glow, 1.0f) * 16.0f)));
        disc((float)HC_X - 4.0f, (float)HC_Y - 5.0f, 7.0f, blend(RGB(110, 80, 46), RGB(255, 250, 230), (int)(fm_minf(glow, 1.0f) * 16.0f)));
    }
    /* the cycle round the rim: its places, the core notes as beads, the travelling light */
    for (p = 0; p < len; p++) {
        static const uint16_t COL[4] = {RGB(214, 165, 78), RGB(116, 187, 164), RGB(210, 124, 108), RGB(138, 124, 108)};
        static const float R[4] = {4.0f, 3.0f, 2.6f, 1.2f};
        int pl = g_place(eng_w.p[P_FORM], p, len);
        disc(rim_x((float)p, len), rim_y((float)p, len), R[pl], eng_w.p[P_AUTO] ? COL[pl] : dim(COL[pl], 7));
    }
    {
        float cyc = 44100.0f * 60.0f / fm_maxf(eng_bpm_now, 2.0f) * (float)len;
        for (i = 0; i < BEADS; i++) {
            float age = (float)(eng_frames - eng_bead_at[i]);
            if (!eng_bead_at[i] || age < 0.0f || age > cyc || eng_bead_pos[i] >= len)
                continue;
            disc(rim_x((float)eng_bead_pos[i], len), rim_y((float)eng_bead_pos[i], len), 1.5f + 1.5f * (1.0f - age / cyc),
                 blend(C_BG, RGB(246, 227, 184), (int)(16.0f * (1.0f - age / cyc))));
        }
    }
    {
        float pos = (float)eng_beat_pos;
        if (eng_running && eng_beat_len)
            pos += fm_clampf((float)(eng_frames - eng_beat_at) / (float)eng_beat_len, 0.0f, 1.0f);
        disc(rim_x(pos, len), rim_y(pos, len), 4.5f, dim(RGB(255, 236, 190), 8));
        disc(rim_x(pos, len), rim_y(pos, len), 2.5f, RGB(255, 240, 205));
    }
    /* under it: the mode and the tuning, what arrives with the gong */
    {
        int t = eng_w.p[P_TUNING] % G_TUNINGS;
        char *q = put_s(b, g_mode_name(t, eng_w.p[P_MODE]));
        if (eng_pend_ens >= 0)
            q = put_s(put_s(q, " > "), G_ENSEMBLE[eng_pend_ens % G_ENSEMBLES].name);
        else if (eng_pend_mode >= 0)
            q = put_s(put_s(q, " > "), g_mode_name(t, eng_pend_mode));
        else if (ui.mine)
            q = put_i(put_s(q, ", MINE "), ui.mine);
        else
            q = put_s(put_s(q, ", "), G_ENSEMBLE[eng_w.p[P_ENS] % G_ENSEMBLES].name);
        text_c(120, 125, &FONT_XS, b, eng_pend_ens >= 0 || eng_pend_mode >= 0 ? C_AMB : C_GRAY);
    }
}

/* ----------------------------------------------------------------- lists --- */
#define ROW_H 22
#define ROWS_SHOWN 6
static void list_row(int i, int sel, const char *name, const char *val, uint16_t vcol)
{
    int y = 2 + i * ROW_H;
    if (sel)
        cv_rect(0, y, 240, ROW_H - 2, dim(C_AMB, 3));
    cv_text(10, vc(&FONT_S, y, ROW_H - 2), &FONT_S, name, sel ? C_HI : C_GRAY);
    text_r(230, vc(&FONT_XS, y, ROW_H - 2), &FONT_XS, val, sel ? C_AMB : vcol);
}
static int list_top(int sel, int n, int shown)
{
    int top = sel < shown ? 0 : sel - shown + 1;
    return top + shown > n ? (n > shown ? n - shown : 0) : top;
}
static void draw_list_edit(void)
{
    int top = list_top(ui.erow, NER, ROWS_SHOWN), r;
    for (r = top; r < NER && r < top + ROWS_SHOWN; r++) {
        char v[32];
        er_text(r, v);
        list_row(r - top, r == ui.erow, ER_N[r], v, C_HI);
    }
}
static void draw_list_lib(void)
{
    int top = list_top(ui.lrow, NLIB, ROWS_SHOWN), r;
    for (r = top; r < NLIB && r < top + ROWS_SHOWN; r++) {
        char n[24];
        lib_name(r, n);
        if (r < G_ENSEMBLES)
            list_row(r - top, r == ui.lrow, n,
                     r == eng_w.p[P_ENS] && !ui.mine ? "PLAYING" : r == eng_pend_ens ? "AT THE GONG" : "", C_AMB);
        else
            list_row(r - top, r == ui.lrow, n,
                     ui.mine == r - G_ENSEMBLES + 1 ? "PLAYING" : mine_kept(r - G_ENSEMBLES) ? "KEPT" : "EMPTY: SAVE",
                     C_AMB);
    }
}
static void draw_list_sys(void)
{
    int top = list_top(ui.row, NROWS, ROWS_SHOWN), r;
    for (r = top; r < NROWS && r < top + ROWS_SHOWN; r++) {
        char v[24];
        row_text(r, v);
        list_row(r - top, r == ui.row, ROW_N[r], v, C_HI);
    }
}

/* SHAPE: the page's four knobs as meters (an as-made one from its middle), its ALGO along the bottom; TIDES the
 * tide now; PAN the field */
static void draw_shape(void)
{
    const page_t *pg = &PAGE[ui.page % NPG];
    int k;
    for (k = 0; k < NPG; k++)
        disc(120.0f + ((float)k - 3.5f) * 10.0f, 6.0f, 2.2f, k == ui.page ? C_AMB : C_LINE);
    if (ui.page >= 6) {                           /* PAN, PAN 2: each part's lane, where its sway has taken it */
        for (k = 0; k < NPARTS; k++) {
            int y = 26 + k * 22, at = 80 + (eng_w.p[P_PAN + k] * 148) / 127, now = 80 + (int)((eng_pan_now[k] + 1.0f) * 74.0f);
            int on = ui.page == 6 || eng_w.p[P_SWAY + k];
            cv_text(6, y - 5, &FONT_XS, pg->k[k].name, on ? C_GRAY : C_DIM);
            cv_rect(80, y, 149, 1, C_LINE);
            cv_rect(154, y - 4, 1, 9, C_DIM);
            if (now != at)
                cv_rect(at, y - 3, 1, 7, C_DIM);
            cv_rect(now - 2, y - 3, 5, 7, on ? C_AMB : C_DIM);
        }
        return;
    }
    for (k = 0; k < 4; k++) {
        const knob_t *kn = &pg->k[k];
        int cx = 30 + k * 60, top = 20, h = 70, x, fill;
        if (!kn->name)
            continue;
        x = eng_w.p[kn->p];
        if (kd_max(kn->kind) < 127)
            x = x * 127 / kd_max(kn->kind);
        fill = x * h / 127;
        cv_rect(cx - 3, top, 6, h, C_LINE);
        if (kn->kind == KD_ASMADE) {
            int mid = top + h - 64 * h / 127;
            cv_rect(cx - 6, mid, 12, 1, C_DIM);
            if (top + h - fill < mid)
                cv_rect(cx - 3, top + h - fill, 6, mid - (top + h - fill), C_AMB);
            else
                cv_rect(cx - 3, mid, 6, top + h - fill - mid + 1, C_AMB);
        } else {
            cv_rect(cx - 3, top + h - fill, 6, fill, C_AMB);
        }
        text_c(cx, 98, &FONT_XS, kn->name, C_GRAY);
    }
    if (ui.page == 5) {                           /* TIDES: where the tide is now */
        int x = 90 + (int)((eng_tide + 1.0f) * 55.0f);
        cv_text(6, 116, &FONT_XS, "THE TIDE", C_GRAY);
        cv_rect(90, 121, 110, 2, C_LINE);
        cv_rect(145, 117, 1, 10, C_DIM);
        cv_rect(x - 2, 119, 5, 6, C_AMB);
    }
}
static void draw_fx(void)
{
    int row, k;
    for (row = 0; row < NFXROWS; row++) {
        int y = 2 + row * 40, sel = row == ui.fxrow;
        if (sel)
            cv_rect(0, y, 240, 36, dim(C_AMB, 3));
        cv_text(8, y + 3, &FONT_XS, FXROW[row].name, sel ? C_HI : C_GRAY);
        for (k = 0; k < 4; k++) {
            char b[16];
            knob_text(&FXROW[row].k[k], 0, b);
            text_c(30 + k * 60, y + 18, &FONT_XS, b, sel ? C_AMB : C_HI);
        }
    }
}

static bench_t bench;
static uint32_t bench_serial;
static void draw_bench(void)
{
    char b[32];
    int k;
    if (engine_load_testing())
        cv_text(6, 1, &FONT_XS, "LOAD TEST", C_AMB);
    else {
        put_s(put_i(put_s(put_i(put_s(put_i(put_s(b, bench.two_cores ? "2 CORES " : "1 CORE "), bench.voices), " VOICES "),
                                bench.ksines[0]), "/"), bench.ksines[1]), "K SIN");
        cv_text(6, 1, &FONT_XS, b, C_GRAY);
    }
    text_r(146, 1, &FONT_XS, "AVG %", C_DIM);
    text_r(210, 1, &FONT_XS, "PEAK %", C_DIM);
    cv_rect(6, 13 + ST_CORE1 * 12, 228, 1, C_LINE);
    for (k = 0; k < ST_N; k++) {
        int total = k >= ST_CORE1, y = 14 + k * 12 + (total ? 3 : 0);
        uint16_t c = total ? C_HI : C_GRAY;
        cv_text(10, y, &FONT_XS, engine_stage_name(k), c);
        if (!bench_serial || (k == ST_CORE2 && !bench.two_cores)) {
            text_r(146, y, &FONT_XS, "-", C_DIM);
            text_r(210, y, &FONT_XS, "-", C_DIM);
            continue;
        }
        put_d1(b, bench.avg[k]);
        text_r(146, y, &FONT_XS, b, c);
        put_d1(b, bench.peak[k]);
        text_r(210, y, &FONT_XS, b, bench.peak[k] >= 900 ? C_RED : bench.peak[k] >= 700 ? C_AMB : c);
    }
}

static void draw_main(void)
{
    cv_begin(240, MAIN_H, C_BG);
    switch (ui.view) {
    case V_HOME: draw_home(); break;
    case V_SHAPE: draw_shape(); break;
    case V_EDIT: draw_list_edit(); break;
    case V_FX: draw_fx(); break;
    case V_LIB: draw_list_lib(); break;
    case V_SYSTEM: draw_list_sys(); break;
    default: draw_bench(); break;
    }
    cv_commit_rect(1, 0, MAIN_Y);
}

/* ------------------------------------------------------------ knob strip --- */
static void cell(int i, const char *name, const char *val, uint16_t c)
{
    int cx = i * 60 + 30;
    if (i)
        cv_rect(i * 60, 24, 1, 28, C_LINE);
    text_c(cx, 20, &FONT_XS, name, C_GRAY);
    text_c(cx, 38, &FONT_XS, val, c);
}
static void hint(const char *l, const char *r)
{
    cv_text(6, 64, &FONT_XS, l, C_DIM);
    if (r)
        text_r(234, 64, &FONT_XS, r, C_DIM);
}
static void cells(const knob_t *ks, int gong, uint16_t c)
{
    int k;
    for (k = 0; k < 4; k++) {
        char v[20];
        if (!ks[k].name)
            continue;
        knob_text(&ks[k], gong, v);
        cell(k, ks[k].name, v, c);
    }
}
static void draw_knobs(void)
{
    char a[40], b[24];
    int l = layer(), k;
    cv_begin(240, KNOB_H, C_BG);
    cv_rect(0, 0, 240, 1, C_LINE);
    if (l == L_THROW) {
        for (k = 0; k < 4; k++) {
            thr_text(k, a);
            cell(k, THR_N[k], a, C_AMB);
        }
        thr_text(TH_OMBAK, b);
        put_s(put_s(a, "ALGO: OMBAK "), b);
        hint(a, "SAVE: KEEP  LET GO: BACK");
    } else if (l == L_BEND) {
        for (k = 0; k < 4; k++) {
            bend_text(ui.hold_w, k, a);
            cell(k, BEND_N[k], a, C_AMB);
        }
        hint("ON THIS NOTE", "LET GO: BACK");
    } else if (l != L_PAGE) {
        const page_t *pg = hold_page(l);
        cells(pg->k, ui.hold_bk, C_AMB);
        if (pg->algo.name) {
            knob_text(&pg->algo, ui.hold_bk, b);
            put_s(put_s(put_s(put_s(a, "ALGO: "), pg->algo.name), " "), b);
        } else
            put_s(a, "");
        hint(a, l == L_PART ? "TURNED: IT STAYS IN/OUT" : "IT STAYS");
    } else {
        switch (ui.view) {
        case V_HOME: {
            static const char *const LP[5] = {"", "  LOOP WAITS", "  LOOP REC", "  LOOP", "  LOOP HELD"};
            cells(MACRO, 0, C_HI);
            put_s(put_i(put_s(put_s(a, eng_running ? (eng_ending ? "ENDING" : "PLAYING") : "STOPPED"),
                              eng_w.p[P_AUTO] ? "  CYCLE " : "  BY HAND "), engine_cycle_len()), LP[eng_loop_state % 5]);
            if (ui.sleep)                         /* SLEEP: the minutes left */
                put_s(put_i(put_s(b, "SLEEP "), (sleep_left_ms() + 59999) / 60000), "M");
            else
                put_sgn(put_s(b, "OCT "), ui.oct);
            hint(a, ui.oct || ui.sleep ? b : 0);
            break;
        }
        case V_SHAPE:
            cells(PAGE[ui.page].k, 0, C_HI);
            knob_text(&PAGE[ui.page].algo, 0, b);
            put_s(put_s(put_s(put_s(a, "ALGO: "), PAGE[ui.page].algo.name), " "), b);
            hint("ENV: NEXT", a);
            break;
        case V_FX:
            cells(FXROW[ui.fxrow].k, 0, C_HI);
            hint("SELECT: THE ROW", FXROW[ui.fxrow].name);
            break;
        case V_EDIT:
            hint("SELECT: ROW", "ALGO, KNOB, SEL: CHANGE");
            break;
        case V_LIB:
            cv_text(6, 20, &FONT_XS, "PLAYING", C_GRAY);
            if (ui.mine) {
                char n[16];
                put_i(put_s(n, "MINE "), ui.mine);
                cv_text(6, 34, &FONT_S, n, C_AMB);
            } else
                cv_text(6, 34, &FONT_S, G_ENSEMBLE[eng_w.p[P_ENS] % G_ENSEMBLES].name, C_AMB);
            if (ui.lrow >= G_ENSEMBLES)
                hint("SEL: OPEN", "SAVE: KEEP THE WORK HERE");
            else
                hint("SEL: OPEN (AT THE GONG)", "PRESETS: THE NEXT");
            break;
        case V_SYSTEM:
            text_c(120, 30, &FONT_XS, "SELECT: ROW    KNOB OR SEL: CHANGE", C_DIM);
            break;
        default: {
            static uint32_t blk_peak, blk_t;
            char c[16], d[16];
            if (plat_ms() - blk_t >= 1000u) {
                blk_t = plat_ms();
                blk_peak = plat_cpu_peak_pct();
            }
            put_i(put_s(put_i(a, (int)plat_cpu_pct()), "/"), (int)blk_peak);
            cell(0, "LOAD/PEAK", a, blk_peak >= 90u ? C_RED : C_HI);
            put_i(b, (int)plat_xruns());
            cell(1, "XRUNS", b, plat_xruns() ? C_RED : C_HI);
            put_i(put_s(put_i(c, (int)eng_econ_count), "/"), (int)eng_guard_count);
            cell(2, "ECON/GUARD", c, engine_guard_active() ? C_RED : engine_economy() ? C_AMB : C_HI);
            put_i(d, (int)eng_out_clips);
            cell(3, "CLIPS", d, eng_out_clips ? C_RED : C_HI);
            hint("SEL: LOAD TEST", 0);
            break;
        }
        }
    }
    cv_commit(2, 0, KNOB_Y);
}

/* ------------------------------------------------------------------- lights --- */
static void leds(void)
{
    static const uint8_t VIEW_BTN[NVIEWS] = {B_HOME, B_ENV, B_EDIT, B_FX, B_LFO, B_GLO, B_GLO};
    uint32_t btn = 1u << VIEW_BTN[ui.view], keys = 0, kdim = 0;
    int i;
    if (eng_running)
        btn |= 1u << B_SEQ;
    if (eng_w.p[P_AUTO])
        btn |= 1u << B_ARP;
    if (eng_loop_state == LS_PLAY)
        btn |= 1u << B_PLAY;
    if ((eng_loop_state == LS_ARM || eng_loop_state == LS_REC) && ((ui.frame >> 4) & 1u))
        btn |= 1u << B_REC;
    if (any_thrown())
        btn |= 1u << B_FX;
    for (i = 0; i < G_N; i++)                     /* the gongs, dim */
        kdim |= 1u << BLACK_KEY[i];
    for (i = BK_FASTER; i <= BK_ENDING; i++)
        kdim |= 1u << BLACK_KEY[i];
    {
        static const uint8_t BIT[3] = {PB_KOT, PB_REY, PB_ORN};
        for (i = 0; i < 3; i++)                   /* the parts: lit while in */
            if (eng_w.p[P_PARTS] & BIT[i])
                keys |= 1u << BLACK_KEY[BK_KOT + i];
            else
                kdim |= 1u << BLACK_KEY[BK_KOT + i];
    }
    keys |= ui.keys;
    plat_glow(1);
    plat_keys_dim(kdim);
    plat_play_red(0);
    plat_leds(btn, keys);
}

static void speak_where(void)                      /* WHERE: the screen, said whole */
{
    char t[ANN_MAX + 1], *q = t, v[32];
    int k;
    q = put_s(put_s(q, VIEW_N[ui.view]), ". ");
    switch (ui.view) {
    case V_HOME:
        q = put_s(q, eng_running ? "PLAYING. " : "STOPPED. ");
        put_tempo(v, eng_running ? eng_bpm_now : tempo_bpm(eng_w.p[P_TEMPO]));
        q = put_s(put_s(put_s(q, "TEMPO "), v), ". ");
        q = put_s(put_s(q, G_ENSEMBLE[eng_w.p[P_ENS] % G_ENSEMBLES].name), ", ");
        q = put_s(put_s(q, g_mode_name(eng_w.p[P_TUNING], eng_w.p[P_MODE])), ". ");
        for (k = 0; k < 4; k++) {
            knob_text(&MACRO[k], 0, v);
            q = put_s(put_s(put_s(put_s(q, MACRO[k].name), " "), v), ". ");
        }
        break;
    case V_SHAPE:
        q = put_s(put_s(q, PAGE[ui.page].name), ". ");
        for (k = 0; k < 4; k++)
            if (PAGE[ui.page].k[k].name) {
                knob_text(&PAGE[ui.page].k[k], 0, v);
                q = put_s(put_s(put_s(put_s(q, PAGE[ui.page].k[k].name), " "), v), ". ");
            }
        break;
    case V_EDIT:
        er_text(ui.erow, v);
        q = put_s(put_s(put_s(q, ER_N[ui.erow]), " "), v);
        break;
    case V_LIB:
        lib_name(ui.lrow, q);
        while (*q)
            q++;
        break;
    case V_SYSTEM:
        row_text(ui.row, v);
        q = put_s(put_s(put_s(q, ROW_N[ui.row]), " "), v);
        break;
    default: break;
    }
    ann_say(t, 0, ANN_NOW);
}

/* ===================================================================== frame === */
void ui_init(void)
{
    int i;
    for (i = 0; i < NKEYS; i++) {
        if (KEY_BLACK[i] >= 0)
            BLACK_KEY[KEY_BLACK[i]] = (uint8_t)i;
        if (KEY_WHITE[i] >= 0)
            WHITE_KEY[KEY_WHITE[i]] = (uint8_t)i;
    }
    ui.view = V_HOME;
    ui.hold_bk = ui.hold_w = -1;
    ui.dirty = 0;
    ui.touch_t = plat_ms();
    ui.last_pend_mode = ui.last_pend_ens = -1;
    for (i = 0; i < TH_N; i++)
        ui.thr_sent[i] = 0;
    for (i = 0; i < 3; i++)
        blit_hash[i] = 0;
    scale_now();
}

void ui_start(void)
{
    if (cfg.autoplay && !eng_running)
        engine_run(1);
}

void ui_frame(void)
{
    static uint32_t t0;
    uint32_t now = plat_ms();
    float dt = fm_minf((float)(now - t0) * 0.001f, 0.1f);
    t0 = now;
    input();
    cue_tick(dt);
    moves_tick(dt);
    if ((eng_pend_mode != ui.last_pend_mode && eng_pend_mode < 0) || (eng_pend_ens != ui.last_pend_ens && eng_pend_ens < 0))
        changed();                                /* (the gong brought a mode or a gamelan: it is saved) */
    ui.last_pend_mode = eng_pend_mode;
    ui.last_pend_ens = eng_pend_ens;
    if (eng_loop_state != ui.last_loop) {
        ui.last_loop = eng_loop_state;
        changed();
    }
    scale_now();
    autosave();
    if (ui.sleep) {                               /* SLEEP: the ending in its last minutes; then still */
        int left = sleep_left_ms();
        if (left <= 0) {
            if (eng_running)
                engine_run(0);
            ui.sleep = 0;
            say("SLEEP: STILL", 0);
        } else if (left <= (int)SLEEP_LAST_MS && !ui.sleep_step) {
            ui.sleep_step = 1;
            if (eng_running && !eng_ending)
                engine_end();
            say("SLEEP: THE ENDING", 0);
        }
    }
    {
        uint32_t s = engine_bench(&bench);
        if (s)
            bench_serial = s;
    }
    draw_header();
    draw_main();
    draw_knobs();
    leds();
    if (ann_where())
        speak_where();
    ann_tick(plat_ms());
    ui.frame++;
}

void ui_input_only(void) { input(); }
void ui_say(const char *a, const char *b) { say(a, b); }
