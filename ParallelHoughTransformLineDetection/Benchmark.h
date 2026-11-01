#pragma once
#include <string>

void run_benchmark(const std::string& image,
    float threshold = 150.0f, double voteRatio = 0.5, int nbhd = 20,
    const std::string& results_path = "results.txt");
