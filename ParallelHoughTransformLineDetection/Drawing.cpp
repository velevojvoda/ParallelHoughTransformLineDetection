#include "Drawing.h"
#include <cmath>

void draw_line(RGBImage& img, const Line& line,
    unsigned char r, unsigned char g, unsigned char b) {
    double c = std::cos(line.theta);
    double s = std::sin(line.theta);

    auto plot = [&](int x, int y) {
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                int xx = x + dx, yy = y + dy;
                if (xx >= 0 && xx < img.w && yy >= 0 && yy < img.h)
                    img.set(xx, yy, r, g, b);
            }
        };

    if (std::fabs(s) > std::fabs(c)) {
        for (int x = 0; x < img.w; ++x) {
            int y = static_cast<int>(std::round((line.rho - x * c) / s));
            if (y >= 0 && y < img.h) plot(x, y);
        }
    }
    else {
        for (int y = 0; y < img.h; ++y) {
            int x = static_cast<int>(std::round((line.rho - y * s) / c));
            if (x >= 0 && x < img.w) plot(x, y);
        }
    }
}

void draw_lines(RGBImage& img, const std::vector<Line>& lines,
    unsigned char r, unsigned char g, unsigned char b) {
    for (const Line& l : lines) draw_line(img, l, r, g, b);
}
