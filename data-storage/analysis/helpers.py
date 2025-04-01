import pandas as pd
import numpy as np
import os
import matplotlib.pyplot as plt

def remove_zero_rows(df):
    """
    Remove rows with zero value in any column.
    """
    if df is None:
        return df
    zero_rows = (df == 0).any(axis=1)
    removed = zero_rows.sum()
    if removed > 0:
        print(f"Removed {removed} rows with zero values before delta calculation.")
    return df[~zero_rows]


def load_and_merge_logs(folder_path):
    filenames = ["ENC.csv", "DEC.csv", "DS_ENC.csv", "DS_DEC.csv", "RTT.csv"]
    merged_df = None

    for fname in filenames:
        full_path = os.path.join(folder_path, fname)
        if os.path.exists(full_path):
            df = pd.read_csv(full_path)
            df = df.drop(columns=["Seq_Num"], errors="ignore")
            prefix = fname.split(".")[0]
            df = df.rename(
                columns={col: f"{prefix}_{col}"
                         for col in df.columns})
            merged_df = df if merged_df is None else pd.concat([merged_df, df],
                                                               axis=1)
    if merged_df is None:
        print(f"No files found in {folder_path}.")
        return None
    if "NONE_scen" in folder_path:
        merged_df = merged_df[["RTT_Start_Time", "RTT_End_Time"]]
    # ✅ Remove rows with zero values after merging
    merged_df = remove_zero_rows(merged_df)

    # Convert timestamps to deltas
    for prefix in ["ENC", "DEC", "DS_ENC", "DS_DEC", "RTT"]:
        start_col = f"{prefix}_Start_Time"
        end_col = f"{prefix}_End_Time"
        if start_col in merged_df.columns and end_col in merged_df.columns:
            merged_df[f"{prefix}_Delta"] = merged_df[end_col] - merged_df[
                start_col]
            merged_df.drop(columns=[start_col, end_col], inplace=True)
            if prefix.startswith("DS_"):
                merged_df[f"{prefix}_Delta"] = (
                    merged_df[f"{prefix}_Delta"] /
                    1000).round().astype(int)

    return merged_df


def load_multiple_logs(data_path, base_names, count):
    return {
        f"{name}_{i}_df":
        load_and_merge_logs(os.path.join(data_path, f"{name}{i}"))
        for name in base_names
        for i in range(1, count + 1)
    }


def compute_stats(logs, base_name, metric):
    stats = []
    for i in range(1, 7):
        df = logs.get(f"{base_name}_{i}_df")
        if df is not None and f"{metric}_Delta" in df.columns:
            stats.append({
                "Scenario": f"{base_name}_{i}",
                f"Mean_{metric}": df[f"{metric}_Delta"].mean(),
                f"Std_{metric}": df[f"{metric}_Delta"].std()
            })
    return pd.DataFrame(stats)


def plot_bar_results(dfs_dict, metric):

    scenarios = [f"scen_{i}" for i in range(1, 6 + 1)]

    # Prepare data

    methods = list(dfs_dict.keys())

    means = []
    stds = []
    for method in methods:
        means.append(dfs_dict[method][f"Mean_{metric}"].values)
        stds.append(dfs_dict[method][f"Std_{metric}"].values)

    x = np.arange(len(scenarios))  # positions for groups
    width = 1/(len(methods) + 1)  # width of each bar

    # Create plot
    fig, ax = plt.subplots(figsize=(10, 6))

    # Plot each method
    for i in range(len(methods)):
        ax.bar(x + i * width,
               means[i],
               width,
               yerr=stds[i],
               label=methods[i],
               capsize=5)

    # Labels and ticks
    ax.set_xlabel("Scenario")
    ax.set_ylabel(f"{metric} [ms]")
    ax.set_title(f"Mean {metric} per Scenario")
    ax.set_xticks(x + width)
    ax.set_xticklabels(scenarios)
    ax.legend()

    plt.tight_layout()
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.show()
