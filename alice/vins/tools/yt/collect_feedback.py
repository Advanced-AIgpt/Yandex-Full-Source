# coding: utf-8

import os
import argparse
from collections import deque
from itertools import groupby

import yt.wrapper as yt

yt.config['proxy']['url'] = os.environ.get('YT_PROXY', 'hahn')


class SlidingWindow(object):
    def __init__(self, stream, window_size):
        self.window_size = window_size
        self._stream = stream
        self._dq = deque()

    def _put(self, item):
        while len(self._dq) >= self.window_size:
            self._dq.popleft()

        self._dq.append(item)

    def __iter__(self):
        for item in self._stream:
            self._put(item)
            yield list(self._dq)


def is_feedback(chunk):
    prev, cur = chunk[-3], chunk[-2]
    prev_form = prev['form_name']
    cur_form = cur['form_name']

    def check(fname):
        if fname is None:
            return False

        for type_ in ('feedback_negative', 'feedback_positive'):
            if type_ in fname:
                return True
        return False

    return check(cur_form) and not check(prev_form)


def get_context(uuid, records):
    res = []
    for rec in records:
        if rec['type'] == "UTTERANCE":
            msg = rec['utterance_text']
        elif rec['type'] == "CALLBACK":
            continue

        request = msg or ''
        response = rec['response']['cards'][0].get('text', '') if len(rec['response'].get('cards', [])) > 0 else ''

        if (request or response):
            res.append(request)
            res.append(response)

    return {'context': '– ' + '\n– '.join(res), 'uuid': uuid}


def merge(group_of_groups):
    for group in group_of_groups:
        for row in group:
            if row['type'] == "CALLBACK":
                continue
            yield row


@yt.reduce_aggregator
def reducer(rows):
    # group by uuid
    for uuid, group_of_groups in groupby(rows, lambda x: x[0]):
        # create one stream of records inside one uuid
        stream = merge(i[1] for i in group_of_groups)

        # create chunks with sliding window
        for chunk in SlidingWindow(stream, window_size=7):
            if len(chunk) < 3:
                continue

            if is_feedback(chunk):
                yield get_context(uuid['uuid'], chunk)


def get_tables(dir_, starts_from):
    return [os.path.join(dir_, i) for i in yt.list(dir_) if str(i) >= starts_from]


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument(
        '-d', '--destination-table',
        help="Destination table",
    )

    parser.add_argument(
        '--source-dir',
        help="Directory with source tables",
        default='//home/voice/vins/logs/dialogs',
    )

    parser.add_argument(
        '--start-from',
        help=("From which table start to collect feedback"),
        default='',
    )

    args = parser.parse_args()
    tables = get_tables(args.source_dir, args.start_from)

    with yt.Transaction():
        yt.run_map_reduce(
            None, reducer,
            source_table=tables,
            destination_table=args.destination_table,
            reduce_by=['uuid'],
            sort_by=['uuid', 'server_time_ms'],
        )


if __name__ == '__main__':
    main()
