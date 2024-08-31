#!/usr/bin/python
# coding=utf-8
import os
import sys
import argparse
import yt.wrapper as yt
from copy import copy
yt.config.set_proxy("hahn.yt.yandex.net")


GC_INTENT_NAME = 'personal_assistant\tgeneral_conversation\tgeneral_conversation'
GC_SKILL_ID = 'bd7c3799-5947-41d0-b3d3-4a35de977111'


class Devices_filter(object):
    def __init__(self, devices):
        self.devices = set(devices)
    def __call__(self, row):
        if row['app'] in self.devices:
            yield row


def good(text):
    if not text:
        return False
    text = text.decode("utf-8")
    return text and len(text) >= 20 and text[0].isupper() and not text.isupper()

class Dialogue_builder(object):
    def __init__(self, context_length):
        self.context_length = context_length
        self.columns = ['context_' + str(context_length - i - 1) for i in xrange(context_length)] + ['reply']

    def _is_gc_reply(self, turn):
        return turn['intent'] == GC_INTENT_NAME or \
                    (turn.get('gc_intent') == GC_INTENT_NAME and \
                        turn['intent'] != 'personal_assistant\tscenarios\texternal_skill_gc')

    def _get_feedback_on_reply(self, turn):
        intent = turn['intent']
        if intent.startswith('personal_assistant\tfeedback\tfeedback_positive'):
            return 1
        if intent.startswith('personal_assistant\tfeedback\tfeedback_negative'):
            return 0
        return None

    def __call__(self, row):
        res = {}
        res['session_id'] = row['session_id']

        queue = [None] * (self.context_length + 1)

        for turn_idx, turn in enumerate(row['session']):
            queue = queue[2:] + [turn['_query'], turn['_reply']]
            if self._is_gc_reply(turn):
                for column, utterance in zip(self.columns, queue):
                    res[column] = utterance
                if turn_idx == len(row['session']) - 1:
                    res['feedback'] = None
                else:
                    res['feedback'] = self._get_feedback_on_reply(row['session'][turn_idx + 1])
                res['ts'] = turn['ts']
                if all([good(el) for el in queue[::2]]):
                    yield res


def main(args):
    srcs = []
    lower_bound = os.path.join(args.src, args.from_date)
    upper_bound = os.path.join(args.src, args.to_date)
    for table in yt.list(args.src, absolute=True):
        if lower_bound <= table <= upper_bound:
            srcs.append(table)
    yt.run_map(Devices_filter(args.devices.split(',')), srcs, args.dst)
    yt.run_map(Dialogue_builder(args.context_length), args.dst, args.dst)
    yt.run_sort(args.dst, sort_by=['session_id', 'ts'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', default='//home/voice/dialog/sessions')
    parser.add_argument('--from-date', required=True)
    parser.add_argument('--to-date', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--context-length', type=int, default=3)
    parser.add_argument('--devices', type=str, default='search_app_prod')
    args = parser.parse_args()
    main(args)
