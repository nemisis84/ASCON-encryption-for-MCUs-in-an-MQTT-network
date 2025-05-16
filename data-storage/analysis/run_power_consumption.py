import os
import gc
import helpers as h
import pandas as pd

fs = 4000

def get_intervals(df, start_time, end_time, fs=4000):
    start_time = int(start_time * fs)
    end_time = int(end_time * fs)
    return df.iloc[start_time:end_time].reset_index(drop=True)

def ascon_traces(orig_df):

    # Scen 1
    start_time = 8
    end_time = 1*60 + 49
    df1 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # scen 2
    start_time = 2 * 60 + 55
    end_time = 4 * 60 + 35
    df2 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 3
    start_time = 5 * 60 + 40
    end_time = 7 * 60 + 20
    df3 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 4
    start_time = 8 * 60 + 26
    end_time = 10 * 60 + 6
    df4 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 5
    start_time = 11 * 60 + 11
    end_time = 27 * 60 + 41
    df5 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 6
    start_time = 28 * 60 + 47
    end_time = 45 * 60 + 17
    df6 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 7
    df7 = h.read_data(os.path.join("..", "results", "energy_consumption", "ascon7", "Main power - Arc.csv"))

    start_time = 2 * 60 + 59.5
    end_time = 0*60*60 + 19 * 60 + 30.5
    df7 = get_intervals(df7, start_time, end_time, fs=fs)

    # Scen 8
    start_time = 1*60*60 + 4 * 60 + 0
    end_time = 1*60*60 + 20 * 60 + 30
    df8 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 9
    start_time = 81 * 60 + 36
    end_time = 3*60*60 + 0 * 60 + 36
    df9 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 10
    start_time = 3*60*60 + 1 * 60 + 47
    end_time = 4*60*60 + 40 * 60 + 46
    df10 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 11
    start_time = 4*60*60 + 41 * 60 + 57
    end_time = 6*60*60 + 20 * 60 + 56
    df11 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 12
    start_time = 6*60*60 + 22 * 60 + 6.5
    end_time = 8*60*60 + 1 * 60 + 6
    df12 = get_intervals(orig_df, start_time, end_time, fs=fs)

    return [df1, df2, df3, df4,
       df5, df6, df7, df8,
       df9, df10, df11, df12]


def masked_ascon_traces(orig_df):
    # Scen 1
    path = os.path.join("..", "results", "energy_consumption", "masked_ascon1_and_8", "Main power - Arc.csv")
    df1_8 = h.read_data(path)
    start_time = 12
    end_time = 1 * 60 + 52
    df1 = get_intervals(df1_8, start_time, end_time, fs=fs)

    # scen 2
    start_time = 2 * 60 + 57
    end_time = 4 * 60 + 37
    df2 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 3
    start_time = 5 * 60 + 42.5
    end_time = 7 * 60 + 23
    df3 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 4
    start_time = 8 * 60 + 28.5
    end_time = 10 * 60 + 9
    df4 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 5
    start_time = 11 * 60 + 14
    end_time = 27 * 60 + 44
    df5 = get_intervals(orig_df, start_time, end_time, fs=fs)
    # Scen 6
    start_time = 28 * 60 + 50
    end_time = 45 * 60 + 21
    df6 = get_intervals(orig_df, start_time, end_time, fs=fs)
    # Scen 7
    start_time = 46 * 60 + 27
    end_time = 1 * 60 * 60 + 2 * 60 + 58
    df7 = get_intervals(orig_df, start_time, end_time, fs=fs)
    # Scen 8
    start_time = 0 * 60 * 60 + 2 * 60 + 57
    end_time = 0 * 60 * 60 + 19 * 60 + 27
    df8 = get_intervals(df1_8, start_time, end_time, fs=fs)
    # Scen 9
    start_time = 81 * 60 + 41
    end_time = 3 * 60 * 60 + 0 * 60 + 41
    df9 = get_intervals(orig_df, start_time, end_time, fs=fs)
    # Scen 10
    start_time = 3 * 60 * 60 + 1 * 60 + 51
    end_time = 4 * 60 * 60 + 40 * 60 + 51
    df10 = get_intervals(orig_df, start_time, end_time, fs=fs)
    # Scen 11
    start_time = 4 * 60 * 60 + 42 * 60 + 1
    end_time = 6 * 60 * 60 + 21 * 60 + 1
    df11 = get_intervals(orig_df, start_time, end_time, fs=fs)
    # Scen 12
    start_time = 6 * 60 * 60 + 22 * 60 + 11
    end_time = 8 * 60 * 60 + 1 * 60 + 11
    df12 = get_intervals(orig_df, start_time, end_time, fs=fs)

    return [df1, df2, df3, df4, df5, df6, df7, df8, df9, df10, df11, df12]


