/* SPDX-License-Identifier: GPL-3.0-only */
/* Ombak's gamelan; see gamelan.h. The tunings are measured sets, as the Scala scale archive (Manuel Op de
 * Coul, huygens-fokker.org) transcribes them:
 *   gendèr wayang, Peliatan (slendro_pliat.scl) and Ubud (slendrod1.scl); low gendèr, Tabanan (McPhee,
 *   Music in Bali, 1966: slendro8.scl); angklung, Sangsit (slendro_ang.scl) and Mas (McPhee: slendro12.scl);
 *   Kyai Kanyut Mesem, Surakarta (Surjodiningrat, Sudarjana & Susanto 1972: slendro_ky1.scl, pelog_me1.scl);
 *   semar pegulingan saih pitu, Tampak Gangsul (McPhee: pelog12.scl) and Ksatria, Denpasar (pelog1.scl);
 *   gamelan gong, Peliatan (McPhee: selisir3.scl).
 * The Balinese modes and their tone names follow Wikipedia's Pelog article (baro: one common version). */
#include "gamelan.h"
#include "../dsp/fastmath.h"

static const char *const BALI[5] = {"DING", "DONG", "DENG", "DUNG", "DANG"};
/* the gendèr wayang's lowest key is dong, its tones sung dong, deng, dung, dang, ding (Gold, Gamelan Gender
 * Wayang of Bali: Form and Style, Wesleyan): the measured sets start from the instrument's lowest */
static const char *const GENDER[5] = {"DONG", "DENG", "DUNG", "DANG", "DING"};
static const char *const ANGKLUNG[4] = {"DONG", "DENG", "DUNG", "DANG"};
static const char *const JAVA_SL[5] = {"1", "2", "3", "5", "6"};
/* the selonding of Tenganan Pegeringsingan, McPhee's measurement (Music in Bali, 1966) as the tuning list relays
 * it: seven tones from about 257.5 Hz, 132 184 181 159 158 255 cents apart. Its players take five-tone modes from
 * the seven; Ombak plays all seven, numbered */
static const char *const SEVEN[7] = {"1", "2", "3", "4", "5", "6", "7"};

const g_tuning_t G_TUNING[G_TUNINGS] = {
    {"GENDER WAYANG, PELIATAN", FAM_SLENDRO, 5, 0, 305.5f, {0, 235.419f, 453.560f, 704.786f, 927.453f}, GENDER},
    {"GENDER WAYANG, UBUD", FAM_SLENDRO, 5, 0, 347.0f, {0, 193, 457, 687, 919}, GENDER},
    {"LOW GENDER, TABANAN", FAM_SLENDRO, 5, 0, 179.0f, {0, 292.926f, 507.690f, 762.150f, 1005.735f}, GENDER},
    {"ANGKLUNG, SANGSIT", FAM_SLENDRO, 5, 0, 294.0f, {0, 231.174f, 457.834f, 684.199f, 922.793f}, BALI},
    {"ANGKLUNG, MAS", FAM_SLENDRO, 4, 0, 410.0f, {0, 232.757f, 408.133f, 715.973f}, ANGKLUNG},
    {"KANYUT MESEM SLENDRO", FAM_SLENDRO, 5, 0, 291.0f, {0, 222.974f, 475.590f, 711.842f, 937.091f}, JAVA_SL},
    {"SAIH PITU, TAMPAK GANGSUL", FAM_PELOG, 7, 1, 310.0f, {0, 144.576f, 282.754f, 546.234f, 671.911f, 774.860f, 1023.790f}, 0},
    {"SAIH PITU, KSATRIA", FAM_PELOG, 7, 1, 312.5f, {0, 153, 315, 552, 706, 848, 1058}, 0},
    {"GONG, PELIATAN", FAM_PELOG, 5, 0, 280.0f, {0, 148.059f, 268.637f, 638.994f, 762.706f}, BALI},
    {"KANYUT MESEM PELOG", FAM_PELOG, 7, 2, 295.0f, {0, 124.521f, 271.058f, 522.809f, 688.207f, 787.819f, 954.515f}, 0},
    {"SELONDING, TENGANAN", FAM_PELOG, 7, 0, 257.5f, {0, 132, 316, 497, 656, 814, 1069}, SEVEN},
};

/* each mode: its five of the seven tones (1..7), in order from its first (ding) */
static const uint8_t BALI_MODES[4][5] = {{1, 2, 3, 5, 6}, {4, 5, 6, 1, 2}, {5, 6, 7, 2, 3}, {3, 4, 5, 7, 1}};
static const char *const BALI_MODE_N[4] = {"SELISIR", "TEMBUNG", "SUNAREN", "BARO"};
static const uint8_t JAVA_MODES[2][5] = {{1, 2, 3, 5, 6}, {2, 3, 5, 6, 7}};
static const char *const JAVA_MODE_N[2] = {"PATHET NEM", "PATHET BARANG"};
static const char *const DIGIT[8] = {"0", "1", "2", "3", "4", "5", "6", "7"};

