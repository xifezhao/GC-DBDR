import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os
import sys
import matplotlib.ticker as ticker

# ==========================================
# 配置参数
# ==========================================
INPUT_CSV = os.path.join('..', 'data', 'results', 'memory_log.csv')
OUTPUT_PDF = os.path.join('..', 'data', 'results', 'rq2_memory.pdf')

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
        print("Please run the C++ experiment (Exp_Memory) first.")
        sys.exit(1)

    print(f"-> Loading memory data from {INPUT_CSV}...")
    df = pd.read_csv(INPUT_CSV)

    # 2. 计算统计指标
    # 计算平均开销因子 (Overhead Factor)
    # 过滤掉初始阶段（边数很少时），因为底噪会导致比率失真
    stable_df = df[df['Edges'] > df['Edges'].max() * 0.2] 
    avg_factor = stable_df['Growth_Factor'].mean()
    max_mb_gc = df['GC_DBDR_MB'].max()
    max_mb_batch = df['Batch_MB'].max()

    print("\n=== RQ2 Statistical Summary ===")
    print(f"  - Peak Memory (GC-DBDR): {max_mb_gc:.2f} MB")
    print(f"  - Peak Memory (Batch):   {max_mb_batch:.2f} MB")
    print(f"  - Average Overhead Factor: {avg_factor:.2f}x")

    # 3. 绘图
    plt.figure(figsize=(8, 6))
    
    # 绘制 Batch Baseline (虚线，表示这是理论下界)
    plt.plot(df['Edges'], df['Batch_MB'], 
             label='Fast-CFL (Batch Baseline)', 
             color='#7f8c8d', # 灰色
             linestyle='--', 
             linewidth=2)

    # 绘制 GC-DBDR (实线，带标记)
    plt.plot(df['Edges'], df['GC_DBDR_MB'], 
             label='GC-DBDR (Derivation Hypergraph)', 
             color='#2980b9', # 蓝色
             linestyle='-', 
             linewidth=2,
             marker='o',
             markevery=len(df)//10, # 不要每个点都画标记，太乱
             markersize=6)

    # 4. 视觉增强：填充区域
    # 这一块区域代表了“为了实现 O(Delta) 更新速度而支付的空间代价”
    plt.fill_between(df['Edges'], 
                     df['Batch_MB'], 
                     df['GC_DBDR_MB'], 
                     color='#3498db', 
                     alpha=0.15, 
                     label='Structural Overhead')

    # 5. 图表装饰
    plt.title('RQ2: Memory Scalability Analysis', fontweight='bold', pad=15)
    plt.xlabel('Number of Edges in Graph')
    plt.ylabel('Resident Set Size (MB)')
    
    # 格式化坐标轴
    ax = plt.gca()
    ax.xaxis.set_major_formatter(ticker.EngFormatter()) # 如 1000 -> 1k
    
    # 添加注释：显示平均开销因子
    # 在图的中间偏右位置写上 "~3.5x Overhead"
    mid_idx = len(df) // 2
    mid_x = df.iloc[mid_idx]['Edges']
    mid_y_gc = df.iloc[mid_idx]['GC_DBDR_MB']
    mid_y_batch = df.iloc[mid_idx]['Batch_MB']
    
    plt.annotate(f'~{avg_factor:.1f}x Overhead', 
                 xy=(mid_x, (mid_y_gc + mid_y_batch)/2), 
                 xytext=(mid_x + df['Edges'].max()*0.1, (mid_y_gc + mid_y_batch)/2),
                 arrowprops=dict(arrowstyle='->', color='black'),
                 fontsize=12,
                 fontweight='bold')

    plt.legend(loc='upper left', frameon=True, framealpha=0.9)
    plt.tight_layout()

    # 6. 保存
    print(f"-> Saving plot to {OUTPUT_PDF}...")
    plt.savefig(OUTPUT_PDF, dpi=300, bbox_inches='tight')
    print("Done.")

if __name__ == "__main__":
    main()