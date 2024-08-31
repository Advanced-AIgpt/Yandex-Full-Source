import argparse
import os
from alice.nlu.tools.ar_fst_entities.common import utils


def apply_fst(args):
    fst = utils.load_fst(args.path)
    for query in args.input:
        print("--------------------------------------------------------", file=args.output)
        print(query, file=args.output)
        matches = utils.get_top_matches(query, fst)
        for match in matches:
            print(match, file=args.output)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Applies FST model on input file with queries",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("-p", "--path", help="Path to get FST model from", required=True)
    parser.add_argument(
        "-i",
        "--input",
        help="Path to file with queries",
        required=True,
        type=argparse.FileType("r"))
    parser.add_argument(
        "-o",
        "--output",
        help="Path, where to output applying results",
        required=True,
        type=argparse.FileType("w"))
    args = parser.parse_args()
    if not os.path.exists(args.path):
        parser.error("Path to model {} does not exist".format(args.path))
    return args


def main():
    apply_fst(parse_args())


if __name__ == "__main__":
    main()
