#pragma once
#include "Image.h"
#include <vector>
#include <string>

struct Line {
    double theta;   // radians
    double rho;     // pixels
    int    votes;
};

struct HoughAccumulator {
    int numTheta = 0, numRho = 0;
    double thetaStep = 0.0, rhoStep = 0.0, rhoMin = 0.0;
    std::vector<int> acc;
    int  at(int t, int r) const { return acc[static_cast<size_t>(t) * numRho + r]; }
    int& at(int t, int r) { return acc[static_cast<size_t>(t) * numRho + r]; }
};

HoughAccumulator hough_transform(const GrayImage& edges,
    double thetaStepDeg = 1.0, double rhoStep = 1.0);
HoughAccumulator hough_transform_tbb(const GrayImage& edges,
    double thetaStepDeg = 1.0, double rhoStep = 1.0);

std::vector<Line> detect_lines(const HoughAccumulator& H,
    double voteRatio = 0.5, int nbhd = 20);
std::vector<Line> detect_lines_tbb(const HoughAccumulator& H,
    double voteRatio = 0.5, int nbhd = 20);

bool save_accumulator_png(const std::string& path, const HoughAccumulator& H);
