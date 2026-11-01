#include "Benchmark.h"
#include "Image.h"
#include "EdgeDetection.h"
#include "Hough.h"
#include "Drawing.h"

#include <tbb/global_control.h>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <thread>
#include <vector>
#include <algorithm>

using Clock = std::chrono::high_resolution_clock;
static double ms(Clock::time_point a) {
    return std::chrono::duration<double, std::milli>(Clock::now() - a).count();
}
template <class F> static double best_ms(F f, int reps = 5) {
    double best = 1e18;
    for (int i = 0; i < reps; ++i) { auto t = Clock::now(); f(); best = std::min(best, ms(t)); }
    return best;
}

void run_benchmark(const std::string& image, float threshold, double voteRatio, int nbhd,
    const std::string& results_path) {
    double t_load = 0.0;
    RGBImage color;
    { auto t = Clock::now(); color = load_rgb(image); t_load = ms(t); }
    if (color.w == 0) { std::printf("Error: cannot load '%s'\n", image.c_str()); return; }

    GrayImage gray, edges; HoughAccumulator H; std::vector<Line> lines;

    double s_gray = best_ms([&] { gray = to_grayscale(color); });
    double p_gray = best_ms([&] { gray = to_grayscale_tbb(color); });

    double s_edge = best_ms([&] { edges = sobel_edges(gray, threshold); });
    double p_edge = best_ms([&] { edges = sobel_edges_tbb(gray, threshold); });

    double s_hough = best_ms([&] { H = hough_transform(edges); });
    double p_hough = best_ms([&] { H = hough_transform_tbb(edges); });

    double s_line = best_ms([&] { lines = detect_lines(H, voteRatio, nbhd); });
    double p_line = best_ms([&] { lines = detect_lines_tbb(H, voteRatio, nbhd); });

    double s_total = s_gray + s_edge + s_hough + s_line;
    double p_total = p_gray + p_edge + p_hough + p_line;

    RGBImage overlay = color;
    draw_lines(overlay, lines);
    save_gray_png("grayscale.png", gray);
    save_gray_png("edges.png", edges);
    save_accumulator_png("accumulator.png", H);
    save_rgb_png("lines.png", overlay);

    unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    std::vector<unsigned> threadCounts;
    for (unsigned n = 1; n <= hw; n *= 2) threadCounts.push_back(n);
    if (threadCounts.back() != hw) threadCounts.push_back(hw);

    struct ScaleRow { unsigned threads; double total_ms; };
    std::vector<ScaleRow> scaling;
    for (unsigned n : threadCounts) {
        tbb::global_control gc(tbb::global_control::max_allowed_parallelism, n);
        double t = best_ms([&] {
            GrayImage g = to_grayscale_tbb(color);
            GrayImage e = sobel_edges_tbb(g, threshold);
            HoughAccumulator h = hough_transform_tbb(e);
            volatile size_t k = detect_lines_tbb(h, voteRatio, nbhd).size(); (void)k;
            }, 3);
        scaling.push_back({ n, t });
    }
    double base = scaling.front().total_ms;

    std::ofstream f(results_path);
    f << "Hough line detection - benchmark results\n";
    f << "========================================\n";
    f << "Image            : " << image << "  (" << color.w << "x" << color.h << ")\n";
    f << "Detected lines   : " << lines.size() << "\n";
    f << "Edge threshold   : " << threshold << "   vote_ratio: " << voteRatio
        << "   nbhd: " << nbhd << "\n";
    f << "Hardware threads : " << hw << "\n";
    f << "Load time        : " << t_load << " ms\n\n";

    f << "Per-phase timing (best of 5 runs)\n";
    f << "---------------------------------------------------------------\n";
    f << "Phase            Sequential    Parallel(TBB)    Speedup\n";
    char buf[256];
    auto row = [&](const char* name, double s, double p) {
        std::snprintf(buf, sizeof(buf), "%-14s   %9.2f ms   %9.2f ms    %6.2fx\n",
            name, s, p, p > 0 ? s / p : 0.0);
        f << buf;
        };
    row("grayscale", s_gray, p_gray);
    row("sobel edges", s_edge, p_edge);
    row("hough", s_hough, p_hough);
    row("detect lines", s_line, p_line);
    f << "---------------------------------------------------------------\n";
    row("TOTAL", s_total, p_total);
    f << "\n";

    f << "Scalability (full parallel pipeline)\n";
    f << "-------------------------------------------\n";
    f << "Threads     Total time     Speedup vs 1 thread\n";
    for (auto& r : scaling) {
        std::snprintf(buf, sizeof(buf), "%5u   %12.2f ms   %10.2fx\n",
            r.threads, r.total_ms, r.total_ms > 0 ? base / r.total_ms : 0.0);
        f << buf;
    }
    f.close();

    std::printf("Benchmark on %s (%dx%d): %zu lines\n",
        image.c_str(), color.w, color.h, lines.size());
    std::printf("Total: seq=%.2f ms  par=%.2f ms  speedup=%.2fx\n",
        s_total, p_total, p_total > 0 ? s_total / p_total : 0.0);
    std::printf("Results written to %s\n", results_path.c_str());
}
