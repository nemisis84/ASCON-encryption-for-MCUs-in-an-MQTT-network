import pandas as pd
import numpy as np
import os
import matplotlib.pyplot as plt

encryption_methods = ["ASCON", "masked_ASCON", "AES-GCM", "NONE"]
scenarios = 12
metrics = ["RTT", "S_PROC", "R_PROC", "DS_PROC", "DS_ENC", "DS_DEC","GW_US_PROC", "GW_DS_PROC", "ENC", "DEC", "HW_and_Network", "%_of_RTT"]
LABEL_ALIASES = {
    "RTT":         "Round‑Trip Time",
    "S_PROC":      "Sensor Sending Processing Time",
    "R_PROC":      "Sensor Receiving Processing Time",
    "DS_PROC":     "Data-Storage Processing Time",
    "DS_ENC":      "Data-Storage Encryption Time",
    "DS_DEC":      "Date-Storage Decryption Time",
    "GW_US_PROC":  "Gateway Upstream Processing Time",
    "GW_DS_PROC":  "Gateway Downstream Processing Time",
    "ENC":         "Sensor Encryption Time",
    "DEC":         "Sensor Decryption Time",
    "HW_and_Network": "Hardware and Network Time",
    "%_of_RTT":    "Hardware and network time as % of RTT"
}

import numpy as np
import pandas as pd

import numpy as np
import pandas as pd

def remove_invalid_rows(df, prefix):
    """
    Remove rows that contain:
      • zero in any column
      • NaN in any column
      • +inf or -inf in any column
      • any Start_Time > End_Time

    Prints counts for each category.
    """
    if df is None:
        return df

    orig_rows = df.shape[0]
    if orig_rows != 100:
        print(f"Warning: original '{prefix}' DataFrame has {orig_rows} rows.")

    # 1) Identify infinities on the original
    mask_inf  = df.isin([np.inf, -np.inf]).any(axis=1)
    # 2) Identify NaNs on the original
    mask_na   = df.isna().any(axis=1)
    # 3) Replace infinities so they turn into NaNs for downstream masks
    df_clean  = df.replace([np.inf, -np.inf], np.nan)
    # 4) Identify zeros
    mask_zero = (df_clean == 0).any(axis=1)
    # 5) Identify Start_Time > End_Time
    mask_time = pd.Series(False, index=df_clean.index)
    for start_col in df_clean.filter(regex=r"_Start_Time$").columns:
        end_col = start_col.replace("_Start_Time", "_End_Time")
        if end_col in df_clean.columns:
            mask_time |= (df_clean[start_col] > df_clean[end_col])

    # Count each
    removed_inf   = mask_inf.sum()
    removed_na    = mask_na.sum()
    removed_zero  = mask_zero.sum()
    removed_time  = mask_time.sum()

    # Print each only if nonzero
    if removed_inf:
        print(f"Removed {removed_inf} rows with infinite values from {prefix}.")
    if removed_na:
        print(f"Removed {removed_na} rows with NaN values from {prefix}.")
    if removed_zero:
        print(f"Removed {removed_zero} rows with zero values from {prefix}.")
    if removed_time:
        print(f"Removed {removed_time} rows where Start_Time > End_Time from {prefix}.")

    # Combine all invalid rows into one mask
    invalid_rows = mask_inf | mask_na | mask_zero | mask_time

    # Return only the valid rows (with infinities already replaced by NaN)
    return df_clean[~invalid_rows]




