# Simple Word-level Classifier Trainer

Trains a simple LSTM-based model on word embeddings.

## Train Parameters

Check the [example config](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/tools/lstm_classifier_trainer/config.json) to learn about possible parameters.

Basically, you have to specify following parameters there:
```json
{
    "dataset": {
        "train_table": <path to yt table with the train data>,
        "valid_table": <path to yt table with the valid data>,
        "text_column": <text column name in the specified tables>,
        "label_column": <label column, should contain either integers in range {0, ..., class_count - 1} (in case of multiclass classification) or integers/floats in range [0, 1] (in case of binary classification)>,
        "max_tokens": <used to filter queries by tokens count>,
        "table_to_predict": <path to yt table with data to predict on>,
        "output_table": <path to output yt table with predicted values>
    },
    "model": {
        ...
        "decoder": {
            ...
            "class_count": <number of classes, use 1 for binary classification>
        },
        ...
        "special_tokens": <list of additional tokens which embeddings should be trained, e.g., ["[SEP]"]>
    }
}
```

**Please note**: if you use embeddings from `embeddings/ru` texts are expected to be normalized!
Use [yt-normalizer](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/tools/normalizer) or this [nirvana op](https://nirvana.yandex-team.ru/operation/ed8d598c-1b0e-43c7-aa6f-a0151413a704) to perform the normalization.
Pay attention to your special tokens, normalizer will remove all punctuation symbols and lowercase the texts! For instance, you can replace `[SEP]` to `spp` before the normalization and swap them back after it.

There is an assert on the number of unknown tokens (it should not be more than 1% of all tokens). If it's too large, you probably use non-normalized texts. If you are sure that this is not the case, you can pass `--allow-high-unknown-token-ratio` arg to the train script.

## Training

You can train the model locally or on Nirvana.

### Local Training

Firstly, run `ya m --checkout embeddings/ru` to download russian embeddings (btw you can use arabic embeddings).

Run the script (or arcadia binary with the following params):
`--config-path` - path to your model's config (`config.json` by default);
`--output-path` - path to the result model's dir (`models` by default);
`--embeddings-path` - path to embeddings (`embeddings/ru` by default);

(Btw, it supports early stopping by ctrl+C)

1. You can use your own python. You will need `tensorflow[-gpu]==1.10`. The full list of the required dependencies:
```
Package                            Version
---------------------------------- -------------------
attrs                              19.3.0
numpy                              1.16.6
scikit-learn                       0.20.3  // Optional
tensorflow-gpu                     1.10.0
vins-models-tf                     0.5.2   // Optional
yandex-yt                          0.9.35
```
(Only the tf version is mandatory though, other libraries are not known for so drastically breaking changes).

2. You can use arcadia python w/ tf.
Run `ya m -r --yt-store` (add `-DTENSORFLOW_WITH_CUDA=1` to use gpu).

### Nirvana Training
Check this [wiki page](https://wiki.yandex-team.ru/alice/megamind/quality/obuchenie-klassifikatorov-dlja-begemot/obuchenie-lstm-klassifikatorov-dlja-begemot/).

## Result Model
After you successfully train your model, you will have following files:
- `model.pb` - model saved in the protobuf format. Can be loaded with [TTfNnModel](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/libs/tf_nn_model/tf_nn_model.h?rev=6615836#L23). It's better to use this format in Begemot.
- `model.mmap` - model saved in the memmaped format. Can be loaded with [TTfNnModel](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/libs/tf_nn_model/tf_nn_model.h?rev=6615836#L24) too.
- `model_description` - model nodes info. Can be used with `model.mmap` in [TEncoder](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/libs/encoder/encoder.h?rev=5132651#L50).
- `model_description.json` - model nodes info in the json format. Check [this class](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/libs/anaphora_resolver/matchers/lstm/lstm.h?rev=5698848#L24) to understand its use case.
- `trainable_embeddings.json` - embeddings for the special_tokens in the json format.

Checkout the [inference example](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/tools/lstm_classifier_trainer/cpp_inference_example) to learn how to use it. Check also the [AliceWordNN](https://a.yandex-team.ru/arc/trunk/arcadia/search/begemot/rules/alice_word_nn) Begemot rule. Please note that this is *not* the best way to apply the model in Begemot, because you have to access the file system to obtain the directory path...
