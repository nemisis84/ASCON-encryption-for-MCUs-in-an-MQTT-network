import pandas as pd
import numpy as np
import os, gc
import matplotlib.pyplot as plt
from typing import Dict, List
from scipy.signal import decimate
from scipy.ndimage import binary_closing

encryption_methods = ["masked_ASCON", "ASCON", "AES-GCM", "NONE"]
scenarios = 12
metrics = ["RTT", "S_PROC", "R_PROC", "DS_PROC", "DS_ENC", "DS_DEC","GW_US_PROC", "GW_DS_PROC", "ENC", "DEC", "HW_and_Network", "percent_of_RTT", "DS_encryption_time_of_RTT", "S_encryption_time_of_RTT","NTNU_network_time", "BLE_transmission_time"]
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
    "percent_of_RTT":    "Hardware and network time as % of RTT",
    "DS_encryption_time_of_RTT": "Data Storage Encryption and decryption time as % of RTT",
    "S_encryption_time_of_RTT": "Sensor Encryption and decryption time as % of RTT",
    "NTNU_network_time": "Gatway to data storage to gateway Time",
    "BLE_transmission_time": "Total BLE transmission Time per round-trip",
}

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


def round_to_ms(df, col):
    if col.startswith("DS_"):
        td = pd.to_timedelta(df[col].astype('int64'), unit='ns')
        # This yields a float in ms, preserving fractions
        df[col] = td / pd.Timedelta(milliseconds=1)
    else:
        td = pd.to_timedelta(df[col].astype('int64'), unit='us')
        # This yields a float in ms, preserving fractions
        df[col] = td / pd.Timedelta(milliseconds=1)

    return df


def load_and_merge_logs(folder_path):
    filenames = [
        "ENC.csv", "DEC.csv", "DS_ENC.csv", "DS_DEC.csv", "RTT.csv",
        "S_PROC.csv", "R_PROC.csv", "DS_PROC.csv", "GW_US_PROC.csv",
        "GW_DS_PROC.csv"
    ]
    merged_df = None

    for fname in filenames:
        full_path = os.path.join(folder_path, fname)
        if os.path.exists(full_path):
            df = pd.read_csv(full_path)
            df = df.drop(columns=["Seq_Num"], errors="ignore")
            prefix = fname.split(".")[0]
            if prefix in ["DS_ENC", "DS_DEC", "ENC", "DEC"] and "NONE_scen" in folder_path:
                # Skip for NONE scenarios
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

    # gw to data storage to gw
    if "GW_US_PROC_End_Time" in merged_df.columns and "GW_DS_PROC_Start_Time" in merged_df.columns:
        merged_df["NTNU_network_time_Delta"] = merged_df[
            "GW_DS_PROC_Start_Time"] - merged_df["GW_US_PROC_End_Time"]
        merged_df = round_to_ms(merged_df, "NTNU_network_time_Delta")

    # BLE transfoer time
    if "GW_US_PROC_Start_Time" in merged_df.columns and "GW_DS_PROC_End_Time" in merged_df.columns:
        merged_df["BLE_transmission_time_Delta"] = merged_df[
            "GW_DS_PROC_End_Time"] - merged_df["GW_US_PROC_Start_Time"]
        merged_df = round_to_ms(merged_df, "BLE_transmission_time_Delta")

    # Convert timestamps to deltas
    for prefix in [
            "RTT", "S_PROC", "R_PROC", "DS_PROC", "DS_ENC", "DS_DEC",
            "GW_US_PROC", "GW_DS_PROC", "ENC", "DEC"
    ]:
        start_col = f"{prefix}_Start_Time"
        end_col = f"{prefix}_End_Time"
        if start_col in merged_df.columns and end_col in merged_df.columns:
            merged_df[
                f"{prefix}_Delta"] = merged_df[end_col] - merged_df[start_col]
            merged_df.drop(columns=[start_col, end_col], inplace=True)
            merged_df = round_to_ms(merged_df, f"{prefix}_Delta")

    if "BLE_transmission_time_Delta" in merged_df.columns and "S_PROC_Delta" in merged_df.columns and "R_PROC_Delta" in merged_df.columns and "RTT_Delta" in merged_df.columns:
        merged_df["BLE_transmission_time_Delta"] = merged_df[
            "RTT_Delta"] - merged_df["BLE_transmission_time_Delta"] - merged_df[
                "S_PROC_Delta"] - merged_df["R_PROC_Delta"]

    if "NTNU_network_time_Delta" in merged_df.columns and "DS_PROC_Delta" in merged_df.columns:
        merged_df["NTNU_network_time_Delta"] -= merged_df["DS_PROC_Delta"]

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
    stats: Dict[str, List] = {"Method": [], f"Mean_{category}": [], f"Std_{category}": []}
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

