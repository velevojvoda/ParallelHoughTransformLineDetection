#include "Benchmark.h"
#include "FlowGraph.h"
#include <vector>
#include <string>
#include <chrono>
#include <cstdio>
#include <stdexcept>

static void print_usage(const char* prog) {
    std::printf(
        "Usage:\n"
        "  %s benchmark [image] [options]\n"
        "  %s graph <image1> [image2 ...] [options]\n"
        "\n"
        "Options:\n"
        "  --threshold <float>    Sobel edge threshold      (default 200)\n"
        "  --vote-ratio <float>   min votes / max votes     (default 0.5)\n"
        "  --nbhd <int>           peak neighbourhood size   (default 20)\n"
        "  --help                 show this message\n"
        "\n",
        prog, prog);
}


int main(int argc, char* argv[]) {
    std::string mode = "benchmark"; // here to change the mode: "benchmark" or "graph"
    float threshold = 200.0f;
    double voteRatio = 0.5;
    int nbhd = 20;
	std::vector<std::string> images; // graph mode: can change to many images
    std::string image = "picture_with_straight_lines.png"; // benchmark mode: one image/

    try {
        int i = 1;
		if (i < argc && argv[i][0] != '-') mode = argv[i++];

        for (; i < argc; ++i) {
            std::string a = argv[i];
            auto next = [&]() -> std::string {
                if (i + 1 >= argc) throw std::runtime_error("missing value for " + a);
                return argv[++i];
                };
            
            if (a == "--help" || a == "-h") { print_usage(argv[0]); return 0; }
            else if (a == "--threshold") threshold = std::stof(next());
            else if (a == "--vote-ratio") voteRatio = std::stod(next());
            else if (a == "--nbhd") nbhd = std::stoi(next());
            else if (a[0] == '-') throw std::runtime_error("unknown option " + a);
            else images.push_back(a); 
        }
	}
	catch (const std::exception& e) {
		std::printf("Error: %s\n", e.what());
		print_usage(argv[0]);
		return 1;
	}

    if (mode == "benchmark") {
        if (!images.empty()) image = images[0];
        run_benchmark(image, threshold, voteRatio, nbhd);
    }

    else if (mode == "graph") {
        if (images.empty()) {
            std::printf("Error: graph mode needs at least one image\n");
            print_usage(argv[0]);
            return 1;
        }
        auto t0 = std::chrono::high_resolution_clock::now();
        run_pipeline(images, threshold, voteRatio, nbhd);
        double t = std::chrono::duration<double, std::milli>(
            std::chrono::high_resolution_clock::now() - t0).count();
        std::printf("Pipeline done: %zu images in %.1f ms\n", images.size(), t);
    }
    else {
		std::printf("Error: unknown mode '%s'\n", mode.c_str());
		print_usage(argv[0]);
		return 1;
    }
    return 0;
    }
