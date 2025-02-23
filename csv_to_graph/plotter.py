import sys
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker

def plot_csv(in_millions, out_file: str):
    csv = pd.read_csv(sys.argv[1])

    plt.rcParams["figure.autolayout"] = True
    plt.rcParams["figure.figsize"] = [12.00, 10.00]

    implementations = csv['Implementation'].unique()
    markers = ['o', 's', '^', 'D', 'v', 'p', '*']

    fig, ax = plt.subplots()

    for i, imp in enumerate(implementations):
        subset = csv[csv['Implementation'] == imp]
        ax.plot(subset['Pixels'], subset['Average'], label=imp, marker=markers[i % len(markers)], markersize=5)

    ax.set_title("BrightnessAndContrast Implementation Comparison", fontsize=16)
    ax.set_xlabel("Image size (Pixels)", fontsize=14)
    ax.set_ylabel(f"Avg. time over {sys.argv[2]} runs (s)", fontsize=14)
    
    if (in_millions != "0"):
        ax.xaxis.set_major_formatter(mticker.FuncFormatter(millions))

    ax.legend(title="Implementation", bbox_to_anchor=(1.05, 1), loc='upper left')
    ax.grid(True)

    plt.tight_layout()
    plt.savefig(out_file)

def millions(x, pos):
    return f'{int(x/1e6)}M'

if __name__ == '__main__':
    if len(sys.argv) != 5:
        print("Usage: plotter.py <input_csv> <no_runs> <output_file (.png)> <in_millions (0/1)>")
        exit(1)
    
    plot_csv(sys.argv[4], sys.argv[3]) # 1 - file, 2 - runs, 3 - outfile
