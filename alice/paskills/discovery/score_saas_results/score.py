#!/usr/local/bin/python
# -*- coding: utf-8 -*-

import argparse
import sys
import six
import json
from io import open
from collections import Counter


def safe_div(first, second):
    if second == 0:
        return 0
    return float(first) / second


def extend_scores(d):
    d['acc1'] = safe_div(d['top1_rel'], d['sth_found'])
    d['acc5'] = safe_div(d['top5_rel'], d['sth_found'])
    d['acc10'] = safe_div(d['top10_rel'], d['sth_found'])
    d['found_percent'] = safe_div(d['sth_found'], d['total_count'])


def load_relevance(relevance_dict, stream):
    for line in stream:
        d = json.loads(line)
        relevance_dict[(d['query'].strip().lower(), d['skill_id'])] = 1 if d['answer'] == 'YES' or d[
            'golden'] == 'YES' else 0


def load_stable(relevance_dict, stream, add_prefix):
    for line in stream:
        d = json.loads(line)
        if d['channel'] != 'aliceSkill':
            continue
        if d['isBanned']:
            continue
        if d['hideInStore']:
            continue
        if d['deletedAt']:
            continue
        if not d['onAir']:
            continue

        for phrase in d['activationPhrases']:
            if not add_prefix:
                relevance_dict[(phrase.strip().lower(), d['id'])] = 1
                continue

            for prefix in [u"открой навык ", u"открой диалог ", u"запусти навык ", u"запусти диалог ",
                           u"активируй навык ", u"включи навык ", u"вызови навык ", u"открой чат ", u"запусти чат с "]:
                relevance_dict[((prefix + phrase).strip().lower(), d['id'])] = 1
            if d['category'] not in ["games_trivia_accessories", "kids"]:
                continue
            for prefix in [u"давай поиграем в ", u"давай сыграем в "]:
                relevance_dict[((prefix + phrase).strip().lower(), d['id'])] = 1


def get_resp_dict(resp):
    if isinstance(resp, dict):
        return resp
    return {'url': resp[0], 'relevance': resp[1]}


def score(stream, relevance_dict, threshold, counters, use_weight=False):
    unique_skills = set()
    for line in stream:
        d = json.loads(line)
        query = d['query']['query'].strip().lower()
        w = 1
        if use_weight and 'weight' in d['query'] and d['query']['weight'] is not None:
            w = d['query']['weight']
        counters['total_count'] += w
        if 'response' not in d:
            continue
        response = [get_resp_dict(x) for x in d['response']]
        response = [x for x in response if int(x['relevance']) >= threshold]
        if len(response) == 0:
            continue
        counters['sth_found'] += w

        if relevance_dict.get((query, response[0]['url']), 0):
            counters['top1_rel'] += w

        for resp in response[:5]:
            if relevance_dict.get((query, resp['url']), 0):
                counters['top5_rel'] += w
                break

        for resp in response[:10]:
            if relevance_dict.get((query, resp['url']), 0):
                counters['top10_rel'] += w
                break

        for i, resp in enumerate(response):
            unique_skills.add(resp['url'])
            if (query, resp['url']) not in relevance_dict:
                if i == 0:
                    counters['not_found1'] += 1
                    counters['not_found5'] += 1
                    counters['not_found10'] += 1
                if 0 < i <= 4:
                    counters['not_found5'] += 1
                    counters['not_found10'] += 1
                if 4 < i <= 9:
                    counters['not_found10'] += 1

                yield (resp['url'], i, query)

    counters['found_unique_skills_count'] = len(unique_skills)


def main():
    parser = argparse.ArgumentParser(prog='Score saas results')
    parser.add_argument('--relevance', required=True)
    parser.add_argument('--skills')
    parser.add_argument('--not-found-out')
    parser.add_argument('--threshold', type=int, default=0)
    parser.add_argument('--use-weight', action='store_true')
    parser.add_argument('--dataset-name')
    parser.add_argument('--fielddate')
    args = parser.parse_args()

    relevance_dict = dict()
    if args.skills:
        with open(args.skills, encoding='utf-8') as f:
            load_stable(relevance_dict, f, False)
        with open(args.skills, encoding='utf-8') as f:
            load_stable(relevance_dict, f, True)
    with open(args.relevance, encoding='utf-8') as f:
        load_relevance(relevance_dict, f)

    counters = Counter()
    with open(args.not_found_out, 'w', encoding='utf-8') as f:
        for skill_id, pos, query in score(sys.stdin, relevance_dict, args.threshold, counters, args.use_weight):
            six.print_(skill_id, unicode(pos).zfill(5), query, sep=u'\t', file=f)
            counters['not_found'] += 1
    extend_scores(counters)
    if args.dataset_name is not None:
        counters['dataset_name'] = args.dataset_name
    if args.fielddate is not None:
        counters['fielddate'] = args.fielddate
    json.dump(counters, sys.stdout, sort_keys=True, indent=4, separators=(',', ': '))


if __name__ == "__main__":
    main()
