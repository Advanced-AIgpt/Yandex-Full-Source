# coding: utf-8

import argparse


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--full-dataset-path', required=True)
    parser.add_argument('--positives-path', required=True)
    parser.add_argument('--output-path', required=True)
    args = parser.parse_args()

    with open(args.positives_path) as f:
        f.readline()
        positives = {line.strip().split('\t')[1] for line in f}

    with open(args.full_dataset_path) as f_in, open(args.output_path, 'w') as f_out:
        f_in.readline()
        for line in f_in:
            weight, text = line.strip().split('\t')[:2]
            if text not in positives:
                f_out.write('{}\t{}\n'.format(weight, text))


if __name__ == "__main__":
    main()