int g_modes(int t) { return G_TUNING[t % G_TUNINGS].modes == 1 ? 4 : G_TUNING[t % G_TUNINGS].modes == 2 ? 2 : 0; }
const char *g_mode_name(int t, int m)
{
    const g_tuning_t *g = &G_TUNING[t % G_TUNINGS];
    if (g->modes == 1)
        return BALI_MODE_N[m & 3];
    if (g->modes == 2)
        return JAVA_MODE_N[m & 1];
    return g->fam == FAM_SLENDRO ? "SLENDRO" : "PELOG";
}

void g_scale(int t, int m, g_scale_t *s)
{
    const g_tuning_t *g = &G_TUNING[t % G_TUNINGS];
    float root = 0.0f, lo;
    int i;
    if (g->modes) {
        const uint8_t *tones = g->modes == 1 ? BALI_MODES[m & 3] : JAVA_MODES[m & 1];
        s->n = 5;
        root = g->cents[tones[0] - 1];
        for (i = 0; i < 5; i++) {
            float c = g->cents[tones[i] - 1] - root;
            s->ratio[i] = fm_exp2f((c < 0.0f ? c + 1200.0f : c) * (1.0f / 1200.0f));
            s->name[i] = g->modes == 1 ? BALI[i] : DIGIT[tones[i]];
        }
    } else {
        s->n = g->n;
        for (i = 0; i < g->n; i++) {
            s->ratio[i] = fm_exp2f(g->cents[i] * (1.0f / 1200.0f));
            s->name[i] = g->names[i];
        }
    }
    s->base = g->hz * fm_exp2f(root * (1.0f / 1200.0f));
    lo = s->n >= 5 ? 130.0f : 105.0f;            /* the first white key between lo and its octave */
    while (s->base >= lo * 2.0f)
        s->base *= 0.5f;
    while (s->base < lo)
        s->base *= 2.0f;
}

static int fdiv(int a, int n) { return a >= 0 ? a / n : -((-a + n - 1) / n); }
float g_pitch(const g_scale_t *s, int idx)
{
    int o = fdiv(idx, s->n), d = idx - o * s->n;
    return s->base * s->ratio[d] * fm_exp2f((float)o);
}
int g_fold(const g_scale_t *s, int idx, float lo)
{
    int i = idx - fdiv(idx, s->n) * s->n;
    while (g_pitch(s, i) < lo)
        i += s->n;
    while (g_pitch(s, i - s->n) >= lo)
        i -= s->n;
    return i;
}
int g_near(const g_scale_t *s, int idx, int ref)
{
    int d = idx - ref, c = ref + (d - fdiv(d, s->n) * s->n);
    if (2 * (c - ref) > s->n)
        c -= s->n;
    return c;
}
const char *g_name(const g_scale_t *s, int idx) { return s->name[idx - fdiv(idx, s->n) * s->n]; }

/* the five: gendèr wayang (Peliatan slendro, gendèr for the core and the kotekan, the gongs by hand: it has
 * none), semar pegulingan (saih pitu, gangsa kotekan, gendèr ornaments), angklung (four-tone slendro, with
 * reyong), STILL (Ombak's own, for meditation: a note every ten seconds, long rings, no kajar, a deep
 * space), gong gede (five-tone pelog, 32 beats, slow and breathing). Tempo and ombak on their params' scales:
 * tempo 98 = 44 a minute, 95 = 40, 101 = 48, 25 = 6, 87 = 32; ombak 109 = 6 Hz, 99 = 4.5, 112 = 6.5, 54 = 1.2,
 * 85 = 3 */
/* and two with the tradition's forms: GILAK (Bali's eight-beat processional and opening form, gong Peliatan,
 * nyog cag kotekan, 60 a minute) and LADRANG (Java's 32-beat form on Kanyut Mesem pelog: the core and the
 * gendèr's ornaments, no kotekan, 30 a minute, little ombak). Tempo 110 = 60, 84 = 30 */
/* (1.1: the kendang and ceng-ceng with the ensembles that have them: semar pegulingan, gong gede, gilak, gong
 * kebyar; the suling with semar pegulingan and STILL. GONG KEBYAR: the twentieth-century style, five-tone
 * pelog from Peliatan's gamelan, fast (76 a minute), brilliant ombak (7 Hz), norot gangsa, everything in;
 * tempo 118 = 76, ombak 116 = 7 Hz) */
