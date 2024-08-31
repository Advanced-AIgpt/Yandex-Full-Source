# coding: utf-8

import argparse
import json
import logging

import numpy as np

import alice.nlu.tools.lstm_classifier_trainer.core.data as data
import yandex.type_info.typing as ti
import yt.wrapper as yt
from alice.nlu.tools.lstm_classifier_trainer.core.model import Model, ModelConfig

logger = logging.getLogger(__name__)


def get_output_column(dataset_config):
    return dataset_config.label_column + '_prediction'


def get_output_schema(dataset_config, model_config):
    schema = yt.schema.TableSchema.from_yson_type(
        yt.get_attribute(dataset_config.table_to_predict, 'schema')
    )

    output_column = get_output_column(dataset_config)

    if any(column.name == output_column for column in schema.columns):
        raise ValueError('{} is already in table'.format(output_column))

    if model_config.trainer.mode == data.ModelMode.BINARY:
        output_column_type = ti.Float
    else:
        output_column_type = ti.Optional[ti.Yson]

    schema.add_column(output_column, output_column_type)

    return schema


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--config-path', default='../config.json')
    parser.add_argument('--label-encode-path', default='')
    parser.add_argument('--model-dir', default='../models')
    parser.add_argument('--embeddings-path', default='../embeddings/ru')
    parser.add_argument('--allow-high-unknown-token-ratio', action='store_true')
    args = parser.parse_args()

    data.setup_yt()

    with open(args.config_path, 'r') as f:
        json_config = json.load(f)

    dataset_config = data.DatasetConfig.load(json_config['dataset'])
    model_config = ModelConfig.load(json_config['model'])

    if dataset_config.table_to_predict is None:
        # TODO(vl-trifonov) make separate dataset config for prediction
        raise ValueError("table_to_predict is not provided in config")

    if dataset_config.output_table is None:
        # TODO(vl-trifonov) make separate dataset config for prediction
        raise ValueError("output_table is not provided in config")

    output_column = get_output_column(dataset_config)
    output_schema = get_output_schema(dataset_config, model_config)

    if not yt.exists(dataset_config.output_table):
        yt.create('table', dataset_config.output_table, recursive=True)

    yt.alter_table(dataset_config.output_table, output_schema)

    if args.label_encode_path:
        label_mapper = np.empty(max(model_config.decoder.class_count, 2), dtype=object)
        for idx, line in enumerate(open(args.label_encode_path, 'r')):
            sample, str_key, idx_key = json.loads(line), None, None
            for key in sample:
                if isinstance(sample[key], str) or isinstance(sample[key], unicode):
                    str_key = key
                else:
                    idx_key = key
            label_mapper[sample[idx_key] if idx_key else idx] = sample[str_key]

    logger.info('Dataset config: %s\nModel config: %s', dataset_config, model_config)

    logger.info('Loading embeddings...')
    embeddings_matrix, _, word_to_index = data.load_embeddings(model_config.special_tokens, args.embeddings_path)

    model = Model(Model.ModelState.VAL, model_config, embeddings_matrix=embeddings_matrix)
    model.restore(args.model_dir, base_name='model.ckpt')

    logger.info('Reading data...')
    test_examples = data.read_data(
        input_table=dataset_config.table_to_predict,
        dataset_config=dataset_config,
        model_config=model_config,
        word_to_index=word_to_index,
        allow_high_unknown_token_ratio=args.allow_high_unknown_token_ratio,
        is_train=False
    )

    test_dataset = data.BatchGenerator(
        test_examples,
        batch_size=model_config.predict.batch_size,
        seed=model_config.seed,
    )

    logger.info("Running model prediction...")

    rows, predictions = model.predict(test_dataset)
    for row, prediction in zip(rows, predictions):
        if isinstance(prediction, np.ndarray):
            if args.label_encode_path:
                prediction = {label_mapper[i]: float(pred) for i, pred in enumerate(prediction)}
            else:
                prediction = list(map(float, prediction))
        else:
            prediction = float(prediction)

        row[output_column] = prediction

    logger.info("Writing table...")
    yt.write_table(dataset_config.output_table, rows)


if __name__ == '__main__':
    main()
