import pandas as pd
import numpy as np
import os
import gc
import matplotlib.pyplot as plt
from typing import Dict, List
from scipy.signal import decimate
from scipy.ndimage import binary_closing

encryption_methods = ["NONE",  "AES-GCM", "ASCON", "masked_ASCON"]
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
    """
    Load and merge log files from the specified folder. Also add columns for
    the time deltas between start and end times for each log type and calulates the NTNU_network_time_Delta
    and BLE_transmission_time_Delta.
    The function also removes invalid rows based on certain criteria.
    """

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
            if prefix in ["DS_ENC", "DS_DEC", "ENC", "DEC"
                          ] and "NONE_scen" in folder_path:
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


def add_hw_and_networking_time(frames):
    """
    Calculate the hardware and networking time as a value and as a percentage of the RTT delta.
    """
    for encryption_method in encryption_methods:
        for i in range(0, scenarios):
            try:
                frames[encryption_method][i][
                    "HW_and_Network_Delta"] = frames[encryption_method][i][
                        "RTT_Delta"] - frames[encryption_method][i][
                            "S_PROC_Delta"] - frames[encryption_method][i][
                                "R_PROC_Delta"] - frames[encryption_method][i][
                                    "GW_DS_PROC_Delta"] - frames[
                                        encryption_method][i][
                                            "GW_US_PROC_Delta"] - frames[
                                                encryption_method][i][
                                                    "DS_PROC_Delta"]
                frames[encryption_method][i]["percent_of_RTT_Delta"] = frames[
                    encryption_method][i]["HW_and_Network_Delta"] / frames[
                        encryption_method][i]["RTT_Delta"] * 100
            except Exception as e:
                if frames[encryption_method][i] is not None:
                    frames[encryption_method][i]["HW_and_Network_Delta"] = 0


def add_encyprion_time_of_RTT(frames):
    """
    Calculate the encryption and decryption times as a percentage of the RTT delta.
    """
    for encryption_method in encryption_methods:
        for i in range(0, scenarios):
            try:
                frames[encryption_method][i]["DS_crypto_time_Delta"] = frames[
                    encryption_method][i]["DS_ENC_Delta"] + frames[
                        encryption_method][i]["DS_DEC_Delta"]
                frames[encryption_method][i][
                    "DS_encryption_time_of_RTT_Delta"] = frames[
                        encryption_method][i]["DS_crypto_time_Delta"] / frames[
                            encryption_method][i]["RTT_Delta"] * 100
                frames[encryption_method][i]["S_crypto_time_Delta"] = frames[
                    encryption_method][i]["ENC_Delta"] + frames[
                        encryption_method][i]["DEC_Delta"]
                frames[encryption_method][i][
                    "S_encryption_time_of_RTT_Delta"] = frames[
                        encryption_method][i]["S_crypto_time_Delta"] / frames[
                            encryption_method][i]["RTT_Delta"] * 100
            except Exception as e:
                print(encryption_method)
                if frames[encryption_method][i] is not None:
                    frames[encryption_method][i][
                        "DS_encryption_time_of_RTT_Delta"] = 0
                    frames[encryption_method][i][
                        "S_encryption_time_of_RTT_Delta"] = 0


def get_encryption_stats(frames: dict,
                         encryption_method: str,
                         category="RTT") -> pd.DataFrame:
    """
    Get the mean and std of the given category for each scenario.
    """
    stats: Dict[str, List[float]] = {
        f"Mean_{category}": [],
        f"Std_{category}": []
    }
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
    """
    Get the means and stds for the given metric for each encryption method.
    """
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
    table = table.reindex(
            columns=["NONE", "AES-GCM", "ASCON", "masked_ASCON"])
    table.to_csv(output_csv)

    return table


