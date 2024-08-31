#!/usr/bin/env python
# encoding: utf-8
from operator import itemgetter
import json


def measure(assignments, step=0.005):
    time_list = sorted(map(itemgetter('submitTs') ,assignments))
    prev_ts = time_list[0]
    for num in xrange(1, int(1 / step) + 1):
        idx = int(len(time_list) * num * step - 1)
        cur_ts = time_list[idx]
        yield (cur_ts - prev_ts) / 1000  # ms -> s
        prev_ts = cur_ts



if __name__ == '__main__':
    with open('tmp/slow_assignments.json') as inp:
        for t in measure(json.load(inp)):
            print '%.1f' % (t / 60.0)
