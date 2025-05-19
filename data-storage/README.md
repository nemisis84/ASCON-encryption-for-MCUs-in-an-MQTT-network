# Data storage

This folder contains the data storage realisation. As with the sensor, this has been built for the given experiments, such that modifications is needed if this were to be put in production. 

## Install guide

The packages needed for running this project (and data analysis) Can be found in environment.yaml. This guide assumes that anaconda or miniconda is installed. First install the environment based on environment.yaml:

```
conda env create -f environment.yaml
```
Then activate the environment:

```
conda activate datastorage_env
```
If yout environment were to be updated:
```
conda env update --file environment.yaml --prune

```
## Run data storage

The data storage can be ran by running:

The data storage can be ran by running:

```
python main.py <scenario_number> <crypto_algorithm>
```
Where scenario_number is the given scenario you wants to start with. The data storage automatically increments the scenario if the sensor is running as normal. The crypto_algorithm options are: NONE. AES-GCM, masked_ASCON and ASCON and should be aligned with the sensor to have succesfull decryptions and encryptions. 

