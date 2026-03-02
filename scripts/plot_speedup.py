import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import os
import sys

# ==========================================
# 配置参数
# ==========================================
INPUT_CSV = os.path.join('..', 'data', 'results', 'efficiency_log.csv')
OUTPUT_PDF = os.path.join('..', 'data', 'results', 'rq1_speedup.pdf')

# 图表样式设置 (适配 LaTeX 论文)
sns.set_theme(style="whitegrid")
sns.set_context("paper", font_scale=1.5) # 放大字体以适应论文排版
plt.rcParams['font.family'] = 'serif'    # 使用衬线字体 (Times New Roman风格)

def calculate_geo_mean(series):
    """计算几何平均数 (Geometric Mean)，用于加速比统计"""
    return np.exp(np.mean(np.log(series)))

def main():
    # 1. 检查文件是否存在
    if not os.path.exists(INPUT_CSV):
        print(f"[Error] Input file not found: {INPUT_CSV}")
        print("Please run the C++ experiment (Exp_Efficiency) first.")
        sys.exit(1)

    print(f"-> Loading data from {INPUT_CSV}...")
    df = pd.read_csv(INPUT_CSV)

    # 2. 数据预处理
    # 处理除零错误：如果 GC-DBDR 耗时为 0 (小于计时器精度)，设为 1us 以避免无穷大
    df['Time_GC_DBDR_us'] = df['Time_GC_DBDR_us'].replace(0, 1)
    
    # 计算加速比 (Speedup = Batch / Incremental)
    df['Speedup'] = df['Time_Batch_us'] / df['Time_GC_DBDR_us']

    # 3. 打印统计摘要 (用于填入论文文本)
    print("\n=== RQ1 Statistical Summary ===")
    for op in ['INS', 'DEL']:
        subset = df[df['Operation'] == op]['Speedup']
        if subset.empty: continue
        
        geo_mean = calculate_geo_mean(subset)
        p50 = np.percentile(subset, 50)
        p99 = np.percentile(subset, 99)
        max_val = np.max(subset)
        
        print(f"Operation: {op}")
        print(f"  - Geometric Mean Speedup: {geo_mean:.2f}x")
        print(f"  - Median Speedup:         {p50:.2f}x")
        print(f"  - Max Speedup:            {max_val:.2f}x")

    # 4. 绘图 (Boxplot)
    plt.figure(figsize=(8, 6))
    
    # 定义颜色 (蓝色系)
    palette = {"INS": "#3498db", "DEL": "#e74c3c"}

    ax = sns.boxplot(
        x='Operation', 
        y='Speedup', 
        data=df, 
        palette=palette,
        width=0.5,
        linewidth=1.5,
        fliersize=3,      # 异常点大小
        flierprops={"marker": "o", "markerfacecolor": "gray", "alpha": 0.5}
    )

    # 5. 调整坐标轴
    plt.yscale('log')  # 关键：使用对数坐标，因为加速比跨度极大 (1x - 10000x)
    
    # 添加基准线 (y=1, 即无加速)
    plt.axhline(y=1, color='black', linestyle='--', linewidth=1, alpha=0.7, label="Baseline (1x)")

    # 标签与标题
    plt.title('RQ1: Speedup Distribution (GC-DBDR vs. Batch)', fontweight='bold', pad=15)
    plt.ylabel('Speedup Factor (Log Scale)')
    plt.xlabel('Operation Type')
    
    # 优化 Y 轴刻度显示
    # 设置主要刻度为 1, 10, 100, 1000, 10000
    ax.yaxis.set_major_formatter(plt.FuncFormatter(lambda y, _: '{:.0f}x'.format(y)))
    
    plt.tight_layout()

    # 6. 保存
    print(f"\n-> Saving plot to {OUTPUT_PDF}...")
    plt.savefig(OUTPUT_PDF, dpi=300, bbox_inches='tight')
    print("Done.")

if __name__ == "__main__":
    main()