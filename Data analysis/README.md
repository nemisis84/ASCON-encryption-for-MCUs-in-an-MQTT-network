
# Data analysis

This file contains the programs ran to analyse the data and make the plots for the master thesis. Analysis contains the ipynb files which makes illustratons. Furthermore, it contains run_power_consumptions which analyse the energy consumption. The file has a unique function for each encryption method which must include the timestamps to separate scenarios from continous power traces. Excact precision is not important, as the traces are later separated into high and low periods. helper.py include helper functions for the other files. Figures for mean execution times and energy consumption can be found in figures/. Tables with the analysed data from the power consumption analysis can be found in avg_power_consumptions/. 

Neat command that can filter for the first X entries in the power traces. Too large power traces might be unfeasible for your computer to handle. 
```
head -n 129600000 'Main power - Arc.csv' | sponge 'Main power - Arc.csv'
```