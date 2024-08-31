# coding=utf-8
import argparse
import yt.wrapper as yt
import re
from alice.boltalka.scripts.twitter_dataset_build.watson.constants import EMPTY_TOKEN

yt.config.set_proxy("hahn.yt.yandex.net")


class FilterWatsonSpecificityMapper(object):
    def __init__(self, columns):
        self.columns = columns

    def __call__(self, row):
        for k in self.columns:
            if row[k] and (row[k] == EMPTY_TOKEN or 'pic.twitter.com/' in row[k]):
                return
        yield row


def main(args):
    columns = args.columns.split(',')
    yt.run_map(FilterWatsonSpecificityMapper(columns), args.src, args.dst)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--columns', required=True, help='comma separated names')
    args = parser.parse_args()
    main(args)
