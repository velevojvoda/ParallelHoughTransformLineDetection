#include "FlowGraph.h"
#include "Image.h"
#include "EdgeDetection.h"
#include "Hough.h"
#include "Drawing.h"

#include <tbb/flow_graph.h>
#include <memory>
#include <cstdio>


namespace {
    struct Job {
        std::string path;
        int index = 0;
        float threshold; double voteRatio; int nbhd;

        RGBImage color;
        GrayImage gray, edges;
        HoughAccumulator H;
        std::vector<Line> lines;
    };
    using JobPtr = std::shared_ptr<Job>;
}

void run_pipeline(const std::vector<std::string>& images,
    float threshold, double voteRatio, int nbhd) {
    using namespace tbb::flow;
    graph g;

    size_t next = 0;
    input_node<JobPtr> src(g, [&](tbb::flow_control& fc) -> JobPtr {
        if (next >= images.size()) { fc.stop(); return JobPtr{}; }
        auto j = std::make_shared<Job>();
        j->path = images[next];
        j->index = static_cast<int>(next);
        j->threshold = threshold; j->voteRatio = voteRatio; j->nbhd = nbhd;
        ++next;
        return j;
        });

    //Phase nodes (unlimited = many images may be in a stage at once)
    function_node<JobPtr, JobPtr> n_load(g, unlimited, [](JobPtr j) {
        j->color = load_rgb(j->path);
        j->gray = to_grayscale(j->color);
        return j;
        });
    function_node<JobPtr, JobPtr> n_edges(g, unlimited, [](JobPtr j) {
        j->edges = sobel_edges(j->gray, j->threshold);
        return j;
        });
    function_node<JobPtr, JobPtr> n_hough(g, unlimited, [](JobPtr j) {
        j->H = hough_transform(j->edges);
        return j;
        });
    function_node<JobPtr, JobPtr> n_lines(g, unlimited, [](JobPtr j) {
        j->lines = detect_lines(j->H, j->voteRatio, j->nbhd);
        return j;
        });

    function_node<JobPtr, continue_msg> n_out(g, serial, [](JobPtr j) {
        RGBImage overlay = j->color;
        draw_lines(overlay, j->lines);
        std::string name = "lines_" + std::to_string(j->index) + ".png";
        save_rgb_png(name, overlay);
        std::printf("[%d] %s : %dx%d -> %zu lines -> %s\n",
            j->index, j->path.c_str(), j->color.w, j->color.h,
            j->lines.size(), name.c_str());
        return continue_msg();
        });

    //src -> load -> edges -> hough -> lines -> out
    make_edge(src, n_load);
    make_edge(n_load, n_edges);
    make_edge(n_edges, n_hough);
    make_edge(n_hough, n_lines);
    make_edge(n_lines, n_out);

    src.activate();
    g.wait_for_all();      // block until every image finished
}
