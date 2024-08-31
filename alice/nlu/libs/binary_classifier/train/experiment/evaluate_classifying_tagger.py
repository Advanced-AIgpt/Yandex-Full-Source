# coding: utf-8

import argparse
import pandas as pd

from metric_utils import compute_metrics


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--predictions-path', required=True)
    parser.add_argument('--output-dir', required=True)
    parser.add_argument('--output-prefix', required=True)
    args = parser.parse_args()

    results = pd.read_csv(args.predictions_path, sep='\t')
    predictions = results['Probability'].values
    labels = results['Expected positiveness'].values

    compute_metrics(predictions, labels, args.output_dir, args.output_prefix)


if __name__ == "__main__":
    main()
