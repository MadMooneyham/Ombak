/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Small-canvas renderer (no full framebuffer). Draw text/lines into
 * an off-screen strip, then blit it in one DMA transfer. Pixels are stored
 * byte-swapped (the panel takes RGB565 big-endian). */
typedef struct {               /* proportional, see tools/gen_font.py */
    uint8_t h;
    uint8_t pad;               /* bitmap starts this many pixels left of the pen */
    uint8_t first, last;
    const uint8_t *adv;        /* advance per glyph */
    const uint8_t *bw;         /* bitmap width per glyph (starts FONT_PAD left of the pen) */
    const uint16_t *off;       /* byte offset of each glyph */
    const uint8_t *data;
} felucca_font_t;
#include "felucca_font.h"

#define CV_MAX (240u * 144u)      /* the main area, both bands, is 240 x 144 */
static uint16_t cv_px[CV_MAX] __attribute__((section(".pool")));
static uint32_t cv_w, cv_h;
static int32_t cv_oy;            /* y offset for graph drawing */
static int32_t cv_y0, cv_y1;     /* the rows drawing may touch */

#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define C_BLACK 0x0000u
#define C_WHITE 0xFFFFu              /* accent only: what is being touched / where we are */
/* Ombak's one palette, BRONZE (Penumbra's ECLIPSE, warmed): five steps, darkest to brightest, on a warm
 * black: brown-grey structure (rules, empty cells, labels), the bronze's gold for what is secondary or
 * active, lamplight for values. Colour on the screen then means something. */
#define C_LINE RGB(52, 42, 32)       /* 1 rules, separators */
#define C_DIM RGB(104, 90, 74)       /* 2 inactive, units */
#define C_GRAY RGB(168, 153, 138)    /* 3 labels */
#define C_AMB RGB(214, 165, 78)      /* 4 the bronze: secondary, active */
#define C_HI RGB(240, 231, 216)      /* 5 lamplight: values */
#define C_BG RGB(14, 11, 9)          /* the night behind everything */

static inline uint16_t swap16(uint32_t c) { return (uint16_t)(((c >> 8) & 0xFFu) | ((c & 0xFFu) << 8)); }

static void cv_begin(uint32_t w, uint32_t h, uint16_t bg)
{
    uint32_t i, n;
    if (w * h > CV_MAX)
        h = CV_MAX / w;
    lcd_sync();                     /* the last blit may still read cv_px */
    cv_w = w;
    cv_h = h;
    cv_oy = cv_y0 = 0;
    cv_y1 = (int32_t)h;
    n = w * h;
    for (i = 0; i < n; i++)
        cv_px[i] = swap16(bg);
}

static void cv_blit(uint32_t x, uint32_t y) { lcd_blit(x, y, cv_w, cv_h, cv_px); }

static inline void cv_pset(int32_t x, int32_t y, uint16_t c)
{
    y += cv_oy;
    if ((uint32_t)x < cv_w && y >= cv_y0 && y < cv_y1)
        cv_px[(uint32_t)y * cv_w + (uint32_t)x] = swap16(c);
}

static void cv_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    int32_t x1 = x + w, y1 = y + h + cv_oy, i;
    uint16_t sc = swap16(c);
    y += cv_oy;                             /* clipped once, then filled row by row */
    if (x < 0)
        x = 0;
    if (y < cv_y0)
        y = cv_y0;
    if (x1 > (int32_t)cv_w)
        x1 = (int32_t)cv_w;
    if (y1 > cv_y1)
        y1 = cv_y1;
    for (; y < y1; y++)
        for (i = x; i < x1; i++)
            cv_px[(uint32_t)y * cv_w + (uint32_t)i] = sc;
}

/* glyph index of a character: lower case folds to upper case when the font
 * has none, anything missing (and C1 controls) draws as '?' */
static uint32_t glyph(const felucca_font_t *f, uint32_t ch)
{
    if (ch >= 'a' && ch <= 'z' && f->last < 'a')
        ch -= 32u;
    if (ch < f->first || ch > f->last || (ch >= 127u && ch < 160u))
        ch = '?';
    return ch - f->first;
}

/* text in colour c, each pixel's coverage blended over what is already on the canvas (an
 * anti-aliased face on a coloured box gets no dark fringe); returns the end x */
static int32_t cv_text(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    uint32_t r = c >> 11, g = (c >> 5) & 63u, b = c & 31u;
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), gx, gy, w, bpr;
        const uint8_t *gd;
        w = f->bw[gi];
        bpr = (w + 1u) / 2u;
        gd = f->data + f->off[gi];
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t a = gd[gy * bpr + gx / 2u];
                int32_t px = x - f->pad + (int32_t)gx, py = y + (int32_t)gy + cv_oy;
                a = (gx & 1u) ? (a & 15u) : (a >> 4);
                if (!a || (uint32_t)px >= cv_w || py < cv_y0 || py >= cv_y1)
                    continue;
                if (a == 15u) {
                    cv_px[(uint32_t)py * cv_w + (uint32_t)px] = swap16(c);
                } else {
                    uint32_t bg = swap16(cv_px[(uint32_t)py * cv_w + (uint32_t)px]);
                    uint32_t br = bg >> 11, bgg = (bg >> 5) & 63u, bb = bg & 31u;
                    uint32_t nr = (r * a + br * (15u - a)) / 15u, ng = (g * a + bgg * (15u - a)) / 15u,
                             nb = (b * a + bb * (15u - a)) / 15u;
                    cv_px[(uint32_t)py * cv_w + (uint32_t)px] = swap16((nr << 11) | (ng << 5) | nb);
                }
            }
        x += f->adv[gi];
    }
    return x;
}

