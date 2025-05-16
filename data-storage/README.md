```
conda env create -f environment.yaml
```
```
conda activate datastorage_env
```
```
conda env update --file environment.yaml --prune

```
```
head -n 129600000 'Main power - Arc.csv' | sponge 'Main power - Arc.csv'
```