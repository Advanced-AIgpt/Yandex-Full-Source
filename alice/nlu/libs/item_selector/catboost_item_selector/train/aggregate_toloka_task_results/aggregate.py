#!/usr/bin/env python3

import argparse
import json

from collections import Counter
from copy import deepcopy
from itertools import groupby


def aggregate(data):
    def generator(data):
        key = lambda row: row["taskId"]
        for task_id, group in groupby(sorted(data, key=key), key):
            group = list(group)
            votes = [row["outputValues"]["result"] for row in group]
            votes_count = Counter([int(item) if item != "nomatch" else "nomatch" for vote in votes for item in vote])
            task = group[0]["inputValues"]
            for item in task["items"]:
                item_row = deepcopy(task)
                del item_row["items"]
                item_row.update(item)
                item_row["task_id"] = task_id
                item_row["score"] = votes_count.get(item["value"], 0) / len(votes)
                if item_row["value"] != "nomatch":
                    yield item_row

    return list(generator(data))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Aggregate toloka results')
    parser.add_argument("-i", "--input", type=str, required=True)
    parser.add_argument("-o", "--output", type=str, required=True)
    args = parser.parse_args()

    with open(args.input, 'r') as f:
        data = json.load(f)

    with open(args.output, 'w') as f:
        json.dump(aggregate(data), f)
