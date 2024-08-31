# coding=utf-8
import yt.wrapper as yt
import argparse

yt.config.set_proxy("hahn.yt.yandex.net")
#yt.config["tabular_data_format"] = yt.YsonFormat(control_attributes_mode="row_fields", table_index_column="@table_index")


class Splitter(object):
    def __init__(self, num_rows, splitting_col, split_val_base, train_fraction, is_test):
        self.num_rows = num_rows
        self.splitting_col = splitting_col
        self.split_val_base = split_val_base
        self.num_train_rows = min(self.num_rows, int(self.num_rows * train_fraction))
        self.is_test = is_test

    def __call__(self, row):
        is_train_sample = int(int(row[self.splitting_col], self.split_val_base) % self.num_rows < self.num_train_rows)
        if is_train_sample != self.is_test:
            yield row

def main(args):
    num_rows = yt.row_count(args.src)
    yt.run_map(Splitter(num_rows, args.split_by, args.split_val_base, args.train_fraction, args.is_test), args.src, args.dst)
    yt.run_sort(args.dst, sort_by=[args.split_by])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--split-by', required=True)
    parser.add_argument('--split-val-base', type=int, default=16)
    parser.add_argument('--train-fraction', type=float, default=0.9)
    parser.add_argument('--is-test', action='store_true')
    args = parser.parse_args()
    main(args)