static int32_t text_w(const felucca_font_t *f, const char *s)
{
    int32_t w = 0;
    for (; *s; s++)
        w += f->adv[glyph(f, (uint8_t)*s)];
    return w;
}

/* one-shot: text in a box, cleared to black, blitted */
static void draw_text_box(uint32_t x, uint32_t y, uint32_t w, const felucca_font_t *f, const char *s,
                          uint16_t c, int align)
{
    int32_t tw = text_w(f, s), tx = 0;
    cv_begin(w, f->h, C_BLACK);
    if (align == 1)
        tx = ((int32_t)w - tw) / 2;
    else if (align == 2)
        tx = (int32_t)w - tw;
    cv_text(tx, 0, f, s, c);
    cv_blit(x, y);
    lcd_sync();                     /* one-shots (boot, crash, UBOOT, update) finish here */
}

/* The boot screen: a bronze gong, lit from above left, its boss bright, and the ripples (ombak: the waves)
 * going out from it into the dark; the name, and the Lunar Eclipse Forge mark (tools/gen_logo.py) by the
 * maker's name. */
#include "logo.h"
#include "dsp/fastmath.h"
#define C_NIGHT C_BG
static uint16_t mix565(uint16_t a, uint16_t b, uint32_t t)   /* a to b, t 0..255 */
{
    uint32_t r = ((a >> 11) * (255u - t) + (b >> 11) * t) / 255u;
    uint32_t g = (((a >> 5) & 63u) * (255u - t) + ((b >> 5) & 63u) * t) / 255u;
    uint32_t bl = ((a & 31u) * (255u - t) + (b & 31u) * t) / 255u;
    return (uint16_t)((r << 11) | (g << 5) | bl);
}

static void draw_splash(const char *name, const char *sub)
{
    const uint16_t bronze = RGB(150, 108, 58), face = RGB(62, 44, 26), shine = RGB(255, 236, 190),
                   ripple = RGB(214, 165, 78), grey = RGB(168, 153, 138);
    const float cx = 120.0f, cy = 70.0f, R = 54.0f, BOSS = 17.0f;
    int32_t x, y, sw, sx;
    lcd_fill(0, 0, 240, 240, C_NIGHT);
    cv_begin(240, 140, C_NIGHT);
    for (y = 0; y < (int32_t)cv_h; y++)
        for (x = 0; x < 240; x++) {
            float dx = (float)x - cx, dy = (float)y - cy, d = fm_sqrtf(dx * dx + dy * dy), a;
            float lit = d > 0.5f ? 0.5f - 0.5f * (dx + dy) * (0.7071f / d) : 0.5f;   /* 1 facing the light */
            uint16_t c = C_NIGHT;
            int k;
            for (k = 0; k < 3; k++) {            /* the ripples, fainter as they go */
                float rr = R + 12.0f + 13.0f * (float)k, e = 1.0f - (d > rr ? d - rr : rr - d) / 1.6f;
                if (e > 0.0f)
                    c = mix565(c, ripple, (uint32_t)(e * (90.0f - 26.0f * (float)k)));
            }
            if (d < R + 1.0f) {
                uint16_t g;
                if (d < BOSS) {                  /* the boss: a dome, its highlight up and to the left */
                    float hx = dx + 5.0f, hy = dy + 6.0f, h = fm_expf(-(hx * hx + hy * hy) / 60.0f);
                    g = mix565(mix565(face, bronze, (uint32_t)(200.0f - 120.0f * d / BOSS)), shine, (uint32_t)(h * 235.0f));
                } else if (d < R - 9.0f) {       /* the face, a little light across it */
                    g = mix565(face, bronze, (uint32_t)(40.0f + 60.0f * lit * (d - BOSS) / (R - BOSS)));
                } else {                         /* the rim, catching the light */
                    g = mix565(mix565(face, bronze, (uint32_t)(110.0f + 145.0f * lit)), shine, (uint32_t)(70.0f * lit * lit));
                }
                if (d > BOSS - 1.0f && d < BOSS + 1.0f)   /* the boss's foot, in shadow */
                    g = mix565(g, face, 140u);
                a = d > R - 0.5f ? R + 0.5f - d : 1.0f;
                if (a > 0.0f)
                    c = mix565(c, g, (uint32_t)(a * 255.0f));
            }
            cv_px[(uint32_t)y * cv_w + (uint32_t)x] = swap16(c);
        }
    cv_blit(0, 6);
    cv_begin(240, 60, C_NIGHT);                  /* the names, on the same night; the maker's mark by its name */
    cv_text((240 - text_w(&FONT_M, name)) / 2, 6, &FONT_M, name, RGB(240, 231, 216));
    sw = text_w(&FONT_XS, sub) + LOGO_T_W + 6;
    sx = (240 - sw) / 2;
    for (y = 0; y < LOGO_T_H; y++)
        for (x = 0; x < LOGO_T_W; x++) {
            uint32_t a = LOGO_T_PX[y * ((LOGO_T_W + 1) / 2) + x / 2];
            a = (x & 1) ? (a & 15u) : (a >> 4);
            if (a) {
                uint32_t i = (uint32_t)(38 + y) * cv_w + (uint32_t)(sx + x);
                cv_px[i] = swap16(mix565(swap16(cv_px[i]), grey, a * 17u));
            }
        }
    cv_text(sx + LOGO_T_W + 6, 40, &FONT_XS, sub, grey);
    cv_blit(0, 160);
    lcd_sync();
}
