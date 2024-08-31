#!/usr/local/bin/python
# -*- coding: utf-8 -*-

import argparse
import json
import sys
from collections import Counter
from io import open


def safe_div(first, second):
    if second == 0:
        return 0
    return float(first) / second


def extend_scores(d, top_n):
    d['precision1'] = safe_div(d['top1_tp'], d['top1_tp'] + d['top1_fp'])
    d['precision5'] = safe_div(d['top5_tp'], d['top5_tp'] + d['top5_fp'])
    d['precision10'] = safe_div(d['top10_tp'], d['top10_tp'] + d['top10_fp'])
    if top_n:
        d['precision' + top_n] = safe_div(d['top' + top_n + '_tp'], d['top' + top_n + '_tp'] + d['top' + top_n + '_fp'])

    d['recall1'] = safe_div(d['top1_tp'], d['top1_tp'] + d['top1_fn'])
    d['recall5'] = safe_div(d['top5_tp'], d['top5_tp'] + d['top5_fn'])
    d['recall10'] = safe_div(d['top10_tp'], d['top10_tp'] + d['top10_fn'])
    if top_n:
        d['recall' + top_n] = safe_div(d['top' + top_n + '_tp'], d['top' + top_n + '_tp'] + d['top' + top_n + '_fn'])

    d['accuracy1'] = safe_div(d['top1_tp'] + d['top1_tn'], d['top1_tp'] + d['top1_tn'] + d['top1_fp'] + d['top1_fn'])
    d['accuracy5'] = safe_div(d['top5_tp'] + d['top5_tn'], d['top5_tp'] + d['top5_tn'] + d['top5_fp'] + d['top5_fn'])
    d['accuracy10'] = safe_div(d['top10_tp'] + d['top10_tn'],
                               d['top10_tp'] + d['top10_tn'] + d['top10_fp'] + d['top10_fn'])
    if top_n:
        d['accuracy' + top_n] = safe_div(d['top' + top_n + '_tp'] + d['top' + top_n + '_tn'],
                                         d['top' + top_n + '_tp'] + d['top' + top_n + '_tn'] + d[
                                             'top' + top_n + '_fp'] + d['top' + top_n + '_fn'])


def load_relevance(relevance_dict, stream):
    for line in stream:
        d = json.loads(line)
        relevance_dict[(d['query'].strip().lower(), d['skill_id'])] = 1 if d['answer'] == 'YES' else 0


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


def filter_saas_results(stream, threshold, use_weight=False):
    saas_results_dict = {}
    query_weights = {}
    for line in stream:
        d = json.loads(line)

        query = d['query']['query'].strip().lower()

        w = 1
        if use_weight and 'weight' in d['query'] and d['query']['weight'] is not None:
            w = d['query']['weight']

        query_weights[query] = w

        if 'response' not in d:
            continue

        responses = [get_resp_dict(x) for x in d['response']]
        responses = [x for x in responses if int(x['relevance']) >= threshold]
        if len(responses) == 0:
            continue

        saas_results_dict[query] = responses

    return saas_results_dict, query_weights


def score(relevance_dict, saas_results_dict, query_weights, counters, top_n):
    for query, skill_id in relevance_dict:
        answer = relevance_dict[(query, skill_id)]

        if query in saas_results_dict:
            w = query_weights.get(query, 1)

            saas_responses = saas_results_dict[query]

            found_in_top_1 = False
            if saas_responses[0]['url'] == skill_id:
                found_in_top_1 = True

            found_in_top_5 = False
            for response in saas_responses[:5]:
                if response['url'] == skill_id:
                    found_in_top_5 = True
                    break

            found_in_top_10 = False
            for response in saas_responses[:10]:
                if response['url'] == skill_id:
                    found_in_top_10 = True
                    break

            found_in_top_n = False
            if top_n:
                for response in saas_responses[:int(top_n)]:
                    if response['url'] == skill_id:
                        found_in_top_n = True
                        break

            if answer == 1:
                if found_in_top_1:
                    counters['top1_tp'] += w
                else:
                    counters['top1_fn'] += w

                if found_in_top_5:
                    counters['top5_tp'] += w
                else:
                    counters['top5_fn'] += w

                if found_in_top_10:
                    counters['top10_tp'] += w
                else:
                    counters['top10_fn'] += w

                if top_n:
                    if found_in_top_n:
                        counters['top' + top_n + '_tp'] += w
                    else:
                        counters['top' + top_n + '_fn'] += w
            elif answer == 0:
                if found_in_top_1:
                    counters['top1_fp'] += w
                else:
                    counters['top1_tn'] += w

                if found_in_top_5:
                    counters['top5_fp'] += w
                else:
                    counters['top5_tn'] += w

                if found_in_top_10:
                    counters['top10_fp'] += w
                else:
                    counters['top10_tn'] += w

                if top_n:
                    if found_in_top_n:
                        counters['top' + top_n + '_fp'] += w
                    else:
                        counters['top' + top_n + '_tn'] += w


def main():
    parser = argparse.ArgumentParser(prog='Score saas results')
    parser.add_argument('--relevance', required=True)
    parser.add_argument('--saas-results', required=True)
    parser.add_argument('--skills')
    parser.add_argument('--top-n')
    parser.add_argument('--threshold', type=int, default=0)
    parser.add_argument('--use-weight', action='store_true')
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

    with open(args.saas_results, encoding='utf-8') as saas_results:
        filtered_saas_results, query_weights = filter_saas_results(saas_results, args.threshold, args.use_weight)
    score(relevance_dict, filtered_saas_results, query_weights, counters, args.top_n)
    extend_scores(counters, args.top_n)

    json.dump(counters, sys.stdout, sort_keys=True, indent=4, separators=(',', ': '))


if __name__ == "__main__":
    main()
