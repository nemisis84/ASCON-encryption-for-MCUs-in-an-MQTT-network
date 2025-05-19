
# Data analysis

This folder contains the programs ran to analyse the data and make the plots for the master thesis. This is the files in the folder:

- Analysis.ipynb --> contains the ipynb files which makes illustratons. 
- run_power_consumptions --> analyses the energy consumption. The file has a unique function for each encryption method which must include the timestamps to separate scenarios from continous power traces.
- helper.py --> helper functions for the other files. 
- power_traces.ipynb --> Plotting of power traces.
- plot_energy.ipynb --> Plotting of the energy consumption during different intervals.
- plot_code_size --> Plots the code size for the encryption libraries. 
- execution_times/ --> Tables showing the mean and std of different execution times
- energy_consumptions/ --> Power traces from the power measurments. 
- avg_power_consumptions/ --> Mean and std of power and energy measurments
- figures/ --> Contains all figures used in the thesis. 

### Install guide

The same environment as in data-storage is used.  Follow the install guide in [data-storage README](../data-storage/README.md) to install and activate environment. 


### Slice dataset
Neat command that can filther for thr the first X entries in the power traces. Too large power traces migth be unfeasable for your computer to handle. This was used when power measurements were done without supervisions, thus leading to measurements after the experiements were completed. 
```
head -n 129600000 'Main power - Arc.csv' | sponge 'Main power - Arc.csv'
```