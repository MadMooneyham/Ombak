/* SPDX-License-Identifier: GPL-3.0-only */
/* Ombak's voices; see bars.h. The instruments' tables are the browser prototype's (prototype/index.html,
 * INST), where they were tuned by ear. */
#include "bars.h"
#include "fastmath.h"
#include "../app/plat.h"

/* ------------------------------------------------------------ the sine --- */
#define SIN_N 2048
static float sin_tab[SIN_N + 1] OMB_POOL;
static inline float sin_t(float turns)           /* any phase within +-64 turns */
{
    float x = turns * (float)SIN_N + (float)(SIN_N * 64), f;
    int32_t i = (int32_t)x;
    f = x - (float)i;
    i &= SIN_N - 1;
    return sin_tab[i] + (sin_tab[i + 1] - sin_tab[i]) * f;
}

/* ------------------------------------------------------ the instruments --- */
/* each carrier: ratio, level, seconds to -60 dB, attack seconds, and its modulator's ratio, index
 * (radians) and seconds to -60 dB (0: none) */
typedef struct { float r, a, d, att, mr, mi, md; } op_spec_t;
typedef struct {
    float gain, ombak, glide;            /* ombak: the share of the beat this instrument takes (gongs beat slower) */
    uint8_t nops;
    op_spec_t op[5];
    float hum_a, hum_rise, hum_d;        /* the gong's hum (0: none) */
    float nz_a, nz_fm, nz_q, nz_d;       /* the mallet: level, centre (x the note), Q, length */
} inst_spec_t;

