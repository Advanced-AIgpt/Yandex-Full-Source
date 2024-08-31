#!/usr/bin/python
# coding=utf-8
import os
import sys
import codecs
import argparse
import yt.wrapper as yt
from copy import copy
yt.config.set_proxy("hahn.yt.yandex.net")


GC_SKILL_ID = 'bd7c3799-5947-41d0-b3d3-4a35de977111'


class Context_aggregator(object):
    def __init__(self, num_context_turns):
        self.num_context_turns = num_context_turns
        self.prev_row_cols = ['uuid', 'client_time', 'session_seq',
                              'response_meta', 'overriden_form']

    def _condition_on_reply(self, row):
        cond = row and \
               row['form_name'].endswith('general_conversation') and \
               row['response_meta'] not in ('error', 'form_restored') and \
               row['session_seq'] != 0
        return cond

    def __call__(self, key, rows):
        turn_queue = [''] * (self.num_context_turns + 1)
        prev_row = None
        prev_timedelta = None

        for row in rows:
            if self._condition_on_reply(prev_row):
                res = {'context_' + str(i): turn_queue[-i - 2] for i in xrange(len(turn_queue)-1)}
                res['reply'] = turn_queue[-1]
                if res['context_0'] != '' and res['reply'] != '':
                    res['feedback'] = row['feedback']
                    res['reply_reaction_time'] = row['client_time'] - prev_row['client_time']
                    res['context_1_reaction_time'] = prev_timedelta
                    res.update({col: prev_row[col] for col in self.prev_row_cols})
                    yield res

            turn_queue = turn_queue[2:] + [row['context'], row['reply']]
            if prev_row is None:
                prev_timedelta = None
            else:
                prev_timedelta = row['client_time'] - prev_row['client_time']
            prev_row = copy(row)


        if self._condition_on_reply(prev_row):
            res = {'context_' + str(i): turn_queue[-i - 2] for i in xrange(len(turn_queue)-1)}
            res['reply'] = turn_queue[-1]
            if res['context_0'] != '' and res['reply'] != '':
                res['feedback'] = row['feedback']
                res['reply_reaction_time'] = None
                res['context_1_reaction_time'] = prev_timedelta
                res.update({col: prev_row[col] for col in self.prev_row_cols})
                yield res


def main(args):
    yt.run_sort(args.src, args.dst, sort_by=['uuid', 'client_time'])
    yt.run_reduce(Context_aggregator(args.num_context_turns), args.dst, args.dst,
                  sort_by=['uuid', 'client_time'], reduce_by=['uuid'])
    yt.run_sort(args.dst, args.dst, sort_by=['uuid', 'client_time'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--num-context-turns', type=int, default=3)
    args = parser.parse_args()
    main(args)