def add_hw_and_networking_time(frames):
    for encryption_method in encryption_methods:
        for i in range(0, scenarios):
            try:
                frames[encryption_method][i]["HW_and_Network_Delta"] = frames[encryption_method][i]["RTT_Delta"] - frames[encryption_method][i]["S_PROC_Delta"]- frames[encryption_method][i]["R_PROC_Delta"] - frames[encryption_method][i]["GW_DS_PROC_Delta"] - frames[encryption_method][i]["GW_US_PROC_Delta"] - frames[encryption_method][i]["DS_PROC_Delta"]
                frames[encryption_method][i]["percent_of_RTT_Delta"] = frames[encryption_method][i]["HW_and_Network_Delta"] / frames[encryption_method][i]["RTT_Delta"]*100
            except Exception as e:
                if frames[encryption_method][i] is not None:
                    frames[encryption_method][i]["HW_and_Network_Delta"] = 0

def add_encyprion_time_of_RTT(frames):
    for encryption_method in encryption_methods:
        for i in range(0, scenarios):
            try:
                frames[encryption_method][i]["DS_crypto_time_Delta"] = frames[encryption_method][i]["DS_ENC_Delta"] + frames[encryption_method][i]["DS_DEC_Delta"]
                frames[encryption_method][i]["DS_encryption_time_of_RTT_Delta"] = frames[encryption_method][i]["DS_crypto_time_Delta"] / frames[encryption_method][i]["RTT_Delta"]*100
                frames[encryption_method][i]["S_crypto_time_Delta"] = frames[encryption_method][i]["ENC_Delta"] + frames[encryption_method][i]["DEC_Delta"]
                frames[encryption_method][i]["S_encryption_time_of_RTT_Delta"] = frames[encryption_method][i]["S_crypto_time_Delta"] / frames[encryption_method][i]["RTT_Delta"]*100
            except Exception as e:
                print(e)
                if frames[encryption_method][i] is not None:
                    frames[encryption_method][i]["DS_encryption_time_of_RTT_Delta"] = 0
                    frames[encryption_method][i]["S_encryption_time_of_RTT_Delta"] = 0

def get_encryption_stats(frames: dict, encryption_method: str, category= "RTT") -> pd.DataFrame:
    stats: Dict[str, List[float]] = {f"Mean_{category}": [], f"Std_{category}": []}
    for scen in range(scenarios):
        frame = frames[encryption_method][scen]
        if frame is None or f"{category}_Delta" not in frame.columns:
            stats[f"Mean_{category}"].append(0)
            stats[f"Std_{category}"].append(0)
        else:
            stats[f"Mean_{category}"].append(frame[f"{category}_Delta"].mean())
            stats[f"Std_{category}"].append(frame[f"{category}_Delta"].std())
    return pd.DataFrame(stats)

def get_means(metric, frames):
    means = []
    stds = []

    for encryption_method in encryption_methods:
        frame = get_encryption_stats(frames, encryption_method, metric)
        means.append(frame[f"Mean_{metric}"].values)
        stds.append(frame[f"Std_{metric}"].values)
    return means, stds

def store_means(metric, frames, encryption_methods, output_csv):
    """
    Build a table of mean±std for each scenario (rows) and encryption method (columns)
    for the given metric, then save it to CSV.

    Parameters:
    - metric: string, e.g. "RTT"
    - frames: dict of {encryption_method: list of DataFrames per scenario}
    - encryption_methods: list of method names in order
    - output_csv: path to write the CSV file
    """
    # Determine number of scenarios from any one entry
    scenarios = len(next(iter(frames.values())))
    index = [f"scen_{i}" for i in range(1, scenarios + 1)]
    table = pd.DataFrame(index=index, columns=encryption_methods)

    colname = f"{metric}_Delta"
    for method in encryption_methods:
        dfs = frames.get(method, [])
        for i, df in enumerate(dfs, 1):
            if df is None or colname not in df.columns:
                table.at[f"scen_{i}", method] = ""
            else:
                m = df[colname].mean()
                s = df[colname].std()
                table.at[f"scen_{i}", method] = f"{m:.3f} ± {s:.3f}"

    # Write out
    table.to_csv(output_csv)

    return table


def plot_bar_results(means, stds, metric, scenarios=12):

    scens = [f"scen_{i}" for i in range(1, scenarios + 1)]

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
    if metric == "percent_of_RTT":
        ax.set_ylabel("% of RTT")
        ax.set_ylim(90, 100)
    elif metric == "encryption_time_of_RTT":
        ax.set_ylabel("% of RTT")
    else:
        ax.set_ylabel("t [ms]")
    ax.set_title(f"Mean {LABEL_ALIASES.get(metric)} per Scenario")
    ax.set_xticks(x + width)
    ax.set_xticklabels(scens)
    ax.legend()

    plt.tight_layout()
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    return fig