static const inst_spec_t INST[IN_N] = {
    /* gendèr: bronze keys over bamboo resonators, a padded mallet: 1, 2.76, 5.40 and the resonator's bloom */
    {0.30f, 1.0f, 0.0f, 4, {{1.0f, 1.0f, 5.5f, 0.004f, 1.0f, 0.8f, 0.22f}, {1.0f, 0.35f, 8.0f, 0.09f, 0, 0, 0},
                            {2.76f, 0.10f, 1.3f, 0.003f, 1.0f, 0.5f, 0.12f}, {5.40f, 0.035f, 0.45f, 0.003f, 0, 0, 0}},
     0, 0, 0, 0.05f, 3.0f, 1.2f, 0.025f},
    /* gangsa: brighter, shorter, a hard mallet */
    {0.22f, 1.0f, 0.0f, 5, {{1.0f, 1.0f, 2.6f, 0.002f, 1.0f, 1.4f, 0.07f}, {1.0f, 0.25f, 4.0f, 0.05f, 0, 0, 0},
                            {2.71f, 0.22f, 0.9f, 0.003f, 0, 0, 0}, {5.15f, 0.08f, 0.25f, 0.003f, 0, 0, 0},
                            {8.9f, 0.03f, 0.08f, 0.003f, 0, 0, 0}},
     0, 0, 0, 0.10f, 4.0f, 1.5f, 0.018f},
    /* jublag: the core, soft and long */
    {0.30f, 1.0f, 0.0f, 4, {{1.0f, 1.0f, 6.0f, 0.006f, 1.0f, 0.6f, 0.25f}, {1.0f, 0.4f, 9.0f, 0.12f, 0, 0, 0},
                            {2.8f, 0.08f, 1.6f, 0.003f, 0, 0, 0}, {5.5f, 0.025f, 0.4f, 0.003f, 0, 0, 0}},
     0, 0, 0, 0.04f, 2.5f, 1.0f, 0.03f},
    /* jegogan: an octave under, longer */
    {0.34f, 1.0f, 0.0f, 3, {{1.0f, 1.0f, 8.0f, 0.008f, 1.0f, 0.5f, 0.3f}, {1.0f, 0.4f, 11.0f, 0.15f, 0, 0, 0},
                            {2.8f, 0.06f, 2.0f, 0.003f, 0, 0, 0}},
     0, 0, 0, 0.03f, 2.5f, 1.0f, 0.04f},
    /* reyong: bossed kettles, short */
    {0.15f, 1.0f, 0.0f, 4, {{1.0f, 1.0f, 1.8f, 0.002f, 1.5f, 1.0f, 0.05f}, {2.37f, 0.2f, 0.6f, 0.003f, 0, 0, 0},
                            {3.94f, 0.12f, 0.35f, 0.003f, 0, 0, 0}, {5.6f, 0.05f, 0.15f, 0.003f, 0, 0, 0}},
     0, 0, 0, 0.14f, 6.0f, 3.0f, 0.012f},
    /* gong (wadon, and the lanang): an inharmonic body, a slight fall in pitch, the hum */
    {0.55f, 0.12f, 0.012f, 5, {{1.0f, 0.5f, 9.0f, 0.012f, 1.41f, 0.9f, 0.7f}, {2.03f, 0.20f, 5.0f, 0.003f, 0, 0, 0},
                               {2.86f, 0.13f, 3.2f, 0.003f, 1.0f, 0.3f, 0.4f}, {3.97f, 0.07f, 2.0f, 0.003f, 0, 0, 0},
                               {5.42f, 0.04f, 1.0f, 0.003f, 0, 0, 0}},
     0.9f, 1.1f, 14.0f, 0.10f, 1.5f, 0.7f, 0.08f},
    /* kempur */
    {0.40f, 0.18f, 0.008f, 4, {{1.0f, 0.55f, 6.0f, 0.008f, 1.41f, 0.8f, 0.45f}, {2.07f, 0.2f, 3.2f, 0.003f, 0, 0, 0},
                               {2.92f, 0.12f, 2.0f, 0.003f, 0, 0, 0}, {4.1f, 0.06f, 1.1f, 0.003f, 0, 0, 0}},
     0.7f, 0.6f, 8.0f, 0.08f, 2.0f, 0.8f, 0.05f},
    /* kemong */
    {0.22f, 0.25f, 0.005f, 4, {{1.0f, 0.6f, 4.0f, 0.005f, 1.41f, 0.8f, 0.3f}, {2.1f, 0.2f, 2.0f, 0.003f, 0, 0, 0},
                               {3.1f, 0.1f, 1.2f, 0.003f, 0, 0, 0}, {4.6f, 0.05f, 0.5f, 0.003f, 0, 0, 0}},
     0.5f, 0.3f, 5.0f, 0.08f, 3.0f, 1.0f, 0.03f},
    /* kajar: a muted kettle */
    {0.13f, 0.5f, 0.0f, 2, {{1.0f, 1.0f, 0.2f, 0.001f, 1.47f, 2.2f, 0.04f}, {2.6f, 0.35f, 0.08f, 0.003f, 0, 0, 0}},
     0, 0, 0, 0.3f, 5.0f, 2.0f, 0.012f},
};
static const char *const INST_N[IN_N] = {"GENDER", "GANGSA", "JUBLAG", "JEGOGAN", "REYONG", "GONG", "KEMPUR",
                                         "KEMONG", "KAJAR"};
const char *bars_inst_name(int inst) { return inst >= 0 && inst < IN_N ? INST_N[inst] : ""; }

static float fall(float secs60)                  /* a sample's factor for a fall of 60 dB in secs60 */
{
    return fm_expf(-6.9077553f / (fm_maxf(secs60, 1e-3f) * BARS_SR));
}

void bars_init(bars_t *b)
{
    int i;
    for (i = 0; i <= SIN_N; i++)
        sin_tab[i] = fm_sin_turns((float)i / (float)SIN_N);
    for (i = 0; i < BARS_VOICES; i++)
        b->v[i].on = 0;
    b->rng = 0x13579BDFu;
    b->gen = 1;
    b->cap = BARS_VOICES;
}

/* a free voice (while fewer than cap sound) in the half of the pool with fewer sounding, so the two cores share
 * the work; else the quietest */
