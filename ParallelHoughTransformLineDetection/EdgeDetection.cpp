#include "EdgeDetection.h"
#include <cmath>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/blocked_range2d.h>

static const int Kx[3][3] = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} };
static const int Ky[3][3] = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} };

GrayImage to_grayscale(const RGBImage& in) {
    GrayImage out(in.w, in.h);
    for (int y = 0; y < in.h; ++y)
        for (int x = 0; x < in.w; ++x) {
            size_t i = (static_cast<size_t>(y) * in.w + x) * 3;
            out.at(x, y) = 0.299f * in.px[i] + 0.587f * in.px[i + 1] + 0.114f * in.px[i + 2];
        }
    return out;
}

GrayImage to_grayscale_tbb(const RGBImage& in) {
    GrayImage out(in.w, in.h);
    const int w = in.w;
    tbb::parallel_for(tbb::blocked_range<int>(0, in.h),
        [&](const tbb::blocked_range<int>& rows) {
            for (int y = rows.begin(); y != rows.end(); ++y)
                for (int x = 0; x < w; ++x) {
                    size_t i = (static_cast<size_t>(y) * w + x) * 3;
                    out.at(x, y) = 0.299f * in.px[i] + 0.587f * in.px[i + 1] + 0.114f * in.px[i + 2];
                }
        });
    return out;
}

static inline float sobel_at(const GrayImage& in, int x, int y) {
    float gx = 0.0f, gy = 0.0f;
    for (int j = -1; j <= 1; ++j)
        for (int i = -1; i <= 1; ++i) {
            float v = in.at(x + i, y + j);
            gx += Kx[j + 1][i + 1] * v;
            gy += Ky[j + 1][i + 1] * v;
        }
    return std::sqrt(gx * gx + gy * gy);
}

GrayImage sobel_edges(const GrayImage& in, float threshold) {
    GrayImage out(in.w, in.h);
    for (int y = 1; y < in.h - 1; ++y)
        for (int x = 1; x < in.w - 1; ++x)
            out.at(x, y) = (sobel_at(in, x, y) >= threshold) ? 255.0f : 0.0f;
    return out;
}

GrayImage sobel_edges_tbb(const GrayImage& in, float threshold) {
    GrayImage out(in.w, in.h);
    tbb::parallel_for(tbb::blocked_range2d<int>(1, in.h - 1, 1, in.w - 1),
        [&](const tbb::blocked_range2d<int>& r) {
            for (int y = r.rows().begin(); y != r.rows().end(); ++y)
                for (int x = r.cols().begin(); x != r.cols().end(); ++x)
                    out.at(x, y) = (sobel_at(in, x, y) >= threshold) ? 255.0f : 0.0f;
        });
    return out;
}
