
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#include "Image.h"
#include <algorithm>

RGBImage load_rgb(const std::string& path) {
    int w, h, ch;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 3);
    if (!data) return RGBImage();
    RGBImage img(w, h);
    std::copy(data, data + static_cast<size_t>(w) * h * 3, img.px.begin());
    stbi_image_free(data);
    return img;
}

bool save_gray_png(const std::string& path, const GrayImage& img) {
    std::vector<unsigned char> buf(static_cast<size_t>(img.w) * img.h);
    for (size_t i = 0; i < buf.size(); ++i) {
        float v = std::clamp(img.px[i], 0.0f, 255.0f);
        buf[i] = static_cast<unsigned char>(v + 0.5f);
    }
    return stbi_write_png(path.c_str(), img.w, img.h, 1, buf.data(), img.w) != 0;
}

bool save_rgb_png(const std::string& path, const RGBImage& img) {
    return stbi_write_png(path.c_str(), img.w, img.h, 3, img.px.data(), img.w * 3) != 0;
}
