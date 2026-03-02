import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import os
import sys
import matplotlib.ticker as ticker

# ==========================================
# 配置参数
# ==========================================
INPUT_CSV = os.path.join('..', 'data', 'results', 'latency_log.csv')
OUTPUT_PDF = os.path.join('..', 'data', 'results', 'rq3_latency.pdf')

# 论文级绘图风格
sns.set_theme(style="whitegrid")
plt.rcParams['font.family'] = 'serif'
plt.rcParams['font.serif'] = ['Times New Roman'] + plt.rcParams['font.serif']
plt.rcParams['axes.labelsize'] = 14
plt.rcParams['xtick.labelsize'] = 12
plt.rcParams['ytick.labelsize'] = 12
plt.rcParams['legend.fontsize'] = 12
plt.rcParams['figure.titlesize'] = 16

def main():
    # 1. 加载数据
    if not os.path.exists(INPUT_CSV):
        print(f"[Error] Input file not found: {INPUT_CSV}")
        print("Please run the C++ experiment (Exp_Latency) first.")
        sys.exit(1)

    print(f"-> Loading latency data from {INPUT_CSV}...")
    df = pd.read_csv(INPUT_CSV)

    # 2. 数据预处理
    # 转换微秒 -> 毫秒
    df['Latency_ms'] = df['Latency_us'] / 1000.0
    
    # 处理 Log Scale 的 0 值问题 (替换为极小值 0.001 ms = 1 us)
    # 这在对数轴上代表 "瞬时"
    df['Latency_ms'] = df['Latency_ms'].replace(0, 0.001)

    # 3. 计算统计指标 (打印到控制台供论文引用)
    p50 = np.percentile(df['Latency_ms'], 50)
    p90 = np.percentile(df['Latency_ms'], 90)
    p99 = np.percentile(df['Latency_ms'], 99)
    p999 = np.percentile(df['Latency_ms'], 99.9)
    max_lat = df['Latency_ms'].max()

    print("\n=== RQ3 Statistical Summary ===")
    print(f"  - Median (P50):  {p50:.4f} ms")
    print(f"  - Tail (P99):    {p99:.4f} ms")
    print(f"  - Tail (P99.9):  {p999:.4f} ms")
    print(f"  - Max Latency:   {max_lat:.4f} ms")

    # 4. 绘图 (ECDF - Empirical Cumulative Distribution Function)
    plt.figure(figsize=(8, 6))

    # 使用 Seaborn 绘制 CDF
    # hue='Op_Type' 可以区分插入和删除，展示它们是否具有不同的分布特征
    sns.ecdfplot(
        data=df, 
        x="Latency_ms", 
        hue="Op_Type", 
        palette={"INS": "#3498db", "DEL": "#e74c3c"},
        linewidth=2,
        alpha=0.8
    )

    # 绘制总体分布 (Combined) - 黑色虚线
    sns.ecdfplot(
        data=df,
        x="Latency_ms",
        color="black",
        linestyle="--",
        linewidth=1.5,
        label="Combined",
        alpha=0.6
    )

    # 5. 坐标轴调整 (关键步骤)
    plt.xscale('log') # 使用对数轴展示长尾
    
    # 设置 X 轴刻度格式 (0.001, 0.01, 0.1, 1, 10, 100)
    ax = plt.gca()
    ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda y, _: '{:g}'.format(y)))
    
    plt.xlabel('Update Latency (ms) [Log Scale]')
    plt.ylabel('Cumulative Probability (CDF)')
    plt.title('RQ3: Latency Distribution & Tail Behavior', fontweight='bold', pad=15)

    # 6. 添加关键阈值线和注释
    
    # 阈值线：100ms (Nielsen's limit for feeling instantaneous)
    plt.axvline(x=100, color='gray', linestyle=':', linewidth=2)
    plt.text(100, 0.05, ' Interactive Limit\n (100 ms)', color='gray', fontsize=10, ha='left')

    # 阈值线：50ms (流畅动画阈值)
    plt.axvline(x=50, color='gray', linestyle=':', linewidth=1, alpha=0.5)

    # 标注 P99 点
    # 在 Combined 曲线上找到 P99 的位置
    # 注意：由于是对数轴，位置需要细心调整
    plt.plot(p99, 0.99, 'ro') # 红点
    plt.annotate(f'P99: {p99:.2f} ms', 
                 xy=(p99, 0.99), 
                 xytext=(p99 * 0.1, 0.9), # 文本位置稍微向左下偏移
                 arrowprops=dict(facecolor='black', shrink=0.05, width=1, headwidth=5),
                 fontsize=10,
                 color='red')

    # 图例优化
    # 由于 sns.ecdfplot(hue=...) 自动生成了图例，我们需要手动添加 "Combined"
    # 或者直接使用 matplotlib 的默认图例合并机制
    handles, labels = ax.get_legend_handles_labels()
    # 调整顺序：Combined 放最后
    plt.legend(handles, labels, loc='lower right', frameon=True)

    plt.tight_layout()

    # 7. 保存
    print(f"-> Saving plot to {OUTPUT_PDF}...")
    plt.savefig(OUTPUT_PDF, dpi=300, bbox_inches='tight')
    print("Done.")

if __name__ == "__main__":
    main()