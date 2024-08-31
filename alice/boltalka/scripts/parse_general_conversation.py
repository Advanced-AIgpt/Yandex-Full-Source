#!/usr/bin/python
# coding=utf-8
import os
import sys
import regex
import codecs
import argparse
import yt.wrapper as yt
yt.config.set_proxy("hahn.yt.yandex.net")

LOGS_PREFIX = '//home/voice/vins/logs/dialogs'

def yield_utterance(row):
    if row['type'] != 'UTTERANCE':
        return
    if row['request']['app_info']['app_id'] not in ('ru.yandex.searchplugin.beta', 'ru.yandex.searchplugin'):
        return
    context = row['utterance_text'].strip()
    if len(context) > 1000:
        return
    yield {
        'uuid': row['uuid'],
        'time': row['server_time'],
        'form_name': row['form_name'],
        'source': row['utterance_source'],
        'context': context,
        'reply': row['response']['cards'][0]['text'],
        'frequency': 1,
    }

def aggregate_sessions(key, rows, max_context_turns):
    context = [(None, '')] * max_context_turns
    for row in rows:
        context.pop(0)
        context.append((row['source'], row['context']))

        if row['form_name'].endswith('general_conversation'):
            has_suggested = sum(1 for source, _ in context if source == 'suggested') > 0
            if not has_suggested:
                res = { 'context_' + str(i): context[-i - 1][1] for i in xrange(len(context)) }
                res['reply'] = row['reply']
                res['frequency'] = row['frequency']
                res['inv_freq'] = 2**64 - res['frequency']
                yield res

        context.pop(0)
        context.append((None, row['reply']))

def calc_frequency(key, rows):
    frequency = 0
    for row in rows:
        frequency += row['frequency']
    res = dict(key)
    res.update({'frequency': frequency})
    yield res

def join_frequency(key, rows):
    rows = list(rows)

    frequency = 0
    for row in rows:
        if row['@table_index'] == 1:
            assert frequency == 0
            frequency = row['frequency']

    for row in rows:
        if row['@table_index'] == 0:
            assert frequency != 0
            row['frequency'] = frequency
            yield row

def main(args):
    srcs = []
    lower_bound = LOGS_PREFIX + '/' + args.from_date
    upper_bound = LOGS_PREFIX + '/' + args.to_date
    for table in yt.list(LOGS_PREFIX, absolute=True):
        if lower_bound <= table <= upper_bound:
            srcs.append(table)

    yt.run_map(yield_utterance, srcs, args.dst)
    yt.run_sort(args.dst, args.dst, sort_by=['context'])

    freq_table = args.dst + '.frequency'
    yt.run_reduce(calc_frequency, args.dst, freq_table, reduce_by=['context'])
    yt.run_sort(freq_table, freq_table, sort_by=['context'])

    yt.run_join_reduce(join_frequency, [args.dst, yt.TablePath(freq_table, attributes={'foreign': True})], args.dst, join_by=['context'])

    yt.run_sort(args.dst, args.dst, sort_by=['uuid', 'time'])
    reducer = lambda key, rows: aggregate_sessions(key, rows, args.max_context_turns)
    yt.run_reduce(reducer, args.dst, args.dst, sort_by=['uuid', 'time'], reduce_by=['uuid'])

    yt.run_sort(args.dst, args.dst, sort_by=['inv_freq', 'context_0'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--from-date', default='2017-06-02')
    parser.add_argument('--to-date', default='2017-06-13')
    parser.add_argument('--dst', required=True)
    parser.add_argument('--max-context-turns', type=int, default=3)
    args = parser.parse_args()

    main(args)
