// Image correlation matcher for the Goodix 52xd (27c6:521d) press sensor

// Copyright (C) 2026 denizberkin <berkindeniz2000@gmail.com>

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

// The 521d window is 64x80 px (about 3x4 mm). That holds too few minutiae
// for NBIS/bozorth3, so frames are compared directly instead: a central
// patch of one frame, rotated over a small range, is searched for in the
// other by normalised cross-correlation.

#include "goodix52xd_match.h"

#include <math.h>
#include <string.h>

#define PATCH_H 30
#define PATCH_W 24
#define ANGLE_MAX 15
#define ANGLE_STEP 3
#define N_ANGLES (2 * ANGLE_MAX / ANGLE_STEP + 1)

typedef struct {
    int w;
    int h;
    float* d;
} Img;

typedef struct {
    Img img;                              // preprocessed frame
    float patch[N_ANGLES][PATCH_H * PATCH_W]; // zero-mean rotated patches
    double patch_norm[N_ANGLES];          // sqrt(sum(patch^2))
    double* sum;                          // integral image of img
    double* sum_sq;                       // integral image of img^2
} Prepared;

static int reflect101(int i, int n)
{
    if (n == 1)
        return 0;
    while (i < 0 || i >= n) {
        if (i < 0)
            i = -i;
        if (i >= n)
            i = 2 * n - 2 - i;
    }
    return i;
}

static int reflect(int i, int n)
{
    while (i < 0 || i >= n) {
        if (i < 0)
            i = -i - 1;
        if (i >= n)
            i = 2 * n - 1 - i;
    }
    return i;
}

// Separable Gaussian blur, kernel size and borders as OpenCV uses for
// floating point images.
static void gauss_blur(const float* src, float* dst, int w, int h, double sigma)
{
    int ks = ((int) lrint(sigma * 8 + 1)) | 1;
    int r = ks / 2;
    double* k = g_new(double, ks);
    float* tmp = g_new(float, w * h);
    double s = 0;

    for (int i = 0; i < ks; i++) {
        double x = i - r;
        k[i] = exp(-x * x / (2 * sigma * sigma));
        s += k[i];
    }
    for (int i = 0; i < ks; i++)
        k[i] /= s;

    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            double acc = 0;
            for (int i = 0; i < ks; i++)
                acc += k[i] * src[y * w + reflect101(x + i - r, w)];
            tmp[y * w + x] = acc;
        }
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            double acc = 0;
            for (int i = 0; i < ks; i++)
                acc += k[i] * tmp[reflect101(y + i - r, h) * w + x];
            dst[y * w + x] = acc;
        }

    g_free(tmp);
    g_free(k);
}

// The sensor reports exact zeros on a dead border; keep the rows and columns
// that are at least 90% live in the mean of the enrolled frames.
static void valid_box(const float* const* frames, guint n, int w, int h,
                      int* y0, int* y1, int* x0, int* x1)
{
    float* mean = g_new0(float, w * h);

    for (guint f = 0; f < n; f++)
        for (int i = 0; i < w * h; i++)
            mean[i] += frames[f][i] / n;

    *y0 = h;
    *y1 = 0;
    *x0 = w;
    *x1 = 0;
    for (int y = 0; y < h; y++) {
        int live = 0;
        for (int x = 0; x < w; x++)
            live += mean[y * w + x] > 0.5;
        if (live > 0.9 * w) {
            *y0 = MIN(*y0, y);
            *y1 = y + 1;
        }
    }
    for (int x = 0; x < w; x++) {
        int live = 0;
        for (int y = 0; y < h; y++)
            live += mean[y * w + x] > 0.5;
        if (live > 0.9 * h) {
            *x0 = MIN(*x0, x);
            *x1 = x + 1;
        }
    }
    g_free(mean);
}

// Crop, remove the low-frequency pressure gradient and normalise variance.
static Img normalise(const float* frame, int w, int y0, int y1, int x0, int x1)
{
    Img o = {x1 - x0, y1 - y0, NULL};
    int n = o.w * o.h;
    float* blur = g_new(float, n);
    double mean = 0, var = 0;

    o.d = g_new(float, n);
    for (int y = 0; y < o.h; y++)
        memcpy(o.d + y * o.w, frame + (y + y0) * w + x0, o.w * sizeof(float));

    gauss_blur(o.d, blur, o.w, o.h, 6);
    for (int i = 0; i < n; i++) {
        o.d[i] -= blur[i];
        mean += o.d[i];
    }
    mean /= n;
    for (int i = 0; i < n; i++)
        var += (o.d[i] - mean) * (o.d[i] - mean);
    var = sqrt(var / n) + 1e-9;
    for (int i = 0; i < n; i++)
        o.d[i] /= var;

    g_free(blur);
    return o;
}

// Remove the fixed sensor pattern, smooth pixel noise and normalise local
// contrast.
static void finish(Img* img, const float* pattern)
{
    int n = img->w * img->h;
    float* a = g_new(float, n);
    float* b = g_new(float, n);

    for (int i = 0; i < n; i++)
        a[i] = img->d[i] - pattern[i];
    gauss_blur(a, img->d, img->w, img->h, 0.8);
    for (int i = 0; i < n; i++)
        a[i] = img->d[i] * img->d[i];
    gauss_blur(a, b, img->w, img->h, 8);
    for (int i = 0; i < n; i++)
        img->d[i] /= sqrt(b[i]) + 1e-6;

    g_free(a);
    g_free(b);
}

