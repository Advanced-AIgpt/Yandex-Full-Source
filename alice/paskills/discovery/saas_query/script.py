from concurrent.futures import ThreadPoolExecutor
from copy import deepcopy
from itertools import islice
import argparse
import json
import requests
import sys
import urllib
import yt.wrapper

CHANNELS = [
    'aliceSkill', 'organizationChat'
]

INPUT_FORMATS = [
    'json', 'plain', 'stable', 'tsv', 'wizqbundle', 'toloka'
]

ACTIVATION_PHRASES = [
    "открой навык ", "открой диалог ", "запусти навык ",
    "запусти диалог ", "активируй навык ", "включи навык ",
    "вызови навык ", "открой чат ", "запусти чат с "
]


def extract_result(response):
    result = []
    if 'results' not in response:
        return result
    for group in response["results"][0]["groups"]:
        for doc in group['documents']:
            result.append({'url': doc['url'], 'relevance': doc['relevance']})
            if '_JSONFactors' in doc['properties']:
                result[-1]['factors'] = doc['properties']['_JSONFactors'][0]
    return result


def __is_query_good(query, args):
    tokens = query.strip().split(' ')
    return (
            args.min_tokens_num <= len(tokens) <= args.max_tokens_num and
            args.min_query_length <= len(query) <= args.max_query_length
    )


def _create_session(retries_num):
    retries = requests.packages.urllib3.util.retry.Retry(
        total=retries_num,
        connect=retries_num,
        backoff_factor=0.1,
        status_forcelist=[500, 502, 503, 504])
    session = requests.Session()
    session.verify = False
    session.mount('http://', requests.adapters.HTTPAdapter(max_retries=retries))

    return session


def shoot(args):
    params = {
        **args.params,
        'format': 'json',
        'kps': args.kps,
        'text': args.query['query'],
        'timeout': args.timeout,
        'service': args.service_name
    }

    if args.concrete_url:
        params['text'] += ' url:{}'.format(args.query['skill_id'])

    if args.softness:
        params['text'] += ' softness:{}'.format(args.softness)

    if args.mau:
        params['text'] += ' i_mau:{}'.format(args.mau)

    url = 'http://{}:17000/?'.format(args.proxy)
    if __is_query_good(args.query['query'], args):
        with _create_session(args.retries_num) as session:
            response = session.get(
                url,
                params=params,
            )
            url_full = response.url
            response_json = response.json()
            response_json_node = response_json['response']
            found_skills = extract_result(response_json_node)
            searcher_properties = response_json_node[
                'searcher_properties'] if 'searcher_properties' in response_json_node else None
            if args.verbose > 2:
                print('#', url_full, file=sys.stderr)
    else:
        found_skills = []
        url_full = None
        searcher_properties = None
        if args.verbose > 1:
            print('Skipped: ', args.query['query'], file=sys.stderr)

    result = {
        'query': args.query,
        'concrete_url': args.concrete_url,
        'url_full': url_full,
        'response': found_skills,
        'searcher_properties': searcher_properties
    }
    return result


def gen_chunks(iterable, size):
    iterator = iter(iterable)
    for first in iterator:  # stops when iterator is depleted
        def chunk():  # construct generator for next chunk
            yield first  # yield element from for loop
            for more in islice(iterator, size - 1):
                yield more  # yield more elements from the iterator

        yield chunk()  # in outer generator, yield next chunk


def _query_saas_by_chunks(queries, args, chunk_size):
    if chunk_size == -1:
        if args.verbose > 0:
            print('chunk_size is -1. chunk partionining is ignored',
                  file=sys.stderr)
        chunks = [queries]
    else:
        chunks = gen_chunks(queries, chunk_size)

    def map_params(query):
        n_params = deepcopy(args)
        n_params.query = query
        return n_params

    with ThreadPoolExecutor(max_workers=args.num_workers) as executor:
        for i, chunk in enumerate(chunks):
            if args.verbose > 0:
                print('chunk #{} of size {} executing.'.format(i, chunk_size), file=sys.stderr)
            params = map(map_params, chunk)
            for result in executor.map(shoot, params):
                yield result


def _read_queries_json(stream):
    for line in stream:
        yield json.loads(line)


def _read_queries_plain(stream):
    for line in stream:
        if not line.strip():
            continue
        yield {'query': line.strip()}


