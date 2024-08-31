from __future__ import print_function
from __future__ import unicode_literals
# from builtins import str

import argparse
import json
import sys
from io import open
from io import StringIO

import hashlib
import yt.wrapper


def from_list_of_dicts(l):
    result = dict()
    for d in l:
        for k, v in d.items():
            result[k] = v
    return result


def invert_dict(d):
    return dict((v, k) for k, v in d.items())


def _read_factors_names(stream):
    result = dict()
    d = json.load(stream)
    for k, v in d['static_factors'].items():
        assert v['index'] not in result
        result[v['index']] = k

    for name in ['zone_factors', 'dynamic_factors']:
        for k, v in d[name].items():
            assert v not in result
            result[v] = k

    return result


def _read_responses(stream, verbose):
    for d in stream:
        if 'response' not in d:
            print(line.strip(), file=sys.stderr)
            continue
        query = d['query']['query']
        skill_id = d['query']['skill_id']
        relev = 1.0 if d['query'].get('answer') == 'YES' or d['query'].get('golden') == 'YES' else 0.0
        if len(d['response']) != 1:
            if verbose > 1:
                print('Not 1 response', line.strip(), file=sys.stderr)
            continue
        response = d['response'][0]
        assert response['url'] == skill_id
        try:
            factors = json.loads(response['factors'])
        except json.decoder.JSONDecodeError:
            if verbose > 0:
                print('Bad factors json: ', line.strip(), file=sys.stderr)
            continue

        yield {'query': query, 'relevance': relev, 'skill_id': skill_id, 'factors': factors}


def _to_yt(d, factor_names, blacklist_idx):
    max_factor_index = max(factor_names.keys())
    key = hashlib.md5((d['query']).encode('utf-8')).hexdigest()
    value = StringIO()
    print(str(d['relevance']) + '\t' + str(d['skill_id']) + '\t0', end='', file=value)
    factors = from_list_of_dicts(d['factors'])
    for index in range(max_factor_index + 1):
        if index in factor_names and index not in blacklist_idx:
            value.write('\t' + str(factors[factor_names[index]]))
        else:
            value.write('\t0.0')
    data = {
        #                'query': query,
        'key': key,
        'value': value.getvalue()
    }

    return data


class FmlMapper(object):
    def __init__(self, factor_names, blacklist_idx, verbose):
        self.factor_names = factor_names
        self.blacklist_idx = blacklist_idx
        self.verbose = verbose

    def __call__(self, row):
        for resp in _read_responses([row], self.verbose):
            yield _to_yt(resp, self.factor_names, self.blacklist_idx)


def main():
    parser = argparse.ArgumentParser(prog='Fml pool')
    parser.add_argument('--factor-names', required=True)
    parser.add_argument('--factor-names-out')
    parser.add_argument('--yt-cluster', required=True)
    parser.add_argument('--yt-input', required=True)
    parser.add_argument('--yt-output', required=True)
    parser.add_argument('--blacklist', nargs='*')
    parser.add_argument('--verbose', type=int, default=0)
    args = parser.parse_args()

    with open(args.factor_names, encoding='utf-8') as f:
        factor_names = _read_factors_names(f)

    if args.factor_names_out:
        with open(args.factor_names_out, 'w', encoding='utf-8') as f:
            max_factor_index = max(factor_names.keys())
            for index in range(max_factor_index + 1):
                name = factor_names.get(index, 'factor_' + str(index))
                print(json.dumps({'key': str(index), 'value': name}, ensure_ascii=False, sort_keys=True), file=f)


    inv_factor_names = invert_dict(factor_names)
    blacklist_idx = [inv_factor_names[b_item] for b_item in args.blacklist]
    if len(args.blacklist) != 0:
        print('[Warning] setting following features to zero', args.blacklist, file=sys.stderr)

    client = yt.wrapper.YtClient(args.yt_cluster)
    client.run_map(FmlMapper(factor_names, blacklist_idx, args.verbose),
            args.yt_input,
            args.yt_output,
            format=yt.wrapper.JsonFormat(attributes={"encode_utf8": False}))


if __name__ == '__main__':
    main()
