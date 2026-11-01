# Parallel Hough Transform Line Detection

Line detection in images using the Hough transform, with a sequential and a parallel (Intel oneAPI TBB) implementation of every phase

## Pipeline
| Phase | TBB construction |
|---|---|
| RGB -> grayscale | `parallel_for` + `blocked_range` |
| Sobel edges | `parallel_for` + `blocked_range2d` |
| Hough acumulator | `parallel_reduce` |
| Line detection | `parallel_for` + `concurrent_vector` |
| More images | `flow_graph` pipeline |

## Requirements

- Windows 10/11, x64
- [Visual Studio 2022](https://visualstudio.microsoft.com/) **17.13 or newer**
- [Intel oneAPI Base Toolkit](https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit-download.html) 2025
  - must include the **Intel oneAPI DPC++/C++ Compiler** and **Intel oneAPI Threading Building Blocks (oneTBB)**


## Build

1. Clone the repository
2. Open `ParallelHoughTransformLineDetection.slnx` in Visual Studio
3. Select the **Release | x64** configuration
4. **Build → Build Solution**

The executable is created in `x64\Release\ParallelHoughTransformLineDetection.exe`.

## Running

The program depends on oneTBB and the Intel runtime libraries, so it has to be started
from a terminal where the oneAPI environment is set up.

### 1. Open the oneAPI terminal

From the Start menu, open
**Intel oneAPI command prompt for Intel 64 for Visual Studio 2022**

### 2. Go to the project folder

```
cd <path-to-repo>\ParallelHoughTransformLineDetection
```

The input image is in the project folder, and all output files are written to the
folder the program is started from.

### 3. Run

```
..\x64\Release\ParallelHoughTransformLineDetection.exe [mode] [images...] [options]
```

| Mode | Description |
|---|---|
| `benchmark` | Runs every phase sequentially and in parallel on one image, measures speedup and thread scalability |
| `graph` | Processes multiple images concurrently through a TBB flow graph pipeline, where each processing phase is a graph node |

| Option | Description | Default |
|---|---|---|
| `--threshold <float>` | Sobel edge threshold | `200` |
| `--vote-ratio <float>` | Minimum votes relative to the strongest line | `0.5` |
| `--nbhd <int>` | Neighbourhood size for peak detection | `20` |
| `--help` | Show usage | |

### Examples

In folder there is already an image picture_with_straight_lines.png for using.

```
..\x64\Release\ParallelHoughTransformLineDetection.exe
..\x64\Release\ParallelHoughTransformLineDetection.exe benchmark picture_with_straight_lines.png --threshold 150
..\x64\Release\ParallelHoughTransformLineDetection.exe graph img1.jpg img2.jpg img3.jpg --nbhd 15
..\x64\Release\ParallelHoughTransformLineDetection.exe --help
```

### Output

**benchmark** mode writes:
- `grayscale.png`, `edges.png`, `accumulator.png`, `lines.png` – intermediate and final images
- `results.txt` – per-phase timings (sequential vs. TBB) and scalability table

**graph** mode writes `lines_0.png`, `lines_1.png`, … – one image with detected lines per input.

## Third-party

- [stb_image / stb_image_write](https://github.com/nothings/stb) by Sean Barrett – public domain / MIT
