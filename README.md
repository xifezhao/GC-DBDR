
# GC-DBDR: Correct-by-Construction Dynamic Reachability

[![Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Build](https://img.shields.io/badge/build-CMake-brightgreen.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

This repository contains the reference implementation and experimental framework for the paper:

> **Correct-by-Construction Dynamic Reachability: A Galois-Connected Approach to Bidirected Dyck Languages**
> *Submitted to FM 2026*

The framework implements the **GC-DBDR** algorithm, a dynamic graph analysis technique based on **Abstract Interpretation** and **Differential Fixpoints**. It provides the infrastructure to reproduce the evaluation results (efficiency, scalability, and responsiveness) presented in Section 7 of the paper.

---

## 📂 Project Structure

```text
GC-DBDR-Experiment/
├── CMakeLists.txt              # CMake build configuration
├── README.md                   # This file
├── data/
│   ├── benchmarks/             # Input graphs (DaCapo/Magma formats)
│   └── results/                # Generated CSV logs and PDF plots
├── src/
│   ├── core/                   # Core algorithm (Galois Connections, Hypergraph)
│   │   ├── GC_DBDR.hpp         # The incremental/decremental solver
│   │   └── Types.hpp           # Abstract domain definitions
│   ├── experiments/            # Experiment runners for RQs
│   │   ├── Exp_Efficiency.cpp  # RQ1: Speedup analysis
│   │   └── Exp_Latency.cpp     # RQ3: Tail latency analysis
│   └── utils/                  # Utilities (Timer, GraphLoader)
└── scripts/                    # Visualization (Python)
    ├── plot_speedup.py         # Generates Figure 7.1
    └── plot_latency.py         # Generates Figure 7.3
```

---

## 🛠️ Prerequisites

To build and run the experiments, you need:

1.  **C++ Compiler**: GCC 9+, Clang 10+, or MSVC 2019+ (Must support **C++17**).
2.  **CMake**: Version 3.15 or higher.
3.  **Python 3**: For generating plots (optional if you only need raw data).
    *   Dependencies: `pandas`, `matplotlib`, `seaborn`, `numpy`.

### Install Python Dependencies

```bash
pip3 install pandas matplotlib seaborn numpy
```

---

## 🚀 Building the Project

**Crucial Note**: The experiments measure microsecond-level performance. You **must** build in `Release` mode to enable compiler optimizations (`-O3`). Debug builds will result in misleading performance data.

```bash
# 1. Create a build directory
mkdir build
cd build

# 2. Configure with CMake (Release Mode is mandatory for RQ1)
cmake -DCMAKE_BUILD_TYPE=Release ..

# 3. Compile binaries
make -j$(nproc)
```

---

## 🧪 Reproducing Experiments

The artifacts correspond to the Research Questions (RQs) in Section 7 of the paper.

### 1. RQ1: Efficiency & Speedup
Measures the execution time of Incremental/Decremental updates vs. Batch re-computation.

*   **Target**: `Exp_Efficiency`
*   **Description**: Performs a controlled mutation protocol (50% Insert / 50% Delete) on synthetic or loaded graphs.
*   **Output**: `data/results/efficiency_log.csv`

**Run:**
```bash
cd build
./Exp_Efficiency
```

### 2. RQ2 & RQ3: Scalability & Responsiveness
Measures memory overhead and tail latency distribution (p99 latency) for IDE integration assessment.

*   **Target**: `Exp_Latency` (or derived from Efficiency logs)
*   **Description**: Analyzes the distribution of update times to validate "interactive" performance (<100ms).
*   **Output**: Data appended to `efficiency_log.csv` or specific latency logs.

**Run:**
```bash
# (If separated)
./Exp_Latency
```

---

## 📊 Visualizing Results

After running the C++ binaries, use the Python scripts to generate the LaTeX-ready PDF plots.

```bash
cd scripts

# Generate RQ1 Speedup Plot (Boxplot)
python3 plot_speedup.py
# -> Output: ../data/results/rq1_speedup.pdf

# Generate RQ3 Latency Distribution (CDF)
python3 plot_latency.py
# -> Output: ../data/results/rq3_latency.pdf
```

### Expected Results
*   **Speedup**: You should observe a geometric mean speedup of **>100x** (up to 1000x for sparse updates) compared to the Batch baseline.
*   **Latency**: The CDF should show a heavy-tailed distribution where the median (p50) is <1ms, verifying the "Minimal Influence Region" theory.

---

## 📝 Configuration

You can tweak experiment parameters in `src/experiments/Exp_Efficiency.cpp`:

*   `NUM_NODES`: Size of the synthetic graph (default: 1000).
*   `NUM_MUTATIONS`: Number of random operations to simulate (default: 5000).
*   `BENCHMARK_PATH`: Path to load real-world `.dot` or `.graph` files (if available).

---

## 📚 Citation

If you use this code or the algorithm in your research, please cite the paper:

```bibtex
@article{GC_DBDR_2026,
  title={Correct-by-Construction Dynamic Reachability: A Galois-Connected Approach to Bidirected Dyck Languages},
  author={Anonymous},
  journal={Proceedings of the International Symposium on Formal Methods (FM)},
  year={2026}
}
```

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.