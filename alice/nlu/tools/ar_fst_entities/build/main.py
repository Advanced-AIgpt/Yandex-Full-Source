import argparse
import os
from alice.nlu.tools.ar_fst_entities.common import fsts


class Arguments:
    name = []
    path = []
    dictionaries = ""


def parse_args():
    parser = argparse.ArgumentParser(
        description="Builds FST models",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        "-e",
        "--entity",
        action="append",
        help="Name of FST to create and path to save FST model to in the form name:path",
        required=True)
    parser.add_argument("-d", "--dictionaries", help="Path to directory with dictionaries", required=True)
    args = parser.parse_args()
    args.name = []
    args.path = []
    build_args = Arguments()
    build_args.dictionaries = args.dictionaries
    allowed_names = ["number", "time", "date", "selection", "float", "datetime", "datetime_range", "weekdays"]
    for entity in args.entity:
        name, path = entity.split(":")
        build_args.name.append(name)
        build_args.path.append(path)
        if not os.path.exists(os.path.dirname(path)):
            parser.error("Path to model {} does not exist".format(path))
        if name not in allowed_names:
            parser.error("Allowed FST names are [{}]".format(",".join(allowed_names)))
    if not os.path.exists(args.dictionaries):
        parser.error("Path to dictionaries {} does not exist".format(args.dictionaries))
    return build_args


def build_fst(args):
    for name, path in zip(args.name, args.path):
        fst = fsts.create_fst_object(name, args.dictionaries, path)
        fst.create_model()
        fst.save_model()


def main():
    build_fst(parse_args())


if __name__ == "__main__":
    main()
