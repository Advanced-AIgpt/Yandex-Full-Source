# coding: utf-8

import argparse
import csv
import yt.wrapper as yt

from intent_renamer import IntentRenamer


def _collect_data(renamer, input_path):
    with open(input_path) as f:
        reader = csv.DictReader(f, dialect="excel-tab")
        for row in reader:
            yield {
                'intent': renamer(row['intent'], by=IntentRenamer.By.TRUE_INTENT),
                'utterance_text': row['text']
            }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input-path', required=True)
    parser.add_argument('--table-path', required=True)
    parser.add_argument('--renames-paths', required=True)
    args = parser.parse_args()

    renamer = IntentRenamer(args.renames_paths.split(','))

    yt.write_table(args.table_path, _collect_data(renamer, args.input_path))


if __name__ == "__main__":
    main()
