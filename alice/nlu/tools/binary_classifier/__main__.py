# coding: utf-8

import click
import os
import traceback

from alice.nlu.tools.binary_classifier.dataset_loader import DatasetLoader
from alice.nlu.tools.binary_classifier.dataset_prepare import (
    prepare_complex_datasets,
    print_complex_dataset_collection_stat,
    save_preprocessed_datasets
)
from alice.nlu.tools.binary_classifier.model_input import create_input_description, prepare_numpy_data
from alice.nlu.tools.binary_classifier.test_model import test_model
from alice.nlu.tools.binary_classifier.train_model import train_model
from alice.nlu.tools.binary_classifier.utils import load_json, change_working_dir


def _load_config(path):
    print('Load config "%s"' % path)
    return load_json(path)


def _learn_and_test(config_path, dataset_loader, model=None):
    config = _load_config(config_path)
    with change_working_dir(os.path.dirname(config_path)):
        datasets = prepare_complex_datasets(config, dataset_loader)
        print_complex_dataset_collection_stat(datasets)
        input_description = create_input_description(config, datasets)
        prepare_numpy_data(input_description, datasets)
        dataset_loader.stub_fetchers.save_changes()
        save_preprocessed_datasets(config, datasets)
        if model is None:
            model = train_model(config, input_description, datasets)
        test_model(model, config, datasets)
        print('Done')
        return model


def _learn_and_test_no_throw(config_path, dataset_loader, model=None):
    try:
        model = _learn_and_test(config_path, dataset_loader, model)
    except Exception:
        print(traceback.format_exc())
    return model


@click.command()
@click.option(
    '--config', type=click.Path(exists=True), required=True,
    help='Path to config. You can find examples at alice/nlu/data/binary_classifier/intents.')
def learn(config):
    dataset_loader = DatasetLoader()
    _learn_and_test(config, dataset_loader)


@click.command()
@click.option(
    '--config', type=click.Path(exists=True), required=True,
    help='Path to config. You can find examples at alice/nlu/data/binary_classifier/intents.')
@click.option(
    '--run-learn', is_flag=True,
    help='Run learning and testing on start.')
def interactive(config, run_learn):
    dataset_loader = DatasetLoader()
    model = None
    c = None
    if run_learn:
        c = 'l'
    while True:
        if c is None:
            pass
        elif c == 'l':
            model = _learn_and_test_no_throw(config, dataset_loader, None)
        elif c == 't':
            model = _learn_and_test_no_throw(config, dataset_loader, model)
        elif c == 'u':
            print('Not implemented yet. But you can do it manually:')
            print('  1. Run')
            print('     ya upload --ttl=inf --tar %s' % config['model']['model_dir'])
            print('  2. Add model to alice/nlu/data/ru/models/alice_binary_intent_classifier/data.inc')
        elif c == 'q':
            print('Quit')
            break
        else:
            print('Invalid input')
        print('What\'s next?')
        print('  l - reload config and changed datasets, learn and test model')
        print('  t - reload config and changed datasets, test model')
        print('  u - upload model')
        print('  q - quit')
        c = click.getchar()


@click.group()
def main():
    pass


main.add_command(learn)
main.add_command(interactive)

if __name__ == '__main__':
    main()