static int pick(bars_t *b)
{
    int i, q = 0, on[2] = {0, 0}, free_v[2] = {-1, -1}, h;
    float lo = 1e30f;
    for (i = 0; i < BARS_VOICES; i++) {
        h = i >= BARS_VOICES / 2;
        if (!b->v[i].on) {
            if (free_v[h] < 0)
                free_v[h] = i;
            continue;
        }
        on[h]++;
        if (b->v[i].level < lo) {
            lo = b->v[i].level;
            q = i;
        }
    }
    if (on[0] + on[1] < b->cap) {
        h = on[1] < on[0];
        if (free_v[h] >= 0)
            return free_v[h];
        if (free_v[!h] >= 0)
            return free_v[!h];
    }
    return on[0] + on[1] ? q : 0;
}

bars_handle_t bars_strike(bars_t *b, const bars_strike_t *s)
{
    const inst_spec_t *sp = &INST[s->inst < IN_N ? s->inst : 0];
    const int gong = s->inst >= IN_GONG;
    const float ring = gong ? s->gong_ring : s->ring, g = s->vel * sp->gain;
    bars_handle_t h;
    bars_voice_t *v;
    float x;
    int k;
    h.i = (int16_t)pick(b);
    v = &b->v[h.i];
    v->on = 1;
    v->part = s->part < BARS_PARTS ? s->part : 0;
    v->gen = h.gen = b->gen++;
    v->delay = s->delay;
    v->damp_in = 0xFFFFFFFFu;
    v->damp_k = 1.0f;
    v->gain = 1.0f;
    x = (fm_clampf(s->pan, -1.0f, 1.0f) + 1.0f) * 0.25f;        /* equal power, 1 both sides in the middle */
    v->gl = fm_sin_turns(x + 0.25f) * 1.41421356f;
    v->gr = fm_sin_turns(x) * 1.41421356f;
    v->xsend = 0.0f;
    v->bph = 0.0f;
    v->binc = s->beat * sp->ombak * 0.5f / BARS_SR;
    v->glide = sp->glide;
    v->gk = fm_expf(-256.0f / (0.13f * BARS_SR));
    {
        int n = 0, j;
        float rat[BARS_OPS];
        for (k = 0; k < sp->nops; k++) {
            const op_spec_t *o = &sp->op[k];
            bars_op_t *p;
            float a = g * o->a * (o->att >= 0.05f ? s->bloom : 1.0f) * (gong ? s->body : 1.0f);
            uint32_t att = (uint32_t)(o->att * BARS_SR);
            for (j = 0; j < n; j++)              /* a bloom on a partial already sounding: its second envelope */
                if (o->mr <= 0.0f && rat[j] == o->r)
                    break;
            if (j < n) {
                p = &v->op[j];
                p->batt = att ? att : 1u;
                p->bstep = a / (float)p->batt;
                p->bamp = 0.0f;
                p->bk = fall(o->d * ring);
                continue;
            }
            rat[n] = o->r;
            p = &v->op[n++];
            p->bamp = p->bstep = 0.0f;
            p->batt = 0;
            p->bk = 1.0f;
            p->inc = p->inc0 = s->hz * o->r / BARS_SR;
            p->k = fall(o->d * ring);
            p->a = a;
            p->att = att ? att : 1u;
            p->astep = a / (float)p->att;
            p->amp = 0.0f;
            p->mph = 0.0f;
            p->minc = p->minc0 = s->hz * o->r * o->mr / BARS_SR;
            p->mi = o->mr > 0.0f ? o->mi * s->bright * (1.0f / FM_TWO_PI) : 0.0f;
            p->mk = o->md > 0.0f ? fall(o->md) : 0.0f;
        }
        v->nops = (uint8_t)n;
    }
    v->bcur = 1.0f;
    v->bstep = 0.0f;
    v->hum = sp->hum_a > 0.0f;
    if (v->hum) {                                /* the hum: amp - hamp2, a rise then a long fall */
        bars_op_t *p = &v->op[v->nops];
        float a = g * sp->hum_a * s->hum, rise = sp->hum_rise * s->swell;
        p->ph = 0.0f;
        p->inc = p->inc0 = s->hz / BARS_SR;
        p->amp = p->a = a;
        p->k = fm_expf(-6.9077553f / (sp->hum_d * ring * BARS_SR));
        p->att = p->batt = 0;
        p->astep = p->bamp = p->bstep = 0.0f;
        p->bk = 1.0f;
        p->mi = 0.0f;
        p->minc = p->minc0 = 0.0f;
        v->hamp2 = a;
        v->hk2 = fm_expf(-2.5f / (fm_maxf(rise, 0.02f) * BARS_SR));
    }
    v->nz = g * sp->nz_a * s->mallet * 2.0f;
    v->nzk = fm_expf(-4.0f / (sp->nz_d * BARS_SR));
    v->nlp = v->nbp = 0.0f;
    v->nf = 2.0f * fm_sin_turns(fm_minf(s->hz * sp->nz_fm, 7000.0f) / BARS_SR * 0.5f);
    v->nq = 1.0f / sp->nz_q;
    v->level = g;
    return h;
}

