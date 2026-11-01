#pragma once
#include <vector>
#include <string>
#include <cstddef>

struct GrayImage {
    int w = 0, h = 0;
    std::vector<float> px;                       // size w*h
    GrayImage() = default;
    GrayImage(int w_, int h_) : w(w_), h(h_), px(static_cast<size_t>(w_)* h_, 0.0f) {}
    float  at(int x, int y) const { return px[static_cast<size_t>(y) * w + x]; }
    float& at(int x, int y) { return px[static_cast<size_t>(y) * w + x]; }
    //at writes in vector px the exact value of the pixel at position (x,y) in the image
};

struct RGBImage {
    int w = 0, h = 0;
    std::vector<unsigned char> px;               // size w*h*3, for  RGB
    RGBImage() = default;
    RGBImage(int w_, int h_) : w(w_), h(h_), px(static_cast<size_t>(w_)* h_ * 3, 0) {}
    void set(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
        size_t i = (static_cast<size_t>(y) * w + x) * 3;
        px[i] = r; px[i + 1] = g; px[i + 2] = b;
    }
};

// I/O. All stb usage lives in Image.cpp.
RGBImage load_rgb(const std::string& path);      // empty image (w==0) on failure
bool save_gray_png(const std::string& path, const GrayImage& img);
bool save_rgb_png(const std::string& path, const RGBImage& img);
