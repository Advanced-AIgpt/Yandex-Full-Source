## Train submodule

Various training helpers.

### Collecting dataset
To collect a dataset for scenarios classifier run
```
VINS_NUM_PROCS=32 OMP_NUM_THREADS=1 CUDA_VISIBLE_DEVICES=-1 python tools/train/dataset_building.py --build-scenarios-dataset --result-path <path to dataset> --feature-cache <path to feature cache> 
```

To collect a dataset from tsv (e.g. toloka dataset) run
```
VINS_NUM_PROCS=32 OMP_NUM_THREADS=1 CUDA_VISIBLE_DEVICES=-1 python tools/train/dataset_building.py --source-data-path <path to .tsv> --source-data-type tsv --target-classifier <classifier name, e.g. 'toloka'> --convert-to-dataset --result-path <path to dataset> --feature-cache <path to feature cache> 
```

Add param `--features-from-tagger` to collect features required by tagger

## Training classifier
To train scenarios classifier run
```
VINS_NUM_PROCS=32 OMP_NUM_THREADS=1 CUDA_VISIBLE_DEVICES=<index> python tools/train/train_scenarios_classifier.py --train-dataset-path <path to dataset collected on previous step> --train-metric
```

## Training taggers
To train taggers run
```
VINS_NUM_PROCS=32 OMP_NUM_THREADS=1 CUDA_VISIBLE_DEVICES=<index> python tools/train/train_taggers.py --train-dataset-path <path to dataset collected on previous step> --config tools/train/configs/rnn_tagger.json
```

You must provide `--config` parameter with path to config of one of supported tagger trainers. Available configs located in `configs` subdirectory.

You can specify taggers to retrain with regexp `--intents-to-train`.

To delete all previous trained taggers from archive, use `--from-scratch`.