int bars_alive(const bars_t *b, bars_handle_t h)
{
    return h.i >= 0 && h.i < BARS_VOICES && b->v[h.i].on && b->v[h.i].gen == h.gen;
}

void bars_damp(bars_t *b, bars_handle_t h, uint32_t in, float tau)
{
    if (!bars_alive(b, h))
        return;
    b->v[h.i].damp_in = in;
    b->v[h.i].damp_k = fm_expf(-1.0f / (fm_maxf(tau, 0.002f) * BARS_SR));
}

void bars_bend(bars_t *b, bars_handle_t h, float beat, float pan, float gain, float xsend)
{
    bars_voice_t *v;
    float x;
    if (!bars_alive(b, h))
        return;
    v = &b->v[h.i];
    v->binc = beat * 0.5f / BARS_SR;
    x = (fm_clampf(pan, -1.0f, 1.0f) + 1.0f) * 0.25f;
    v->gl = fm_sin_turns(x + 0.25f) * 1.41421356f * gain;
    v->gr = fm_sin_turns(x) * 1.41421356f * gain;
    v->xsend = xsend;
}

static inline float rnd(uint32_t *s)
{
    *s ^= *s << 13;
    *s ^= *s >> 17;
    *s ^= *s << 5;
    return (float)(int32_t)*s * (1.0f / 2147483648.0f);
}

