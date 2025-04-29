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
    start_time = 46 * 60 + 23
    end_time = 1*60*60 + 2 * 60 + 53
    df7 = get_intervals(orig_df, start_time, end_time, fs=fs)

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
    start_time = 46 * 60 + 23
    end_time = 1*60*60 + 2 * 60 + 53
    df7 = get_intervals(orig_df, start_time, end_time, fs=fs)
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

def process_encryption_method(encryption_method, path, traces):
    print(f"Processing {encryption_method} traces")
    orig_df = h.read_data(path)

    dfs = traces(orig_df)
    del orig_df
    gc.collect()

    # 4) Your fixed parameters
    smooth_s  = 0.03
    thresh    = 0.037
    min_gap_s = 0.5
    
    # 5) Process one scenario at a time
    rows = []
    for i in range(len(dfs)):
        print(f"Processing scenario {i}")
        
        # segmentation in‐place on df
        segmented = h.fast_segment(
            dfs[i],
            value_col='Value',
            fs=fs,
            smooth_s=smooth_s,
            thresh=thresh,
            min_gap_s=min_gap_s
        )
        
        # compute currents (numpy‐based, no extra DataFrame copies)
        stats = h.calculate_currents(segmented)
        rows.append({
            'Scenario': f'scen_{i+1}',
            'High (mA)': stats['high_mean_current'],
            'Low (mA)':  stats['low_mean_current'],
            'All (mA)':  stats['mean_current'],
        })
        
        # drop this scenario’s data before moving on
        dfs[i] = None
        del segmented
        gc.collect()
    
    # 6) Build the final table
    table = pd.DataFrame(rows).set_index('Scenario')

    # Format each cell as “mean ± std”
    for col in ["High (mA)", "Low (mA)", "All (mA)"]:
        table[col] = table[col].apply(lambda t: f"{t[0] * 1000:.6f} ± {t[1] * 1000:.6f}")

    # 7) Write out to CSV
    out_path = os.path.join("..", "results", "avg_power_consumptions" ,f"power_consumption_{encryption_method}.csv")
    table.to_csv(out_path)
    print(f"Wrote summary to {out_path}")

    # 8) Print to console
    print(table.to_string())

if __name__ == "__main__":
    data_path = "../results/energy_consumption/"
    path = os.path.join(data_path, "ascon1-12", "Main current - Arc.csv")
    for encryption_method in ["ascon", "masked_ascon"]:
        path = os.path.join(data_path, f"{encryption_method}1-12", "Main current - Arc.csv")
        process_encryption_method("ascon", path, ascon_traces)


