# coding: utf-8

import argparse
import logging
import os
import yt.wrapper as yt

from alice.nlu.py_libs import tagger as m_tagger
from alice.nlu.tools.tagger.lib.data import load_data_from_disk, load_data_from_yt

yt.config['proxy']['url'] = os.environ.get('YT_PROXY', 'hahn')
logger = logging.getLogger(__name__)


class Validator(object):
    def __init__(self, data, early_stopping_eps, early_stopping_steps, model_dir):
        self._data = data
        self._early_stopping_eps = early_stopping_eps
        self._early_stopping_steps = early_stopping_steps
        self._bad_steps = 0
        self._best_accuracy = 0.0
        self._model_dir = model_dir

    def __call__(self, tagger_trainer):
        if not self._data:
            tagger_trainer.save(self._model_dir)
            return True

        matches = 0
        tagger = tagger_trainer.convert_to_applier()
        for sample_features in self._data:
            tag_list_list, _ = tagger.predict([sample_features], nbest=1)
            if tag_list_list[0][0] == sample_features.sample.tags:
                matches += 1
        accuracy = 1.0 * matches / len(self._data)
        logger.info('Validation accuracy (over samples): {:.4f}'.format(accuracy))

        if accuracy >= self._best_accuracy + self._early_stopping_eps:
            logger.info('Validation accuracy improved: {:.4f} > {:.4f}'.format(
                accuracy, self._best_accuracy))
            tagger_trainer.save(self._model_dir)
            self._best_accuracy = accuracy
            self._bad_steps = 0
        else:
            self._bad_steps += 1
            logger.info('No validation improvement for {} steps, current best = {:.4f}'.format(
                self._bad_steps, self._best_accuracy))

        if self._early_stopping_steps > 0 and self._bad_steps >= self._early_stopping_steps:
            logger.info('Early stopping triggered by validation improvement absence')
            return False

        return True


def _train(
    intent_to_data, epoch_count, batch_size, learning_rate, model_dir,
    batch_count_to_validate, validation_intent_to_data,
    early_stopping_eps, early_stopping_steps, rnn_encoder_dim
):
    trainer = m_tagger.RnnTaggerTrainer(epoch_count, batch_count_to_validate, batch_size, learning_rate, rnn_encoder_dim)

    for intent, intent_data in intent_to_data.items():
        logger.info('Training %s tagger', intent)
        validation_data = validation_intent_to_data.get(intent, [])
        on_validation = Validator(validation_data, early_stopping_eps, early_stopping_steps, os.path.join(model_dir, intent))
        trainer.fit(
            X=intent_data, y=None,
            on_validation=on_validation
        )
        on_validation(trainer)


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    train_group = parser.add_mutually_exclusive_group(required=True)
    train_group.add_argument('-t', '--train-data-path', help='Path to file with training data')
    train_group.add_argument('-T', '--train-data-table', help='Path to YT table with begemot results with training data')
    validation_group = parser.add_mutually_exclusive_group()
    validation_group.add_argument('-v', '--validation-data-path', help='Path to file with validation data')
    validation_group.add_argument('-V', '--validation-data-table', help='Path to YT table with begemot results with validation data')
    parser.add_argument('-m', '--model-dir', required=True)
    parser.add_argument('-n', '--epoch-count', default=20, type=int, help='Number of training epochs')
    parser.add_argument('--batch-size', default=256, type=int)
    parser.add_argument('-b', '--batch-count-to-validate', metavar='N_BATCHES', default=100, type=int,
                        help='Run validation each N_BATCHES batches')
    parser.add_argument('-e', '--early-stopping-eps', metavar='EPS', default=1e-4, type=float,
                        help='Stop training when accuracy decreases by EPS last N_STEPS validation steps (disabled when N_STEPS=0)')
    parser.add_argument('-s', '--early-stopping-steps', metavar='N_STEPS', default=0, type=int,
                        help='Stop when training accuracy decreases by EPS last N_STEPS validation steps (disabled when N_STEPS=0)')
    parser.add_argument('--learning-rate', default=1e-3, type=float)
    parser.add_argument('--rnn-encoder-dim', default=128, type=int)

    args = parser.parse_args()

    if args.train_data_table:
        logger.info('Loading data from YT')
        train_data = load_data_from_yt(args.train_data_table)
    elif args.train_data_path:
        logger.info('Loading data from disk')
        train_data = load_data_from_disk(args.train_data_path)
    else:
        raise ValueError

    if args.validation_data_table:
        validation_data = load_data_from_yt(args.validation_data_table)
    elif args.validation_data_path:
        validation_data = load_data_from_disk(args.validation_data_path)
    else:
        validation_data = {}

    entry_count = sum(len(data) for intent, data in train_data.items())
    logging.info('{0} entries for {1} intents read'.format(entry_count, len(train_data)))

    # TODO(vl-trifonov): need some mechanism to patch default training config instead of single parameter forwarding and argparse defaults
    _train(
        train_data, args.epoch_count, args.batch_size, args.learning_rate, args.model_dir,
        args.batch_count_to_validate, validation_data,
        args.early_stopping_eps, args.early_stopping_steps, args.rnn_encoder_dim
    )
    logger.info('Done.')

    return 0

if __name__ == '__main__':
    main()
