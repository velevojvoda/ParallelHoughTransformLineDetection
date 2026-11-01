#pragma once
#include <string>
#include <vector>

// Run the full Hough pipeline as a TBB flow graph over a list of images.
// Phases are graph nodes; multiple images flow through concurrently.

void run_pipeline(const std::vector<std::string>& images,
    float threshold = 150.0f, double voteRatio = 0.5, int nbhd = 20);