def _read_queries_tsv(stream):
    for line in stream:
        sp = line.strip().split('\t')
        if len(sp) == 2:
            if not sp[1].strip():
                continue
            yield {'query': sp[1], 'task_descr': sp[0]}
        elif len(sp) == 3:
            if not sp[2].strip():
                continue
            yield {'query': sp[2], 'task_descr': sp[1], 'skill_id': sp[0]}


def _read_wizqbundle(stream, filter_the_same=True):
    for line in stream:
        d = json.loads(line)
        original_query = d['query'].split('\t')[0]
        original_query_lower = original_query.lower()
        for _, request in enumerate(d['stats'][1]):
            for sub_request in request[2].split(' ^ '):
                if filter_the_same and sub_request == original_query_lower:
                    continue
                yield {'query': sub_request, 'original_query': original_query}


def _read_toloka(stream, filter_positive=True):
    for line in stream:
        d = json.loads(line)
        query = d['query']
        is_positive = d['answer'] == 'YES' or d['golden'] == 'YES'
        if not is_positive and filter_positive:
            continue
        yield {
            'query': query,
            'skill_id': d['skill_id'],
            'is_positive': is_positive
        }


def _read_queries_stable(stream, add_prefix, channel):
    for line in stream:
        d = json.loads(line)
        if d['channel'] != channel:
            continue
        if d['isBanned']:
            continue
        if d['hideInStore']:
            continue
        if d['deletedAt']:
            continue
        if not d['onAir']:
            continue

        data = {'skill_id': d['id'], 'slug': d['slug']}
        for phrase in d['activationPhrases']:
            if not add_prefix:
                data['query'] = phrase
                yield data
                continue

            for prefix in ACTIVATION_PHRASES:
                data['query'] = prefix + phrase
                yield data
            if d['category'] not in ["games_trivia_accessories", "kids"]:
                continue
            for prefix in ["давай поиграем в ", "давай сыграем в "]:
                data['query'] = prefix + phrase
                yield data


def main():
    parser = argparse.ArgumentParser(prog='Query saas')
    parser.add_argument('--yt-output')
    parser.add_argument('--yt-cluster')
    parser.add_argument('--format', choices=INPUT_FORMATS, default='plain')
    parser.add_argument('--params', default='',
                        help='for example: dump=eventlog&numdoc=10')
    parser.add_argument('--channel', choices=CHANNELS, default='aliceSkill')
    parser.add_argument('--add-prefix', action='store_true')
    parser.add_argument('--concrete-url', action='store_true')
    parser.add_argument('--verbose', type=int, default=0)
    parser.add_argument('--softness', type=int)
    parser.add_argument('--mau', type=int)
    parser.add_argument('--kps', type=int, default=101)
    parser.add_argument('--proxy',
                        default='saas-searchproxy-prestable.yandex.net')
    parser.add_argument('--service-name', default='alisa_skills')
    parser.add_argument('--num-workers', type=int, default=16)
    parser.add_argument('--timeout', type=int, default=1000000)
    parser.add_argument('--chunk-size', type=int, default=2000)

    parser.add_argument('--min-tokens-num', type=int, default=1)
    parser.add_argument('--max-tokens-num', type=int, default=200)
    parser.add_argument('--min-query-length', type=int, default=0)
    parser.add_argument('--max-query-length', type=int, default=20000)
    parser.add_argument('--retries-num', type=int, default=10)
    args = parser.parse_args()

    if args.format == 'json':
        queries = _read_queries_json(sys.stdin)
    elif args.format == 'plain':
        queries = _read_queries_plain(sys.stdin)
    elif args.format == 'stable':
        queries = _read_queries_stable(sys.stdin, args.add_prefix,
                                       args.channel)
    elif args.format == 'tsv':
        queries = _read_queries_tsv(sys.stdin)
    elif args.format == 'wizqbundle':
        queries = _read_wizqbundle(sys.stdin)
    elif args.format == 'toloka':
        queries = _read_toloka(sys.stdin)

    args.params = urllib.parse.parse_qs(args.params)

    if args.yt_output:
        client = yt.wrapper.YtClient(args.yt_cluster)
        client.write_table(args.yt_output,
                           _query_saas_by_chunks(queries, args, args.chunk_size),
                           format=yt.wrapper.JsonFormat(attributes={"encode_utf8": False}))
    else:
        for result in _query_saas_by_chunks(queries, args, args.chunk_size):
            print(json.dumps(result, ensure_ascii=False))


if __name__ == '__main__':
    main()
