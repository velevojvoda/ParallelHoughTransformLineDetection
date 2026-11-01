#include "Hough.h"
#include "stb_image_write.h"   
#include <cmath>
#include <algorithm>

#include <tbb/parallel_reduce.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/concurrent_vector.h>

static const double PI = 3.14159265358979323846;

static void setup_dims(const GrayImage& edges, double thetaStepDeg, double rhoStep,
    HoughAccumulator& H,
    std::vector<double>& cosT, std::vector<double>& sinT) {
    double diag = std::sqrt(double(edges.w) * edges.w + double(edges.h) * edges.h); //to know the maximum possible rho value
    H.thetaStep = thetaStepDeg * PI / 180.0; // convert degrees to radians
    H.rhoStep = rhoStep; // step size for rho
    H.rhoMin = -diag; // minimum possible rho value
    H.numTheta = static_cast<int>(std::round(180.0 / thetaStepDeg)); // number of theta steps (0 to 180 degrees)
    H.numRho = static_cast<int>(std::ceil(2.0 * diag / rhoStep)) + 1; // number of rho steps
    cosT.resize(H.numTheta);
    sinT.resize(H.numTheta);
    for (int t = 0; t < H.numTheta; ++t) { // precompute cos and sin for each theta
        double theta = t * H.thetaStep;
        cosT[t] = std::cos(theta);
        sinT[t] = std::sin(theta);
    }
}

HoughAccumulator hough_transform(const GrayImage& edges, double thetaStepDeg, double rhoStep) {
    HoughAccumulator H;
    std::vector<double> cosT, sinT;
    setup_dims(edges, thetaStepDeg, rhoStep, H, cosT, sinT);
    H.acc.assign(static_cast<size_t>(H.numTheta) * H.numRho, 0);

    for (int y = 0; y < edges.h; ++y)
        for (int x = 0; x < edges.w; ++x) {
            if (edges.at(x, y) < 128.0f) continue;
            for (int t = 0; t < H.numTheta; ++t) {
                double rho = x * cosT[t] + y * sinT[t];
                int rIdx = static_cast<int>(std::round((rho - H.rhoMin) / H.rhoStep));
                if (rIdx >= 0 && rIdx < H.numRho) H.at(t, rIdx)++;
            }
        }
    return H;
}

// parallel version of the Hough transform using TBB
namespace {
    struct HoughReducer {
        const GrayImage& edges;
        const std::vector<double>& cosT;
        const std::vector<double>& sinT;
        int numTheta, numRho;
        double rhoMin, rhoStep;
        std::vector<int> local;

        HoughReducer(const GrayImage& e, const std::vector<double>& c, const std::vector<double>& s,
            int nt, int nr, double rmin, double rstep)
            : edges(e), cosT(c), sinT(s), numTheta(nt), numRho(nr),
            rhoMin(rmin), rhoStep(rstep),
            local(static_cast<size_t>(nt)* nr, 0) {
        }

        HoughReducer(HoughReducer& o, tbb::split)
            : edges(o.edges), cosT(o.cosT), sinT(o.sinT),
            numTheta(o.numTheta), numRho(o.numRho),
            rhoMin(o.rhoMin), rhoStep(o.rhoStep),
            local(static_cast<size_t>(o.numTheta)* o.numRho, 0) {
        }

        void operator()(const tbb::blocked_range<int>& rows) {
            for (int y = rows.begin(); y != rows.end(); ++y)
                for (int x = 0; x < edges.w; ++x) {
                    if (edges.at(x, y) < 128.0f) continue;
                    for (int t = 0; t < numTheta; ++t) {
                        double rho = x * cosT[t] + y * sinT[t];
                        int rIdx = static_cast<int>(std::round((rho - rhoMin) / rhoStep));
                        if (rIdx >= 0 && rIdx < numRho)
                            local[static_cast<size_t>(t) * numRho + rIdx]++;
                    }
                }
        }

        void join(const HoughReducer& o) {
            for (size_t i = 0; i < local.size(); ++i) local[i] += o.local[i];
        }
    };
}

HoughAccumulator hough_transform_tbb(const GrayImage& edges, double thetaStepDeg, double rhoStep) {
    HoughAccumulator H;
    std::vector<double> cosT, sinT;
    setup_dims(edges, thetaStepDeg, rhoStep, H, cosT, sinT);

    HoughReducer body(edges, cosT, sinT, H.numTheta, H.numRho, H.rhoMin, H.rhoStep);
    tbb::parallel_reduce(tbb::blocked_range<int>(0, edges.h), body);
    H.acc = std::move(body.local);
    return H;
}

std::vector<Line> detect_lines(const HoughAccumulator& H, double voteRatio, int nbhd) {
    int maxVote = 0;
    for (int v : H.acc) maxVote = std::max(maxVote, v);
    int voteThreshold = static_cast<int>(voteRatio * maxVote);

    std::vector<Line> lines;
    for (int t = 0; t < H.numTheta; ++t)
        for (int r = 0; r < H.numRho; ++r) {
            int v = H.at(t, r);
            if (v < voteThreshold) continue;
            bool isMax = true;
            for (int dt = -nbhd; dt <= nbhd && isMax; ++dt)
                for (int dr = -nbhd; dr <= nbhd; ++dr) {
                    int tt = t + dt, rr = r + dr;
                    if (tt < 0 || tt >= H.numTheta || rr < 0 || rr >= H.numRho) continue;
                    if (H.at(tt, rr) > v) { isMax = false; break; }
                }
            if (isMax) lines.push_back({ t * H.thetaStep, H.rhoMin + r * H.rhoStep, v });
        }
    return lines;
}


std::vector<Line> detect_lines_tbb(const HoughAccumulator& H, double voteRatio, int nbhd) {
    int maxVote = 0;
    for (int v : H.acc) maxVote = std::max(maxVote, v);
    int voteThreshold = static_cast<int>(voteRatio * maxVote);

    tbb::concurrent_vector<Line> found;

    tbb::parallel_for(tbb::blocked_range<int>(0, H.numTheta),
        [&](const tbb::blocked_range<int>& tr) {
            for (int t = tr.begin(); t != tr.end(); ++t)
                for (int r = 0; r < H.numRho; ++r) {
                    int v = H.at(t, r);
                    if (v < voteThreshold) continue;
                    bool isMax = true;
                    for (int dt = -nbhd; dt <= nbhd && isMax; ++dt)
                        for (int dr = -nbhd; dr <= nbhd; ++dr) {
                            int tt = t + dt, rr = r + dr;
                            if (tt < 0 || tt >= H.numTheta || rr < 0 || rr >= H.numRho) continue;
                            if (H.at(tt, rr) > v) { isMax = false; break; }
                        }
                    if (isMax)
                        found.push_back({ t * H.thetaStep, H.rhoMin + r * H.rhoStep, v });
                }
        });

    return std::vector<Line>(found.begin(), found.end());
}

bool save_accumulator_png(const std::string& path, const HoughAccumulator& H) {
    int maxVote = 0;
    for (int v : H.acc) maxVote = std::max(maxVote, v);
    if (maxVote == 0) maxVote = 1;
    std::vector<unsigned char> buf(static_cast<size_t>(H.numTheta) * H.numRho);
    for (int r = 0; r < H.numRho; ++r)
        for (int t = 0; t < H.numTheta; ++t)
            buf[static_cast<size_t>(r) * H.numTheta + t] =
            static_cast<unsigned char>(255.0f * H.at(t, r) / maxVote + 0.5f);
    return stbi_write_png(path.c_str(), H.numTheta, H.numRho, 1, buf.data(), H.numTheta) != 0;
}
