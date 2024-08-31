#!/usr/bin/env python3

import argparse
import os
import math

from collections import defaultdict


def list_files(d):
    r = []
    for root, dirs, files in os.walk(d):
        for name in files:
            path = os.path.join(root, name)
            if path.endswith("nlu"):
                r.append(os.path.join(root, name))
    return r


def update_with_document(path):
    d = 0
    with open(path, "r") as f:
        collection = f.readlines()

    for document in collection:
        tokens = document.strip().lower().split()
        if not tokens or len(tokens[0]) > 30:
            continue
        for token in tokens:
            term_count[token] += 1
        d += 1
    return d


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-p", "--path",
        help="Path to the folder with nlu files",
        type=str,
        required=True
    )
    args = parser.parse_args()

    term_count = defaultdict(int)
    document_count = 0

    for path in list_files(args.path):
        document_count += update_with_document(path)

    idfs = {key: math.log(document_count / value) / math.log(document_count) for key, value in term_count.items()}
    idfs["не"] = 10

    with open("idfs.tsv", "w") as f:
        for key, value in idfs.items():
            if key.isalnum() and value < 1:
                f.write(key + "\t" + str(value) + "\n")