/* (1.2: SELONDING, the sacred iron gamelan of the Bali Aga villages: everything on iron keys over a wooden
 * trough, the gongs' parts too, as the selonding's own large keys play them; Tenganan's tuning, all seven tones;
 * no kajar, no drums, no reyong, no suling; stately (48 a minute), a slow, small shimmer (0.6 Hz: ombak 30),
 * nyog cag interlocking for the nyong-nyong's part) */
const g_ensemble_t G_ENSEMBLE[G_ENSEMBLES] = {
    {"GENDER WAYANG", 0, 1, 0, 1, 1, 0x3, 109, 98, 32, 76, 0xFF, 1, 64, 0, KS_CIRCLE, 0},
    {"SEMAR PEGULINGAN", 6, 2, 1, 0, 0, 0x7B, 99, 95, 44, 64, 0xFF, 1, 64, 0, KS_CIRCLE, 0},
    {"ANGKLUNG", 4, 1, 1, 0, 0, 0x7, 112, 101, 25, 70, 0xFF, 1, 64, 0, KS_NOROT, 0},
    {"STILL", 0, 1, 1, 1, 1, 0x43, 54, 25, 51, 38, 89, 0, 108, 0, KS_CIRCLE, 0},
    {"GONG GEDE", 8, 3, 1, 0, 0, 0x37, 85, 87, 76, 38, 0xFF, 1, 64, 0, KS_CIRCLE, 0},
    {"GILAK", 8, 1, 1, 0, 0, 0x37, 99, 110, 20, 90, 0xFF, 1, 64, 1, KS_NYOGCAG, 0},
    {"LADRANG", 9, 3, 1, 0, 1, 0x9, 54, 84, 51, 50, 0xFF, 1, 76, 4, KS_CIRCLE, 0},
    {"GONG KEBYAR", 8, 2, 1, 0, 0, 0x3F, 116, 118, 40, 96, 0xFF, 1, 56, 0, KS_NOROT, 0},
    {"SELONDING", 10, 2, 1, 0, 0, 0xB, 30, 101, 51, 64, 0xFF, 0, 76, 0, KS_NYOGCAG, 1},
};

/* The forms. GILAK as the University of Guelph / Waterloo teaching gamelan sets it out (Music of the World,
 * openwa.pressbooks.pub: the female gong on 1, the male on 5, the kempur on 6 and 8, the kempli on the odd
 * beats); LANCARAN (TWTN TPTN TPTN TPTG), KETAWANG (pTpW pTpN pTpP pTpG) and LADRANG (pTpW pTpN pTpP pTpN
 * pTpP pTpN pTpP pTpG) as the Javanese gendhing structures (Wikipedia: Colotomy). Java counts to the gong
 * at the end: its beat 16 (32) is Ombak's place 0 */
const g_form_t G_FORM[G_FORMS] = {
    {"FREE", 0, {0, 0, 0, 0, 0}},
    {"GILAK", 8, {"x.......", "....x...", ".....x.x", "........", "x.x.x.x."}},
    {"LANCARAN", 16, {"x...............", "................", "......x...x...x.", "x...x...x...x...",
                      ".x.x.x.x.x.x.x.x"}},
    {"KETAWANG", 16, {"x...............", "................", "............x...", "x.......x.......",
                      ".oxo.oxo.oxo.oxo"}},
    {"LADRANG", 32, {"x...............................", "................................",
                     "............x.......x.......x...", "x.......x.......x.......x.......",
                     ".oxo.oxo.oxo.oxo.oxo.oxo.oxo.oxo"}},
};
const char *const G_STYLE[G_STYLES] = {"CIRCLING", "NOROT", "NYOG CAG", "EMPAT"};

int g_strike(int form, int g, int pos, int len)
{
    const g_form_t *f = &G_FORM[(unsigned)form % G_FORMS];
    if (f->len) {
        char c = f->line[g % 5][(unsigned)pos % f->len];
        return c == 'x' ? 2 : c == 'o' ? 1 : 0;
    }
    switch (g) {                                 /* FREE: the gong, the half, the quarters, the kajar every beat */
    case 0: return pos == 0 ? 2 : 0;
    case 1: case 3: return 2 * pos == len ? 2 : 0;
    case 2: return pos && 2 * pos != len && len >= 4 && pos % (len / 4) == 0 ? 2 : 0;
    default: return pos & 1 ? 1 : 2;
    }
}
int g_place(int form, int pos, int len)
{
    if (g_strike(form, 0, pos, len))
        return PL_GONG;
    if (g_strike(form, 1, pos, len) || g_strike(form, 3, pos, len))
        return PL_KEMONG;
    if (g_strike(form, 2, pos, len))
        return PL_KEMPUR;
    return PL_KAJAR;
}
