import sys
import argparse
import itertools
from alice.boltalka.extsearch.query_basesearch.lib.grequests import QueryBasesearch


def _parse_yt_input(line):
    turns = {}
    for part in line.split('\t'):
        if part.startswith('context_'):
            k, v = part.split('=', 1)
            k = int(k.split('_', 1)[1]) + 1
            turns[k] = v
    return '\n'.join([v for k, v in sorted(turns.items(), reverse=True)])


def iter_input():
    for line in sys.stdin:
        yield line.rstrip('\n')


def make_queries(lines, args):
    assert not (args.from_yt and args.use_entity), "This combination is not supported"
    for line in lines:
        if args.from_yt:
            query = _parse_yt_input(line)
        else:
            query = line.replace('\t', '\n')
            if args.use_entity:
                query = query.split('\n', 1)
        yield query


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--host', default='general-conversation-hamster.yappy.beta.yandex.ru')
    parser.add_argument('--port', type=int, default=80)
    parser.add_argument('--max-results', type=int, default=1)
    parser.add_argument('--min-ratio', type=float, default=1.0)
    parser.add_argument('--context-weight', type=float, default=1.0)
    parser.add_argument('--ranker', default='catboost')
    parser.add_argument('--extra-params', default='')
    parser.add_argument('--experiments', default='')
    parser.add_argument('--output-attr', default='reply')
    parser.add_argument('--from-yt', action='store_true')
    parser.add_argument('--max-retries', type=int, default=0)
    parser.add_argument('--pool-size', type=int, default=1)
    parser.add_argument('--non-deterministic', action='store_true')
    parser.add_argument('--use-entity', action='store_true')
    args = parser.parse_args()

    query_basesearcher = QueryBasesearch(args.host, args.port, args.experiments, args.max_results,
                                         args.min_ratio, args.context_weight, args.ranker,
                                         args.extra_params, args.pool_size, args.max_retries,
                                         not args.non_deterministic, args.output_attr)
    lines, lines2 = itertools.tee(iter_input(), 2)
    for line, replies in zip(lines, query_basesearcher.process_iterator(make_queries(lines2, args), args.use_entity)):
        for el in replies:
            if args.from_yt:
                print('\t'.join([line, 'source=' + str(el['source']), 'nlg=' + el['reply']]))
            else:
                print('\t'.join([str(el['relevance']), line, el['reply'], el['source']]))
