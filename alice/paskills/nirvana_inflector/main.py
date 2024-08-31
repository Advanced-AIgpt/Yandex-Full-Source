# -*- coding: utf-8 -*-
from __future__ import print_function

import argparse
import io
import json
import random
import sys

from alice.paskills.nirvana_inflector.inflector_wrapper import inflect


def sample(arr, max_inflected_num):
    if len(arr) <= max_inflected_num:
        return arr
    else:
        return random.sample(arr, max_inflected_num)


def __inflect(record, max_inflected_num):
    text = record['text']
    inflected = inflect('', [text])
    inflected = sample(inflected, max_inflected_num)

    return json.dumps({
        'text': text,
        'original_text': record.get('original_text', text),
        'inflected': inflected,
    }, encoding='utf-8', ensure_ascii=False)


def __inflect_stream(stream, max_inflected_num):
    for line in stream:
        yield __inflect(json.loads(line), max_inflected_num)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--seed', default=0, type=int)
    parser.add_argument('--max-inflected-num', required=True, type=int)
    parser.add_argument('--workers', default=1, type=int)
    args = parser.parse_args()
    if args.seed == 0:
        # TODO: Support multiple workers
        print("[Warning] Using default seed = 0")
    if args.workers != 1:
        # TODO: Support multiple workers
        print("[Warning] multple workers are not supported yet. Ignoring flag")

    random.seed(args.seed)

    print("Seed:{}. first random:{}".format(args.seed, random.random()), file=sys.stderr)

    with io.open(args.input, mode='r', encoding='utf-8') as __input, io.open(args.output, mode='w+', encoding='utf-8') as __output:
        for line in __inflect_stream(__input, args.max_inflected_num):
            __output.write(line + "\n")
