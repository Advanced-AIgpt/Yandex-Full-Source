# coding: utf-8

import argparse
import csv
import json
import os

from intent_renamer import IntentRenamer

from dataset_utils import DatasetConfig, load_dataset


def _dump_data_for_lstm(dataset, target_intent, data_type, is_positive):
    with open('data/{}_{}.tsv'.format(data_type, 'positives' if is_positive else 'negatives'), 'w') as f:
        f.write('weight\ttext\tcan_use_to_train_tagger\n')
        for sample in dataset:
            if is_positive == (sample['intent'] == target_intent):
                f.write('1\t{}\t{}\n'.format(sample['markup'].encode('utf-8'), sample['can_use_to_train_tagger']))


def _dump_test_data(renamer, input_path, target_intent):
    input_file_name = os.path.splitext(os.path.basename(input_path))[0]
    positives_output_path = 'data/{}_positives.tsv'.format(input_file_name)
    negatives_output_path = 'data/{}_negatives.tsv'.format(input_file_name)

    with open(input_path) as f_in:
        with open(positives_output_path, 'w') as positives_out, open(negatives_output_path, 'w') as negatives_out:
            reader = csv.DictReader(f_in, dialect="excel-tab")
            for row in reader:
                intent = renamer(row['intent'], by=IntentRenamer.By.TRUE_INTENT)
                if intent == target_intent:
                    positives_out.write('1\t{}\tFalse\n'.format(row['text']))
                else:
                    negatives_out.write('1\t{}\tFalse\n'.format(row['text']))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--dataset-type', required=True)
    parser.add_argument('--dataset-config-path', required=True)
    parser.add_argument('--dataset-path')
    parser.add_argument('--renames-paths')
    args = parser.parse_args()

    with open(args.dataset_config_path) as f:
        dataset_config = DatasetConfig(**json.load(f))

    if args.dataset_type == 'train':
        train_dataset, valid_dataset = load_dataset(dataset_config)

        if not os.path.isdir('data'):
            os.makedirs('data')

        _dump_data_for_lstm(train_dataset, target_intent=dataset_config.intent, data_type='train', is_positive=True)
        _dump_data_for_lstm(train_dataset, target_intent=dataset_config.intent, data_type='train', is_positive=False)
        _dump_data_for_lstm(valid_dataset, target_intent=dataset_config.intent, data_type='valid', is_positive=True)
        _dump_data_for_lstm(valid_dataset, target_intent=dataset_config.intent, data_type='valid', is_positive=False)
    else:
        renamer = IntentRenamer(args.renames_paths.split(','))
        _dump_test_data(renamer, args.dataset_path, target_intent=dataset_config.intent)


if __name__ == "__main__":
    main()
