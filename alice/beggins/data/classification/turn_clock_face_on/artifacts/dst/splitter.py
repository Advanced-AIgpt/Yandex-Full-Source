import argparse
import random


def process(filename, train_share=0.9):
    with open(filename, 'r') as f:
        lines = f.readlines()
    random.shuffle(lines)
    train_size = int(train_share * len(lines))
    with open(f'{filename}@train.split', 'w') as f:
        f.writelines(lines[:train_size])
    with open(f'{filename}@test.split', 'w') as f:
        f.writelines(lines[train_size:])



def main():
    random.seed(42)
    parser = argparse.ArgumentParser()
    parser.add_argument('path')
    args = parser.parse_args()
    process(filename=args.path)


if __name__ == '__main__':
    main()