def load_and_merge_logs(folder_path):
    filenames = ["ENC.csv", "DEC.csv", "DS_ENC.csv", "DS_DEC.csv", "RTT.csv", "S_PROC.csv", "R_PROC.csv", "DS_PROC.csv", "GW_US_PROC.csv", "GW_DS_PROC.csv"]
    merged_df = None

    for fname in filenames:
        full_path = os.path.join(folder_path, fname)
        if os.path.exists(full_path):
            df = pd.read_csv(full_path)
            df = df.drop(columns=["Seq_Num"], errors="ignore")
            prefix = fname.split(".")[0]
            if prefix in ["DS_ENC", "DS_DEC"] and "NONE_scen" in folder_path:
                # Skip DS_ENC and DS_DEC for NONE scenarios
                continue
            df = df.rename(
                columns={col: f"{prefix}_{col}"
                         for col in df.columns})

            merged_df = df if merged_df is None else pd.concat([merged_df, df],
                                                               axis=1)
    
    merged_df = remove_invalid_rows(merged_df, folder_path)
    
    if merged_df is None:
        print(f"No files found in {folder_path}.")
        return None
    
    # Convert timestamps to deltas
    for prefix in ["RTT", "S_PROC", "R_PROC", "DS_PROC", "DS_ENC", "DS_DEC","GW_US_PROC", "GW_DS_PROC", "ENC", "DEC"]:
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


def load_multiple_logs(data_path, encryption_methods, count):
    return {
        f"{encryption_method}_{i}_df":
        load_and_merge_logs(os.path.join(data_path, encryption_method,f"{encryption_method}_scen{i}"))
        for encryption_method in encryption_methods
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

def get_stats(frames: dict, scen: int, category= "RTT"):
    stats = {"Method": [], f"Mean_{category}": [], f"Std_{category}": []}
    for encryption_method in encryption_methods:
        frame = frames[encryption_method][scen-1]
        stats["Method"].append(f"{encryption_method}_{category}")
        if frame is None or f"{category}_Delta" not in frame.columns:
            stats[f"Mean_{category}"].append(0)
            stats[f"Std_{category}"].append(0)
        else:
            stats[f"Mean_{category}"].append(frame[f"{category}_Delta"].mean())
            stats[f"Std_{category}"].append(frame[f"{category}_Delta"].std())
    return pd.DataFrame(stats)


def get_encryption_stats(frames: dict, encryption_method: str, category= "RTT"):
    stats = {f"Mean_{category}": [], f"Std_{category}": []}
    for scen in range(scenarios):
        frame = frames[encryption_method][scen]
        if frame is None or f"{category}_Delta" not in frame.columns:
            stats[f"Mean_{category}"].append(0)
            stats[f"Std_{category}"].append(0)
        else:
            stats[f"Mean_{category}"].append(frame[f"{category}_Delta"].mean())
            stats[f"Std_{category}"].append(frame[f"{category}_Delta"].std())
    return pd.DataFrame(stats)

def plot_bar_results(frames, metric, scenarios=12):

    scens = [f"scen_{i}" for i in range(1, scenarios + 1)]

    means = []
    stds = []

    for encryption_method in encryption_methods:
        frame = get_encryption_stats(frames, encryption_method, metric)
        means.append(frame[f"Mean_{metric}"].values)
        stds.append(frame[f"Std_{metric}"].values)

    x = np.arange(scenarios)  # positions for groups
    width = 1/(len(encryption_methods) + 1)  # width of each bar

    # Create plot
    fig, ax = plt.subplots(figsize=(10, 6))

    # Plot each method
    for i in range(len(encryption_methods)):
        ax.bar(x + i * width,
               means[i],
               width,
               yerr=stds[i],
               label=encryption_methods[i],
               capsize=5)

    # Labels and ticks
    ax.set_xlabel("Scenario")
    ax.set_ylabel("t [ms]")
    ax.set_title(f"Mean {LABEL_ALIASES.get(metric)} per Scenario")
    ax.set_xticks(x + width)
    ax.set_xticklabels(scens)
    ax.legend()

    plt.tight_layout()
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    return fig


def save_figure(frames, path):
    """
    Save the given figure to a file.
    """
    for metric in metrics:
        fig = plot_bar_results(frames, metric)
        fig.savefig(os.path.join(path, metric), bbox_inches='tight')
        print(f"Saved {metric}")