def aes_gcm_traces(orig_df):
    # Scen 1
    start_time = 14
    end_time = 1*60 + 54
    df1 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # scen 2
    start_time = 3 * 60 + 0
    end_time = 4 * 60 + 40
    df2 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 3
    start_time = 5 * 60 + 45
    end_time = 7 * 60 + 25
    df3 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 4
    start_time = 8 * 60 + 30
    end_time = 10 * 60 + 10
    df4 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 5
    start_time = 11 * 60 + 16
    end_time = 27 * 60 + 46
    df5 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 6
    start_time = 28 * 60 + 52
    end_time = 45 * 60 + 22
    df6 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 7
    start_time = 46 * 60 + 29
    end_time = 1*60*60 + 2 * 60 + 59
    df7 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 8
    start_time = 1*60*60 + 4 * 60 + 5
    end_time = 1*60*60 + 20 * 60 + 35
    df8 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 9
    start_time = 81 * 60 + 41
    end_time = 3*60*60 + 0 * 60 + 42
    df9 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 10
    start_time = 3*60*60 + 1 * 60 + 52
    end_time = 4*60*60 + 40 * 60 + 51
    df10 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 11
    start_time = 4*60*60 + 42 * 60 + 2
    end_time = 6*60*60 + 21 * 60 + 1
    df11 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 12
    start_time = 6*60*60 + 22 * 60 + 11.8
    end_time = 8*60*60 + 1 * 60 + 11
    df12 = get_intervals(orig_df, start_time, end_time, fs=fs)

    return [df1, df2, df3, df4,
       df5, df6, df7, df8,
       df9, df10, df11, df12]

def none_traces(orig_df):

    # Scen 1
    start_time = 14
    end_time = 1 * 60 + 54
    df1 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # scen 2
    start_time = 2 * 60 + 59
    end_time = 4 * 60 + 39
    df2 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 3
    start_time = 5 * 60 + 44
    end_time = 7 * 60 + 24
    df3 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 4
    start_time = 8 * 60 + 29
    end_time = 10 * 60 + 9
    df4 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 5
    start_time = 11 * 60 + 15
    end_time = 27 * 60 + 45
    df5 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 6
    start_time = 28 * 60 + 50
    end_time = 45 * 60 + 20
    df6 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 7
    start_time = 46 * 60 + 25
    end_time = 1 * 60 * 60 + 2 * 60 + 55
    df7 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 8
    start_time = 1 * 60 * 60 + 4 * 60 + 2.5
    end_time = 1 * 60 * 60 + 20 * 60 + 32.5
    df8 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 9
    start_time = 1 * 60 * 60 + 21 * 60 + 39
    end_time = 3 * 60 * 60 + 0 * 60 + 38
    df9 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 10
    start_time = 3 * 60 * 60 + 1 * 60 + 48
    end_time = 4 * 60 * 60 + 40 * 60 + 48
    df10 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 11
    start_time = 4 * 60 * 60 + 41 * 60 + 58
    end_time = 6 * 60 * 60 + 20 * 60 + 58
    df11 = get_intervals(orig_df, start_time, end_time, fs=fs)

    # Scen 12
    df12 = h.read_data(os.path.join("..", "results", "energy_consumption", "none12", "Main power - Arc.csv"))
    start_time = 0 * 60 * 60 + 2 * 60 + 57
    end_time = 1 * 60 * 60 + 41 * 60 + 56
    df12 = get_intervals(df12, start_time, end_time, fs=fs)

    return [df1, df2, df3, df4, df5, df6, df7, df8, df9, df10, df11, df12]

def mask_initial_scenarios_inplace(
    df: pd.DataFrame,
    exclude_cols: list = ["All periods power [mW]"],
    num_scenarios: int = 4,
    scenario_prefix: str = "scen_",
    na_value: str = "Not applicable"
) -> None:

    # 1) Build list of scenario index names to mask
    rows_to_mask = [f"{scenario_prefix}{i}" for i in range(1, num_scenarios+1)]
    # 2) Determine columns *to* mask (all minus the excluded ones)
    cols_to_mask = [c for c in df.columns if c not in exclude_cols]
    # 3) Assign
    df.loc[rows_to_mask, cols_to_mask] = na_value