def analyse_execution_times(frames, path):
    """
    Save the given figure to a file.
    """
    for metric in metrics:
        means, stds = get_means(metric, frames)
        store_means(metric, frames, encryption_methods, os.path.join(path, "results", "execution_times", f"{metric}.csv"))
        fig = plot_bar_results(means, stds, metric)
        fig.savefig(os.path.join(path,"figures", "means_bar_plot" ,metric), bbox_inches='tight')
        print(f"Saved {metric}")


def read_data(path, downsample=2):
    df = pd.read_csv(
        path,
        engine="pyarrow",
        usecols=["Value", "Timestamp"],
        dtype={"Value": "float32"},
    )
    return df


def plot_scenarios(dfs, ylim=(0.03, 0.075)):
    fig, axes = plt.subplots(3, 4, figsize=(15, 12))

    for idx, ax in enumerate(axes.flatten()):
        df = dfs[idx].iloc[::4]
        ax.plot(df['Timestamp'], df['Value'], linewidth = 0.2)
        ax.set_title(f"Scenario {idx+1}")
        ax.set_xlabel("Time (s)")
        ax.set_ylabel("Current (A)")
        if ylim is not None:
            ax.set_ylim(ylim)

    plt.tight_layout()
    plt.show()


def fast_segment(df, value_col="Value", fs=4000,
                 target_fs=100,     # decimate to 50 Hz
                 smooth_s=0.5,     # 0.5 s window @50 Hz → 25 samples
                 thresh=0.035,        # pick your threshold on the smoothed decimated signal
                 min_gap_s=0.1     # close gaps shorter than 0.1 s @50 Hz → 5 samples
                ):
    sig = df[value_col].values.astype(float)

    # 1) decimate
    q = int(fs/target_fs)
    sig_dec = decimate(sig, q, ftype="fir", zero_phase=True)

    # 2) smooth with convolution (fast)
    win = int(smooth_s * target_fs)
    kernel = np.ones(win)/win
    roll_dec = np.convolve(sig_dec, kernel, mode="same")

    # 3) threshold
    mask_dec = roll_dec > thresh

    # 4) close tiny gaps
    gap = int(min_gap_s * target_fs)
    mask_dec = binary_closing(mask_dec, structure=np.ones(gap))

    # 5) upsample mask back to fs
    mask_full = np.repeat(mask_dec, q)[:len(sig)]

    # 6) add to DataFrame
    df["state"] = np.where(mask_full, "high", "low")
    return df

def high_period_lengths(df, state_col='state', time_col='Timestamp', fs=4000):
    """
    Identify contiguous 'high' periods in a segmented DataFrame and return their lengths.
    
    Returns a DataFrame with columns:
      - start_time:      Timestamp of the first sample in the high period
      - end_time:        Timestamp of the last sample in the high period
      - length_samples:  Number of samples in the high period
      - length_s:        Duration of the high period in seconds
    """
    # Boolean mask: True when state is 'high'
    mask = df[state_col] == 'high'
    # Label runs where mask is constant
    run_id = (mask != mask.shift(fill_value=False)).cumsum()
    # Group only the high runs
    high_runs = df[mask].groupby(run_id)

    # Build the summary
    results = pd.DataFrame({
        'start_time':     high_runs[time_col].first(),
        'end_time':       high_runs[time_col].last(),
        'length_samples': high_runs.size()
    })
    # Convert sample count to seconds
    results['length_s'] = results['length_samples'] / fs

    return results.reset_index(drop=True)

def summarize_high_intervals(
    df,
    value_col='Value',
    state_col='state',
    time_col='Timestamp'
):
    """
    Summarize contiguous 'high' periods, aggregating the provided value column.
    
    Returns a DataFrame with columns:
      - start_time:   Timestamp of the first sample in the high period
      - end_time:     Timestamp of the last sample in the high period
      - duration_s:   end_time - start_time (s)
      - sum_value:    Sum of value_col over the high period
      - mean_value:   Mean of value_col over the high period
      - std_value:    Standard deviation of value_col over the high period
    """
    # Boolean mask for high state
    mask = df[state_col] == 'high'
    # Label runs where mask changes value
    run_id = (mask != mask.shift(fill_value=False)).cumsum()
    # Select only high-state rows and assign run labels
    high = df[mask].copy()
    high['run'] = run_id[mask]

    # Aggregate per run
    agg = high.groupby('run').agg(
        start_time=pd.NamedAgg(column=time_col, aggfunc='first'),
        end_time=pd.NamedAgg(column=time_col, aggfunc='last'),
        sum_value=pd.NamedAgg(column=value_col, aggfunc='sum'),
        mean_value=pd.NamedAgg(column=value_col, aggfunc='mean'),
        std_value=pd.NamedAgg(column=value_col, aggfunc='std'),
    )
    # Compute duration
    agg['duration_s'] = agg['end_time'] - agg['start_time']

    del high, mask
    gc.collect()

    return agg.reset_index(drop=True)

