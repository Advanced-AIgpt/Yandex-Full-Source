import argparse
from typing import List, Optional
from tqdm import tqdm
import random

import numpy as np

import yt.wrapper as yt


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--in-table", required=True, type=str, help="YT table path to traverse.")
    parser.add_argument("--out-table", required=True, type=str, help="YT table path to traverse.")
    args = parser.parse_args()

    rows = []
    for i, row in tqdm(enumerate(yt.read_table(args.in_table))):
        rows.append({
            'embedding': row['embedding'],
            'text': row['text'],
            'doc_id': i
        })

    random.shuffle(rows)
    yt.write_table(args.out_table, rows, format="json")