volatile uint32_t bars_sines[2];
int bars_render(bars_t *b, int lo, int hi, float (*bus)[2][256], float *xs, int n)
{
    int vi, sounded = 0;
    uint32_t sines = 0;
    uint32_t rng = b->rng ^ (uint32_t)lo * 2654435761u;
    for (vi = lo; vi < hi; vi++) {
        bars_voice_t *v = &b->v[vi];
        bars_op_t *act[BARS_OPS];
        int na = 0, k, i, start;
        float *L, *R, lvl = 0.0f;
        if (!v->on)
            continue;
        if (v->delay >= (uint32_t)n) {
            v->delay -= (uint32_t)n;
            continue;
        }
        start = (int)v->delay;
        v->delay = 0;
        sounded++;
        if (v->glide > 1e-5f) {                  /* the gong's pitch falls into place */
            float m = 1.0f + v->glide;
            for (k = 0; k < v->nops + v->hum; k++) {
                v->op[k].inc = v->op[k].inc0 * m;
                v->op[k].minc = v->op[k].minc0 * m;
            }
            v->glide *= v->gk;
        }
        for (k = 0; k < v->nops; k++) {          /* the carriers still sounding (above -80 dB) */
            bars_op_t *o = &v->op[k];
            if (o->amp + o->bamp > 1e-4f || o->att || o->batt)
                act[na++] = o;
            if (o->mi < 2e-4f)                   /* (a modulator that has fallen silent stops) */
                o->mi = 0.0f;
        }
        L = bus[v->part][0];
        R = bus[v->part][1];
        {
            uint32_t per = (uint32_t)na + (uint32_t)v->hum;   /* (the carriers, the hum; the beating each 16 samples) */
            for (k = 0; k < na; k++)
                per += act[k]->mi > 0.0f;
            sines += per * (uint32_t)(n - start) + (uint32_t)(n - start + 15) / 16u;
        }
        for (i = start; i < n; i++) {
            float s = 0.0f;
            for (k = 0; k < na; k++) {
                bars_op_t *o = act[k];
                float m = 0.0f;
                if (o->mi > 0.0f) {
                    m = sin_t(o->mph) * o->mi;
                    o->mph += o->minc;
                    if (o->mph >= 1.0f)
                        o->mph -= 1.0f;
                    o->mi *= o->mk;
                }
                s += sin_t(o->ph + m) * (o->amp + o->bamp);   /* (the carrier and its bloom: one sine) */
                o->ph += o->inc;
                if (o->ph >= 1.0f)
                    o->ph -= 1.0f;
                if (o->att) {
                    o->amp += o->astep;
                    o->att--;
                } else {
                    o->amp *= o->k;
                }
                if (o->batt) {
                    o->bamp += o->bstep;
                    o->batt--;
                } else {
                    o->bamp *= o->bk;
                }
            }
            if (v->hum) {                        /* the hum, swelling: the slow fall less the fast one */
                bars_op_t *o = &v->op[v->nops];
                s += sin_t(o->ph) * (o->amp - v->hamp2);
                o->ph += o->inc;
                if (o->ph >= 1.0f)
                    o->ph -= 1.0f;
                o->amp *= o->k;
                v->hamp2 *= v->hk2;
            }
            if (v->nz > 1e-5f) {                 /* the mallet: noise through a band-pass */
                float hp = rnd(&rng) * v->nz - v->nlp - v->nq * v->nbp;
                v->nbp += v->nf * hp;
                v->nlp += v->nf * v->nbp;
                s += v->nbp * v->nq;
                v->nz *= v->nzk;
            }
            if (!(i & 15) || i == start) {       /* the ombak, the pair's beating: slow, so a line each 16 samples */
                float to = sin_t(v->bph + 16.0f * v->binc + 0.25f);
                v->bstep = (to - v->bcur) * (1.0f / 16.0f);
            }
            s *= v->bcur * v->gain;
            v->bcur += v->bstep;
            v->bph += v->binc;
            if (v->bph >= 1.0f)
                v->bph -= 1.0f;
            if (v->damp_in == 0u)
                v->gain *= v->damp_k;
            else if (v->damp_in != 0xFFFFFFFFu)
                v->damp_in--;
            L[i] += s * v->gl;
            R[i] += s * v->gr;
            if (v->xsend != 0.0f)
                xs[i] += s * v->xsend;
        }
        for (k = 0; k < v->nops; k++)
            lvl += v->op[k].amp + v->op[k].bamp + (float)(v->op[k].att != 0 || v->op[k].batt != 0) * v->op[k].a;
        if (v->hum)
            lvl += v->op[v->nops].amp;
        lvl = lvl * v->gain + v->nz;
        v->level = lvl;
        if (lvl < 1e-4f)                         /* (-80 dB: it has rung out) */
            v->on = 0;
        for (k = 0; k < v->nops; k++) {          /* (no denormals in the falls) */
            v->op[k].amp = fm_flush(v->op[k].amp);
            v->op[k].bamp = fm_flush(v->op[k].bamp);
            v->op[k].mi = fm_flush(v->op[k].mi);
        }
        v->nlp = fm_flush(v->nlp);
        v->nbp = fm_flush(v->nbp);
    }
    if (lo == 0)                                 /* (the halves run on two cores: one keeps the seed) */
        b->rng = rng;
    bars_sines[lo == 0] += sines;
    return sounded;
}

int bars_active(const bars_t *b)
{
    int i, n = 0;
    for (i = 0; i < BARS_VOICES; i++)
        n += b->v[i].on;
    return n;
}