def process_encryption_method(encryption_method, path, traces):
    print(f"Processing {encryption_method} traces")
    orig_df = h.read_data(path)

    dfs = traces(orig_df)
    del orig_df
    gc.collect()

    # 4) Your fixed parameters
    smooth_s = 0.03
    thresh = 0.120
    min_gap_s = 0.3
    target_fs = 2000
    early_window_ms = 35
    # 5) Process one scenario at a time
    rows = []
    for i in range(len(dfs)):
        print(f"Processing scenario {i+1} with df length {len(dfs[i])}")
        # segmentation in‐place on df
        segmented = h.fast_segment(dfs[i],
                                   value_col='Value',
                                   fs=fs,
                                   target_fs=target_fs,
                                   smooth_s=smooth_s,
                                   thresh=thresh,
                                   min_gap_s=min_gap_s,
                                   early_window_ms=early_window_ms)


        h.calulate_energy(segmented) # Watt to energy
        # compute currents (numpy‐based, no extra DataFrame copies)
        stats = h.calculate_segments(segmented) #Segment the data into highs and lows


        high_periods = h.summarize_high_intervals(segmented,
                                                value_col='energy[mJ]',
                                                state_col='state',
                                                time_col='Timestamp',
                                                early_window_ms=early_window_ms) # Calculate the high energy consumption periods

        mean_high_energy_consumption = high_periods["sum_value"].mean() # Mean energy usage per high period
        std_high_energy_consumption = high_periods["sum_value"].std() #std energy usage per high period

        early_window_sum = high_periods[f"initial_{early_window_ms}ms_sum"].mean() # Mean energy usage in the first early_window_ms
        early_window_std = high_periods[f"initial_{early_window_ms}ms_sum"].std() #std energy usage in the first early_window_ms

        duration_means = high_periods["duration_s"].mean() # Duation of high energy consumption periods
        duration_stds = high_periods["duration_s"].std() #std duration of high energy consumption periods

        rows.append({
            'Scenario':
            f'scen_{i+1}',
            'High periods [W]':
            stats['high_mean'],
            'Low periods [W]':
            stats['low_mean'],
            'All periods [W]':
            stats['mean'],
            'High period energy consumption [mJ]':
            (mean_high_energy_consumption, std_high_energy_consumption),
            'High durations [s]': (duration_means, duration_stds),
            f'First {early_window_ms}ms [mJ]': (early_window_sum, early_window_std),
        })

        # drop this scenario’s data before moving on
        dfs[i] = None
        del segmented, stats, high_periods
        gc.collect()

    # 6) Build the final table
    table = pd.DataFrame(rows).set_index('Scenario')

    # Format each cell as “mean ± std”
    for col in ['High periods [W]', 'Low periods [W]', 'All periods [W]']:
        table[col[:-3]+"power [mW]"] = table[col].apply(
            lambda t: f"{t[0] * 1000:.3f} ± {t[1] * 1000:.3f}")
        table.drop(columns=[col], inplace=True)

    table["High period energy consumption [mJ]"] = table[
        "High period energy consumption [mJ]"].apply(
            lambda t: f"{t[0]:.3f} ± {t[1]:.3f}")

    table["High durations [s]"] = table["High durations [s]"].apply(
            lambda t: f"{t[0]:.2f} ± {t[1]:.2f}")

    table[f"First {early_window_ms}ms [mJ]"] = table[f"First {early_window_ms}ms [mJ]"].apply(
            lambda t: f"{t[0]:.3f} ± {t[1]:.3f}")

    mask_initial_scenarios_inplace(table,
        exclude_cols=["All periods power [mW]"],
        num_scenarios=4,
        scenario_prefix="scen_"
    )
    # 7) Write out to CSV
    out_path = os.path.join("..", "results", "avg_power_consumptions",
                            f"power_consumption_{encryption_method}_{early_window_ms}.csv")
    table.to_csv(out_path)
    print(f"Wrote summary to {out_path}")

    # 8) Print to console
    print(table.to_string())


if __name__ == "__main__":
    data_path = "../results/energy_consumption/"

    encryption_methods = {
        "NONE": (none_traces, "none1-11"),
        "AES-GCM": (aes_gcm_traces, "aes-gcm1-12"),
        "ASCON": (ascon_traces, "ascon1-6:8-12"),
        # "masked_ASCON": (masked_ascon_traces, "masked-ascon2:7-9:12")
    }

    for encryption_method, (traces, path) in encryption_methods.items():
        path = os.path.join(data_path, path, "Main power - Arc.csv")
        process_encryption_method(encryption_method, path, traces)
