import argparse
import os


def read_pattern(pattern, files):
    union = set()
    for filename in filter(lambda f: pattern in f, files):
        if not filename.endswith('tsv'):
            continue
        with open(filename, 'r') as f:
            for line in f.readlines():
                union.add(line)
    return list(union)


def process(folder):
    files = [os.path.join(folder, filename) for filename in os.listdir(folder)]
    with open(os.path.join(folder, 'full@positives.tsv'), 'w') as f:
        f.writelines(read_pattern('positives', files))
    with open(os.path.join(folder, 'full@negatives.tsv'), 'w') as f:
        f.writelines(read_pattern('negatives', files))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('path')
    args = parser.parse_args()
    process(folder=args.path)


if __name__ == '__main__':
    main()
