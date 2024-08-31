# Neural classifier/tagger

Tool to train and apply a neural model.

The intended use-case: to train a model on one part of granet-parsed pool and apply it on another.
This way set of samples that are not recognized by the current Granet grammar, but recognized by the
trained model may be obtained (as well as samples recognized by grammar, but not by the model).

These samples may be manually assessed, e.g., to find entities that are missed by the grammar.

## How to train

Firstly, create `~/.vhrc` file and add the [nirvana token](https://wiki.yandex-team.ru/jandekspoisk/nirvana/components/api)
there, e.g.:
```
echo "--oauth-token=<nirvana token>" > ~/.vhrc && chmod 400 ~/.vhrc
```

Afterwards, create train and test pools. The simplest way is following:

0. For simplicity set up `GRANET=<path to arcadia>/alice/nlu/granet`. Build Granet: `$GRANET/data/ru/test/prepare.sh`.

1. Download Granet pool with requests from logs:\
`ya m -r --checkout $GRANET/data/ru/test/pool`\
or\
`$GRANET/data/ru/test/pool/load_dataset.sh random7v3.tsv`\
for a larger pool.

2. Split it randomly:
```bash
POOL_NAME=<pool name>
cd $GRANET/data/ru/test/pool
tail -n+2 $POOL_NAME | shuf -o shuffled_pool.tsv
echo -e "weight\ttext\tform\tmock" > train_pool.tsv && echo -e "weight\ttext\tform\tmock" > test_pool.tsv
split -nl/2 shuffled_pool.tsv && rm shuffled_pool.tsv
cat xaa >> train_pool.tsv && rm xaa
cat xab >> test_pool.tsv && rm xab
FULL_TRAIN_POOL=$GRANET/data/ru/test/pool/train_pool.tsv
FULL_TEST_POOL=$GRANET/data/ru/test/pool/test_pool.tsv
```

3. Go to `$GRANET/data/ru/test/quality/main` and update `config.json`:
```json
{
  "language": "ru",
  "type": "quality",
  "cases": [
    {
      "name": "train",
      "form": "<your form name>",
      "base": "../../pool/train_pool.tsv",
      "negative_from_base_ratio": 1
    },
    {
      "name": "test",
      "form": "<your form name>",
      "base": "../../pool/test_pool.tsv",
      "negative_from_base_ratio": 1
    }
  ]
}
```
But, mind you, don't commit it.

4. Run `./test_this.sh`.

5. In `results/<timestamp>/result/tagger` you will see `train.tsv` and `test.tsv` with positive
samples according to the grammar:
```bash
TIMESTAMP=<set it>
TRAIN_POSITIVES=$GRANET/data/ru/test/quality/main/results/$TIMESTAMP/result/tagger/train.tsv
TRAIN_NEGATIVES=$GRANET/data/ru/test/quality/main/results/$TIMESTAMP/result/tagger/train_negatives.tsv
TEST_POSITIVES=$GRANET/data/ru/test/quality/main/results/$TIMESTAMP/result/tagger/test.tsv
TEST_NEGATIVES=$GRANET/data/ru/test/quality/main/results/$TIMESTAMP/result/tagger/test_negatives.tsv
```

6. Go to `$GRANET/tools/neural` and collect negative samples:
```bash
python collect_negatives.py \
    --full-dataset-path $FULL_TRAIN_POOL \
    --positives-path $TRAIN_POSITIVES \
    --output-path $TRAIN_NEGATIVES

python collect_negatives.py \
    --full-dataset-path $FULL_TEST_POOL \
    --positives-path $TEST_POSITIVES \
    --output-path $TEST_NEGATIVES
```

7. Run train/apply script:
```bash
ya m -r --checkout $GRANET/tools/neural
$GRANET/tools/neural/neural_applier main \
    --train-positives-path $TRAIN_POSITIVES \
    --train-negatives-path $TRAIN_NEGATIVES \
    --test-positives-path $TEST_POSITIVES \
    --test-negatives-path $TEST_NEGATIVES
```

It will create the nirvana graph. Wait till it finishes and check the state output node in the 
Python Deep Learning block. It will contain an archive with model (in `model/`) and predictions for
the test files (in `predictions`).