// Rotate about the centre with bilinear sampling and reflected borders.
static void rotate(const Img* src, float* dst, double degrees)
{
    double t = degrees * G_PI / 180, a = cos(t), b = sin(t);
    double cx = src->w / 2.0, cy = src->h / 2.0;

    for (int y = 0; y < src->h; y++)
        for (int x = 0; x < src->w; x++) {
            double sx = a * (x - cx) - b * (y - cy) + cx;
            double sy = b * (x - cx) + a * (y - cy) + cy;
            int ix = (int) floor(sx), iy = (int) floor(sy);
            double fx = sx - ix, fy = sy - iy;
            int x0 = reflect(ix, src->w), x1 = reflect(ix + 1, src->w);
            int y0 = reflect(iy, src->h), y1 = reflect(iy + 1, src->h);

            dst[y * src->w + x] =
                (1 - fy) * ((1 - fx) * src->d[y0 * src->w + x0] + fx * src->d[y0 * src->w + x1]) +
                fy * ((1 - fx) * src->d[y1 * src->w + x0] + fx * src->d[y1 * src->w + x1]);
        }
}

static void prepare(Prepared* p, Img img)
{
    int w = img.w, h = img.h;
    float* rot = g_new(float, w * h);

    p->img = img;
    for (int k = 0; k < N_ANGLES; k++) {
        int py = h / 2 - PATCH_H / 2, px = w / 2 - PATCH_W / 2;
        double mean = 0, ss = 0;

        rotate(&img, rot, -ANGLE_MAX + k * ANGLE_STEP);
        for (int y = 0; y < PATCH_H; y++)
            for (int x = 0; x < PATCH_W; x++)
                mean += p->patch[k][y * PATCH_W + x] = rot[(py + y) * w + px + x];
        mean /= PATCH_H * PATCH_W;
        for (int i = 0; i < PATCH_H * PATCH_W; i++) {
            p->patch[k][i] -= mean;
            ss += p->patch[k][i] * p->patch[k][i];
        }
        p->patch_norm[k] = sqrt(ss);
    }
    g_free(rot);

    p->sum = g_new0(double, (w + 1) * (h + 1));
    p->sum_sq = g_new0(double, (w + 1) * (h + 1));
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            double v = img.d[y * w + x];
            int i = (y + 1) * (w + 1) + x + 1;
            p->sum[i] = v + p->sum[i - 1] + p->sum[i - (w + 1)] - p->sum[i - (w + 1) - 1];
            p->sum_sq[i] = v * v + p->sum_sq[i - 1] + p->sum_sq[i - (w + 1)] -
                           p->sum_sq[i - (w + 1) - 1];
        }
}

static void prepared_clear(Prepared* p)
{
    g_free(p->img.d);
    g_free(p->sum);
    g_free(p->sum_sq);
}

static double box_sum(const double* s, int w, int y, int x, int bh, int bw)
{
    int W = w + 1;
    return s[(y + bh) * W + x + bw] - s[y * W + x + bw] - s[(y + bh) * W + x] + s[y * W + x];
}

// Best normalised cross-correlation of any rotated patch of a within b.
static double locate(const Prepared* a, const Prepared* b)
{
    const int n = PATCH_H * PATCH_W, w = b->img.w, h = b->img.h;
    double best = -1;

    for (int k = 0; k < N_ANGLES; k++) {
        const float* t = a->patch[k];

        for (int y = 0; y + PATCH_H <= h; y++)
            for (int x = 0; x + PATCH_W <= w; x++) {
                double s = box_sum(b->sum, w, y, x, PATCH_H, PATCH_W);
                double ss = box_sum(b->sum_sq, w, y, x, PATCH_H, PATCH_W);
                double var = ss - s * s / n, num = 0;

                if (var <= 1e-9)
                    continue;
                for (int py = 0; py < PATCH_H; py++) {
                    const float* row = b->img.d + (y + py) * w + x;
                    const float* trow = t + py * PATCH_W;
                    for (int px = 0; px < PATCH_W; px++)
                        num += trow[px] * row[px];
                }
                best = MAX(best, num / (a->patch_norm[k] * sqrt(var)));
            }
    }
    return best;
}

double goodix52xd_match_score(const float* const* enrolled, guint n_enrolled,
                              const float* probe, int width, int height,
                              double stop_at)
{
    int y0, y1, x0, x1, n;
    Img* normed;
    float* pattern;
    Prepared* gallery;
    Prepared p;
    double best = -1;

    g_return_val_if_fail(n_enrolled > 0, -1);

    valid_box(enrolled, n_enrolled, width, height, &y0, &y1, &x0, &x1);
    if (y1 - y0 < PATCH_H + 4 || x1 - x0 < PATCH_W + 4)
        return -1;

    normed = g_new(Img, n_enrolled);
    for (guint i = 0; i < n_enrolled; i++)
        normed[i] = normalise(enrolled[i], width, y0, y1, x0, x1);

    n = normed[0].w * normed[0].h;
    pattern = g_new0(float, n);
    for (guint i = 0; i < n_enrolled; i++)
        for (int j = 0; j < n; j++)
            pattern[j] += normed[i].d[j] / n_enrolled;

    gallery = g_new(Prepared, n_enrolled);
    for (guint i = 0; i < n_enrolled; i++) {
        finish(&normed[i], pattern);
        prepare(&gallery[i], normed[i]);
    }

    {
        Img pi = normalise(probe, width, y0, y1, x0, x1);
        finish(&pi, pattern);
        prepare(&p, pi);
    }

    for (guint i = 0; i < n_enrolled && best < stop_at; i++) {
        best = MAX(best, locate(&p, &gallery[i]));
        if (best < stop_at)
            best = MAX(best, locate(&gallery[i], &p));
    }

    for (guint i = 0; i < n_enrolled; i++)
        prepared_clear(&gallery[i]);
    prepared_clear(&p);
    g_free(gallery);
    g_free(pattern);
    g_free(normed);
    return best;
}
