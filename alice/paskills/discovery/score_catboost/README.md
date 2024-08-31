# AliceDiscovery score CatBoost
> [Nirvana operation](https://beta.nirvana.yandex-team.ru/operation/f54f0c99-ad80-489e-b285-43334a3ac807)

## Usage
``` bash
$ alice/paskills/discovery/score_catboost/score_catboost --help
usage: score_catboost [-h] --input INPUT [--output OUTPUT]
                      [--output-roc-plot OUTPUT_ROC_PLOT] --loss-type
                      {mse,rmse,log-loss} [--dataset-name DATASET_NAME]

optional arguments:
  -h, --help            show this help message and exit
  --input INPUT
  --output OUTPUT
  --output-roc-plot OUTPUT_ROC_PLOT
  --loss-type {mse,rmse,log-loss}
  --dataset-name DATASET_NAME

```

### Input and required arguments
- __input__ - CatBoost inference results. format: `query_id\ttarget\tskill_id\t0\tprediction`
- __--loss-type__ - data to build pool, i. e. `alice/paskills/discovery/saas_query/saas_query` results

### Output arguments
- __--output__ - file to score
- __--output-roc-plot__ - file to save ROC plot

> Example in `build_pool.sh`
