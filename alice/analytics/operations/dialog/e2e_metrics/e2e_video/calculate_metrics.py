# -*-coding: utf8 -*-
#!/usr/bin/env python

import argparse
import json
import math
from collections import defaultdict
import nirvana.job_context as nv
import sys


reload(sys)
sys.setdefaultencoding('utf-8')

ctx = nv.context()
parameters = json.loads(ctx.get_parameters().get('kwargs', '{}'))
inputs = ctx.get_inputs()
outputs = ctx.get_outputs()


RESULT_TO_FLOAT = {
    'irrel': 0,
    'rel_minus': 0.5,
    'rel_plus': 1
}


UE2E_TO_FLOAT = {
    'bad': 0,
    'part': 0.5,
    'good': 1
}


def get_provider(url):
    if url:
        if url.startswith(('https://www.', 'http://www.')):
            return url.split('www.')[1].split('.')[0]
        elif url.startswith(('https://', 'http://')):
            return url.split('://')[1].split('.')[0]
    else:
        print url
        return ""


def get_metrics_obj(request):
    request['result'] = request.get('result') or 'video'

    obj = {
        'type': 'SERP',
        'query': {
            'text': request['text'],
            'regionId': 225,
            'country': 'RU',
            'device': 0,
            'uid': request.get('request_id')
            },
        'components': [],
        'metric.true.quasar_video_score': request['score'],
        'metric.true.quasar_video_rel_top1': request['score_top1'],
        'judgements.ue2e_relevance': {
                'name': request['score'],
                'scale': 'ue2e_relevance'
            }
        }

    judgements_keys = [
        'result',
        'hyp',
        'query_type',
        'mds_url',
        'display_type',
        'scenario_type',
        'intent',
        'reply',
        'ya_video_request',
        'debug_url'
    ]

    for jk in judgements_keys:
        obj['judgements.{}'.format(jk)] = {'name': request.get(jk)}

    judgements_json_keys = ['attentions', 'provider_requests']

    for jk in judgements_json_keys:
        obj['judgements.{}'.format(jk)] = {'name': json.dumps(request.get(jk))}

    items = request.get('items', [])
    provider_dict = {}

    for item in items:
        comps = {
            'componentUrl': {
                'pageUrl': item['video'],
                'viewUrl': item['video']
            },
            'text.yang_url': item.get('yang_url', ''),
            'text.video_uuid': item.get('video_uuid', ''),
            'text.title': item.get('name', ''),
            'judgements.yang_relevance': {
                'scale': 'yang_relevance',
                'name': item.get('yang_result', {}).get('result'),
                'jid': item.get('yang_result', {}).get('jid'),
                'ts': item.get('yang_result', {}).get('ts')
            },
            'type': 'COMPONENT'
        }

        obj['components'].append(comps)

        if get_provider(item['video']) in ('kinopoisk', 'ivi', 'amediateka'):
            provider_dict['has_provider_response'] = 1.0
        else:
            provider_dict['has_yavideo_response'] = 1.0

    for jk in provider_dict.keys():
        obj['judgements.{}'.format(jk)] = {'value': provider_dict[jk]}

    return obj


def calc_score(query_type, mark, i, total):
    if query_type == 'film_wide':
        weight = 1.0 / total
    else:
        weight = 1.0 / math.log(i + 2, 2) # wtf 2**(total - i - 1) / (2**total - 1)
    return RESULT_TO_FLOAT.get(mark, 0) * weight


def main():

    yang_results = {}
    with open(inputs.get("input1"), "r") as f:
        for item in json.load(f):
            key = (item["query_text"], item['component_page_url'])

            if key not in yang_results:
                yang_results[key] = {
                    "result": item.get("dynamic_judgement:video_relevance", ""),
                    "ts": item.get("datetime", ""),
                    "jid": item.get("dynamic_judgement:multi_judgement:video_relevance:id", "")
                    }

    results = []
    results_metrics = []
    metrics = defaultdict(int)
    scores = defaultdict(int)

    with open(inputs.get("input2"), "r") as f:
        for request in json.load(f):
            request['score'] = 0
            request['score_top1'] = 0
            items = request.get('items', [])
            if items:
                metrics['{}_total'.format(request['query_type'])] += 1
                metrics['total'] += 1
            elif request.get('result'):
                metrics['{}'.format(request['result'])] += 1
                metrics['{}_{}'.format(request['query_type'], request['result'])] += 1

            for i, item in enumerate(items):
                url = item.get('actual_url') or item.get('video')
                key = (request['text'], url)
                yang_result = yang_results.get(key) or {'result': 'not_found'}
                if yang_result['result'] != 'not_found':
                    if yang_result['result'] in ["RELEVANT_PLUS", "RELEVANT_MINUS"]:
                        mark = "rel_plus"
                    elif yang_result['result'] == "IRRELEVANT":
                        mark = "irrel"
                    else:
                        mark = "404"
                else:
                    mark = 'not_found'
                request['items'][i]['yang_result'] = yang_result
                request['items'][i]['yang_url'] = url
                metrics['url_result_{}'.format(request['items'][i]['result'])] += 1
                request['score'] += calc_score(request['query_type'], mark, i, min(len(request['items']), int(parameters.get('top'))))
                if i == 0:
                    request['score_top1'] = calc_score(request['query_type'], mark, i, 1)

            results.append(request)
            results_metrics.append(get_metrics_obj(request))
            scores[request['query_type']] += request['score']
    """
    if inputs.get("input3"):
        with open(inputs.get("input3"), "r") as f:
            for request in json.load(f):
                new_request = {}
                mark = UE2E_TO_FLOAT.get(request.get('result'), 0)
                new_request['result'] = 'ue2e'
                new_request['text'] = request.get('text')
                new_request['hyp'] = request.get('asr_text')
                new_request['reply'] = request.get('answer')
                new_request['request_id'] = request.get('req_id')
                intent = request.get('intent') or ''
                new_request['intent'] = intent.replace('\t', '.')
                new_request['query_type'] = 'other'
                new_request['score'] = mark
                new_request['score_top1'] = mark
                new_request['provider_requests'] = {}
                new_request['attentions'] = []

                results.append(new_request)
                results_metrics.append(get_metrics_obj(new_request))
    """
    with open(outputs.get("output1"), 'w') as f:
        json.dump(results, f, indent=2, ensure_ascii=False)

    with open(outputs.get("output2"), 'w') as f:
        json.dump(results_metrics, f, indent=2, ensure_ascii=False)

    total_score = 0
    for query_type in scores.keys():
        if metrics['{}_total'.format(query_type)]:
            metrics['{}_score'.format(query_type)] = 1.0 * scores[query_type] / metrics['{}_total'.format(query_type)]
            total_score += scores[query_type]

    metrics['score'] = 1.0 * total_score / metrics['total']
    metrics['not_video'] = 1.0 * metrics['not_video'] / metrics['total']
    metrics['fielddate'] = parameters.get('fielddate')

    with open(outputs.get("output3"), 'w') as f:
        json.dump(metrics, f, indent=2)


if __name__ == '__main__':
    main()
