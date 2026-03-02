#!/bin/bash

# ==============================================================================
# GC-DBDR 实验自动化脚本 (macOS 修复版)
# 功能：环境检查 -> 编译(Release) -> 运行实验(RQ1-3) -> 绘图 -> 归档结果
# ==============================================================================

# 设置错误中断：任何命令失败则立即终止脚本
set -e

# 定义颜色常量 (用于终端美化)
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# 获取脚本所在目录作为项目根目录
PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
DATA_DIR="${PROJECT_ROOT}/data/results"
SCRIPTS_DIR="${PROJECT_ROOT}/scripts"
ARCHIVE_DIR="${PROJECT_ROOT}/archive"
TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S")

# 打印带时间戳的日志
log() {
    echo -e "${BLUE}[$(date +"%T")] $1${NC}"
}

success() {
    echo -e "${GREEN}[SUCCESS] $1${NC}"
}

warn() {
    echo -e "${YELLOW}[WARNING] $1${NC}"
}

error() {
    echo -e "${RED}[ERROR] $1${NC}"
    exit 1
}

# ==============================================================================
# 1. 环境与依赖检查
# ==============================================================================
log "Step 1: Checking Environment..."

# 检查 CMake
if ! command -v cmake &> /dev/null; then
    error "CMake is not installed. Please run: brew install cmake"
fi

# 检查 Python3
if ! command -v python3 &> /dev/null; then
    error "Python3 is not installed. Please install it."
fi

# 检查 Python 库依赖
log "Checking Python dependencies..."
REQUIRED_PKG="pandas matplotlib seaborn numpy"
for PKG in $REQUIRED_PKG; do
    if ! python3 -c "import $PKG" &> /dev/null; then
        warn "Python package '$PKG' not found. Installing..."
        pip3 install $PKG
    fi
done
success "Environment check passed."

# ==============================================================================
# 2. 构建项目 (Release 模式)
# ==============================================================================
log "Step 2: Building Project (Release Mode)..."

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 配置 CMake (强制 Release 模式以启用 -O3)
cmake -DCMAKE_BUILD_TYPE=Release .. || error "CMake configuration failed."

# 编译 (使用 macOS 的 sysctl 获取核心数进行并行编译)
CORES=$(sysctl -n hw.ncpu)
log "Compiling with $CORES cores..."
make -j$CORES || error "Compilation failed."

success "Build complete."

# ==============================================================================
# 3. 运行实验 (C++)
# ==============================================================================
log "Step 3: Running Experiments..."

# FIX: 关键修复 - 返回项目根目录运行，确保相对路径 (data/results) 正确
cd "$PROJECT_ROOT"

# 确保输出目录存在
mkdir -p "$DATA_DIR"

# 运行 RQ1: Efficiency
log "--> Running RQ1: Efficiency & Speedup..."
if [ -f "build/Exp_Efficiency" ]; then
    ./build/Exp_Efficiency || error "Exp_Efficiency failed."
else
    error "Executable build/Exp_Efficiency not found."
fi

# 运行 RQ2: Memory (如果存在)
if [ -f "build/Exp_Memory" ]; then
    log "--> Running RQ2: Memory Scalability..."
    ./build/Exp_Memory || error "Exp_Memory failed."
fi

# 运行 RQ3: Latency (如果存在)
if [ -f "build/Exp_Latency" ]; then
    log "--> Running RQ3: Tail Latency..."
    ./build/Exp_Latency || error "Exp_Latency failed."
fi

success "All experiments finished."

# ==============================================================================
# 4. 可视化 (Python)
# ==============================================================================
log "Step 4: Generating Plots..."

cd "$SCRIPTS_DIR"

# 检查 CSV 是否存在
if [ ! -f "${DATA_DIR}/efficiency_log.csv" ]; then
    error "Data file efficiency_log.csv missing."
fi

log "--> Plotting RQ1 (Speedup)..."
python3 plot_speedup.py || warn "Failed to plot speedup."

# 如果有 RQ2 数据
if [ -f "${DATA_DIR}/memory_log.csv" ]; then
    log "--> Plotting RQ2 (Memory)..."
    python3 plot_memory.py || warn "Failed to plot memory."
fi

# 如果有 RQ3 数据
if [ -f "${DATA_DIR}/latency_log.csv" ]; then
    log "--> Plotting RQ3 (Latency)..."
    python3 plot_latency.py || warn "Failed to plot latency."
fi

success "Visualization complete. PDF files are in data/results/"

# ==============================================================================
# 5. 归档结果 (防止覆盖)
# ==============================================================================
log "Step 5: Archiving Results..."

TARGET_ARCHIVE="${ARCHIVE_DIR}/Run_${TIMESTAMP}"
mkdir -p "$TARGET_ARCHIVE"

# 复制 CSV 和 PDF 到归档目录
cp "${DATA_DIR}"/*.csv "$TARGET_ARCHIVE" 2>/dev/null || true
cp "${DATA_DIR}"/*.pdf "$TARGET_ARCHIVE" 2>/dev/null || true

# 也可以把日志或摘要放进去
echo "Experiment run on macOS $(sw_vers -productVersion)" > "${TARGET_ARCHIVE}/meta.txt"
date >> "${TARGET_ARCHIVE}/meta.txt"

success "Results archived to: ${TARGET_ARCHIVE}"

echo -e "\n${GREEN}========================================================${NC}"
echo -e "${GREEN}   ALL TASKS COMPLETED SUCCESSFULLY   ${NC}"
echo -e "${GREEN}========================================================${NC}"
# 打开 Finder 显示结果 (macOS 特有)
open "$TARGET_ARCHIVE"