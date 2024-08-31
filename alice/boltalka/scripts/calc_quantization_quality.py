import sys, argparse, codecs
import numpy as np
from itertools import izip
from collections import Counter
import yt.wrapper as yt

yt.config.set_proxy("hahn.yt.yandex.net")


def main(args):
    old_batch = []
    new_batch = []
    metric = 0
    num_batches = 0
    prev_query_id = None

    for old_row, new_row in izip(yt.read_table(args.old_top), yt.read_table(args.new_top)):
        query_id = old_row['context_id']
        old_line = old_row['reply'].decode('utf-8')
        new_line = new_row['reply'].decode('utf-8')
        if prev_query_id is not None and query_id != prev_query_id:
            metric += sum((Counter(new_batch) & Counter(old_batch[:args.k])).values()) / float(args.k)
            num_batches += 1
            new_batch = []
            old_batch = []
        old_batch.append(old_line)
        new_batch.append(new_line)
        prev_query_id = query_id

    metric += sum((Counter(new_batch) & Counter(old_batch[:args.k])).values()) / float(args.k)
    num_batches += 1

    metric /= float(num_batches)

    print '%.3f' % metric



if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--old-top', required=True)
    parser.add_argument('--new-top', required=True)
    parser.add_argument('-k', type=int, default=10)
    args = parser.parse_args()
    main(args)

