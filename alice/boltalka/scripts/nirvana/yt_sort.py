#!/usr/bin/python
import argparse
import yt.wrapper as yt
yt.config.set_proxy("hahn.yt.yandex.net")

def main(args):
    yt.run_sort(args.src, args.dst, sort_by=args.sort_by.split(','))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--sort-by', required=True)
    args = parser.parse_args()
    main(args)
