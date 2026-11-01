#pragma once
#include "Image.h"
#include "Hough.h"
#include <vector>

void draw_line(RGBImage& img, const Line& line,
    unsigned char r, unsigned char g, unsigned char b);

void draw_lines(RGBImage& img, const std::vector<Line>& lines,
    unsigned char r = 255, unsigned char g = 0, unsigned char b = 0);
