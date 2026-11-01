#pragma once
#include "Image.h"

// Phase 1: color -> grayscale (Rec.601 luma).
GrayImage to_grayscale(const RGBImage& in);
GrayImage to_grayscale_tbb(const RGBImage& in);

// Phase 2: Sobel gradient magnitude + threshold -> binary edges (0 or 255).
GrayImage sobel_edges(const GrayImage& in, float threshold);
GrayImage sobel_edges_tbb(const GrayImage& in, float threshold);