def plot_bar_results(means, stds, metric, scenarios=12):
    """
    Plot the means and stds for the given metric for each encryption method.
    """
    scens = [f"Scen {i}" for i in range(1, scenarios + 1)]

    x = np.arange(scenarios)  # positions for groups
    width = 1 / (len(encryption_methods) + 1)  # width of each bar

    # Create plot
    fig, ax = plt.subplots(figsize=(10, 6))

    # Plot each method
    for i in range(len(encryption_methods)):
        label = "Masked ASCON" if encryption_methods[i] == "masked_ASCON" else encryption_methods[i]

        ax.bar(x + i * width,
               means[i],
               width,
               yerr=stds[i],
               label=label,
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
        store_means(metric, frames, encryption_methods, os.path.join(path, "execution_times", f"{metric}.csv"))
        fig = plot_bar_results(means, stds, metric)
        fig.savefig(os.path.join(path,"figures", "means_bar_plot" ,metric), bbox_inches='tight')
        print(f"Saved {metric}")


def read_data(path):
    """
    Read the data from the given path
    """
    df = pd.read_csv(
        path,
        engine="pyarrow",
        usecols=["Value", "Timestamp"],
        dtype={"Value": "float32"},
    )
    return df


def mark_initial_high_window(df, state_col='state',
                             fs=4000,
                             window_ms=30,
                             new_col='is_initial_high_30ms'):
    """
    Given df[state_col] with 'high'/'low', mark True for the first `window_ms` 
    of each high period.
    """
    mask = df[state_col] == 'high'
    # identify runs
    run_id = (mask != mask.shift(fill_value=False)).cumsum()

    result = pd.Series(False, index=df.index, name=new_col)

    # how many samples in window_ms?
    n_win = int(window_ms * fs / 1000)

    # only iterate runs that actually are 'high'
    for run in np.unique(run_id[mask]):
        idxs = np.nonzero(run_id == run)[0]
        # pick the first n_win samples of this run
        head = idxs[:n_win]
        result.iloc[head] = True

    return result


def fast_segment(df,
                 value_col="Value",
                 fs=4000,
                 target_fs=100,
                 smooth_s=0.5,
                 thresh=0.035,
                 min_gap_s=0.1,
                 early_window_ms=30):
    """
    Segment the data into 'high' and 'low' states based on a threshold.
    Parameters:
    - df: DataFrame with 'Value' and 'Timestamp' columns
    - value_col: column name for the value to segment
    - fs: original sampling frequency
    - target_fs: target sampling frequency for decimation
    - smooth_s: smoothing window in seconds
    - thresh: threshold for high/low segmentation
    - min_gap_s: minimum gap in seconds to close
    - early_window_ms: window in milliseconds to mark initial high period
    Output:
    - df: DataFrame with additional columns 'state' and 'is_initial_high'
    """

    sig = df[value_col].values.astype(float)

    # 1) decimate (needed to not kill memory)
    q = max(1, int(fs / target_fs))
    sig_dec = decimate(sig, q, ftype="fir", zero_phase=True)

    # 2) smooth
    win = max(1, int(smooth_s * target_fs))
    kernel = np.ones(win) / win
    roll_dec = np.convolve(sig_dec, kernel, mode="same")

    # 3) threshold + 4) close gaps
    mask_dec = roll_dec > thresh
    gap = max(1, int(min_gap_s * target_fs))
    mask_dec = binary_closing(mask_dec, structure=np.ones(gap))

    # 5) upsample back
    mask_full = np.repeat(mask_dec, q)[:len(sig)]

    # 6) assign states
    df["state"] = np.where(mask_full, "high", "low")

    # 7) mark the first early_window_ms ms of each high period
    df["is_initial_high"] = mark_initial_high_window(df,
                                                     state_col="state",
                                                     fs=fs,
                                                     window_ms=early_window_ms)

    return df


def summarize_high_intervals(
    df,
    value_col='Value',
    state_col='state',
    time_col='Timestamp',
    init_col='is_initial_high',
    early_window_ms=30,
):
    """
    Summarize the high intervals in the given DataFrame.
    Parameters:
    - df: DataFrame with 'Value', 'state', 'Timestamp', and 'is_initial_high' columns
    - value_col: column name for the value to summarize
    - state_col: column name for the state ('high' or 'low')
    - time_col: column name for the timestamp
    - init_col: column name for the initial high period
    - early_window_ms: window in milliseconds to mark initial high period
    Output:
    - agg: DataFrame with summarized high intervals
    """
    # 1) Mask & run-id for high periods
    mask = df[state_col] == 'high'
    run_id = (mask != mask.shift(fill_value=False)).cumsum()

    # 2) Extract the high periods
    high = df.loc[mask, [time_col, value_col, init_col]].copy()
    high['run'] = run_id[mask]

    # 3) Aggregate the full-period stats
    agg = high.groupby('run').agg(
        start_time=pd.NamedAgg(column=time_col, aggfunc='first'),
        end_time=pd.NamedAgg(column=time_col, aggfunc='last'),
        sum_value=pd.NamedAgg(column=value_col, aggfunc='sum'),
        mean_value=pd.NamedAgg(column=value_col, aggfunc='mean'),
        std_value=pd.NamedAgg(column=value_col, aggfunc='std'),
    )

    # 4) Compute the initial_Xms_sum and std
    init = high[high[init_col]].groupby('run')[value_col]
    init_sum = init.sum().rename(f'initial_{early_window_ms}ms_sum')
    init_std = init.std().rename(f'finitial_{early_window_ms}ms_std')
    agg = agg.join(init_sum).join(init_std).fillna({
        f'initial_{early_window_ms}ms_sum':
        0.0,
        f'initial_{early_window_ms}ms_std':
        0.0
    })

    # 5) Duration
    agg['duration_s'] = agg['end_time'] - agg['start_time']

    # clean up
    del high, mask
    gc.collect()

    return agg.reset_index(drop=True)



def calulate_energy(segmented):
    """
    Calculate the energy consumption in mJ based on the segmented data.
    """
    segmented["energy[mJ]"] = segmented["Value"] / 4000 * 1e3



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
    - downsample: Downsampling factor (default: None).
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
    Calculate the mean and std of the high and low segments in the given DataFrame.
    Parameters:
    - segmented_df: DataFrame with 'Value' and 'state' columns
    Output:
    - Dictionary with mean and std for high, low, and overall segments
    """
    # pull out the raw values
    vals = segmented_df['Value'].to_numpy()
    # rebuild exactly the same mask_full boolean array
    mask_high = segmented_df['state'].to_numpy() == 'high'
    # high and low samples
    high = vals[mask_high]
    low = vals[~mask_high]

    return {
        'high_mean': (high.mean(), high.std()),
        'low_mean': (low.mean(), low.std()),
        'mean': (vals.mean(), vals.std())
    }


def plot_energy_results(means, stds, x_label, y_label, title, scenarios=(1,12)):
    """
    Plot the means and stds for the given metric for each encryption method.
    """
    scens = [f"Scen {i}" for i in range(scenarios[0], scenarios[1] + 1)]

    x = np.arange(scenarios[1]-scenarios[0]+1)  # positions for groups
    width = 1 / (len(encryption_methods) + 1)  # width of each bar

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
    ax.set_xlabel(x_label)
    ax.set_ylabel(y_label)
    ax.set_title(title)
    ax.set_xticks(x + width)
    ax.set_xticklabels(scens)
    ax.legend()

    plt.tight_layout()
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    return fig, ax