def calulate_energy(segmented):
    segmented["energy[mJ]"] = segmented["Value"]/4000*1e3

def summarize_high_periods(df,
                           energy_col='energy',
                           state_col='state',
                           time_col='Timestamp'):
    """
    Summarize contiguous 'high' periods in a DataFrame where the energy per sample
    is already computed in `energy_col`.

    Returns a DataFrame with columns:
      - start_time:   Timestamp of the first sample in the high period
      - end_time:     Timestamp of the last sample in the high period
      - duration_s:   end_time - start_time (s)
      - total_energy: Sum of energy_col over the period
      - sample_count: Number of samples in the period
      - avg_power_per_sample: total_energy / sample_count
    """
    mask = df[state_col] == 'high'
    # Label runs of constant mask value
    run_id = (mask != mask.shift(fill_value=False)).cumsum()
    # Filter to high runs
    high_df = df[mask].copy()
    high_df['run'] = run_id[mask]

    rows = []
    for run, grp in high_df.groupby('run'):
        start = grp[time_col].iat[0]
        end = grp[time_col].iat[-1]
        duration = end - start
        total_energy = grp[energy_col].sum()
        rows.append({
            'start_time': start,
            'end_time': end,
            'duration_s': duration,
            'total_energy': total_energy,
        })
    del high_df
    gc.collect()

    return pd.DataFrame(rows).reset_index(drop=True)

# Example usage:
# df_energy is your DataFrame with 'state' and 'energy' columns
# summary = summarize_high_periods(df_energy, energy_col='energy', state_col='state', time_col='Timestamp')
# tools.display_dataframe_to_user(name="High Period Energy Summary", dataframe=summary)


def plot_segmented(df,
                   fs=4000,
                   ylim=(0.03, 0.075),
                   xlim=None,
                   downsample=None):
    """
    Plot segmented data with optional slicing in seconds.

    Parameters:
    - df: DataFrame containing 'Timestamp', 'Value', and 'state' columns.
    - fs: Sampling frequency (default: 4000 Hz).
    - ylim: Tuple specifying y-axis limits (default: (0.03, 0.075)).
    - start: Start time in seconds for slicing (default: None).
    - end: End time in seconds for slicing (default: None).
    """
    fig, ax = plt.subplots(figsize=(20, 5))

    if xlim is not None:
        start_idx = int(xlim[0] * fs)
        end_idx = int(xlim[1] * fs)
        df = df.iloc[start_idx:end_idx]
        ax.set_xlim(xlim)

    if downsample is not None:
        plot_df = df.iloc[::downsample]  # downsample for plotting
    else:
        plot_df = df
    times = plot_df['Timestamp'].values
    vals = plot_df['Value'].values

    # build two arrays that are NaN whenever they’re not in that state
    high_vals = np.where(plot_df['state'] == 'high', vals, np.nan)
    low_vals = np.where(plot_df['state'] == 'low', vals, np.nan)

    # plot each as a continuous line but with NaNs breaking the segments
    ax.plot(times, low_vals, '-', linewidth=0.5, color='blue', label='Low')
    ax.plot(times, high_vals, '-', linewidth=0.5, color='red', label='High')

    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Power (mW)")
    ax.set_title("Power Usage Breakdown")
    if ylim is not None:
        ax.set_ylim(ylim)
    ax.legend(loc='upper right')
    ax.grid()
    ax.set_xlabel(ax.get_xlabel(), fontsize=14)
    ax.set_ylabel(ax.get_ylabel(), fontsize=14)
    ax.title.set_fontsize(16)
    plt.tight_layout()
    return fig, ax


def calculate_segments(segmented_df):
    """
    Given a df with columns:
      - 'Value' : float array of currents
      - 'state' : 'high' or 'low' labels per sample
    Compute:
      • high_mean  & std
      • low_mean   & std
      • overall meant & std
    Returns a dict of (mean, std) tuples.
    """
    # pull out the raw values
    vals = segmented_df['Value'].to_numpy()
    # rebuild exactly the same mask_full boolean array
    mask_high = segmented_df['state'].to_numpy() == 'high'
    # high and low samples
    high = vals[mask_high]
    low  = vals[~mask_high]

    return {
        'high_mean': (high.mean(),       high.std()),
        'low_mean':  (low.mean(),        low.std()),
        'mean':      (vals.mean(),       vals.std())
    }
