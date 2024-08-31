# coding: utf-8

import argparse
import json
import logging
import os

import alice.nlu.tools.lstm_classifier_trainer.core.data as data
from alice.nlu.tools.lstm_classifier_trainer.core.model import Trainer, ModelConfig

logger = logging.getLogger(__name__)


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--config-path', default='../config.json')
    parser.add_argument('--embeddings-path', default='../embeddings/ru')
    parser.add_argument('--output-path', default='../models')
    parser.add_argument('--allow-high-unknown-token-ratio', action='store_true', default=False)

    args = parser.parse_args()

    data.setup_yt()

    with open(args.config_path) as f:
        json_config = json.load(f)

    dataset_config = data.DatasetConfig.load(json_config['dataset'])
    model_config = ModelConfig.load(json_config['model'])

    logger.info('Dataset config: %s\nModel config: %s', dataset_config, model_config)

    logger.info('Loading embeddings...')
    embeddings_matrix, _, word_to_index = data.load_embeddings(model_config.special_tokens, args.embeddings_path)

    logger.info('Reading data...')
    train_examples = data.read_data(
        input_table=dataset_config.train_table,
        dataset_config=dataset_config,
        model_config=model_config,
        word_to_index=word_to_index,
        allow_high_unknown_token_ratio=args.allow_high_unknown_token_ratio
    )

    valid_examples = data.read_data(
        input_table=dataset_config.valid_table,
        dataset_config=dataset_config,
        model_config=model_config,
        word_to_index=word_to_index,
        allow_high_unknown_token_ratio=args.allow_high_unknown_token_ratio
    )

    if dataset_config.encode_labels:
        label_to_index = data.encode_labels(train_examples, valid_examples)
        with open(os.path.join(args.output_path, 'label_to_index.json'), 'w') as f:
            json.dump(label_to_index, f, indent=2, ensure_ascii=False)

        logger.info('Class count: %s', len(label_to_index))
        model_config.decoder.class_count = len(label_to_index)

    logger.info('Train data size: %s, valid data size: %s', len(train_examples), len(valid_examples))

    logger.info('Examples:')
    for example in train_examples[0: 1000: 50]:
        logger.info(example)

    train_dataset = data.BatchGenerator(
        train_examples,
        batch_size=model_config.trainer.batch_size,
        sqrt_weight_normalizer=model_config.trainer.sqrt_weight_normalizer,
        seed=model_config.seed,
        shuffle_dataset=model_config.trainer.shuffle_dataset,
        use_weights=model_config.trainer.use_weights,
    )

    valid_dataset = data.BatchGenerator(
        valid_examples,
        batch_size=model_config.trainer.batch_size,
        sqrt_weight_normalizer=model_config.trainer.sqrt_weight_normalizer,
        seed=model_config.seed,
        shuffle_dataset=model_config.trainer.shuffle_dataset,
        use_weights=model_config.trainer.use_weights,
    )

    logger.info('Batch: %s', next(train_dataset))

    trainer = Trainer(config=model_config, embeddings_matrix=embeddings_matrix)
    trainer.fit(train_dataset, valid_dataset)

    trainer.save(args.output_path)


if __name__ == '__main__':
    main()
