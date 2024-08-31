import requests
import json

from .cfg import SAAS_CONFIG


def __extract(saas_response):
    responses = []
    for result in saas_response['response'].get('results', {}):
        for group in result.get('groups', {}):
            for doc in group.get('documents', []):
                skill_id, relevance, title = doc['url'], doc['relevance'], doc['title']
                title = "".join(map(lambda x: x['text'], title))
                responses.append({
                    'skill_id': skill_id,
                    'relevance': int(relevance),
                    # 'title': title,
                })
    return responses, saas_response['response']['searcher_properties']['RankingModel']


def __curl_saas(text, proxy, kps, service_name, formula,
                softness=None, how=None, saas_output=None, verbose=0, **args):
    params = {
        'text': text,
        'format': 'json',
        'kps': kps,
        'relev': 'formula={}'.format(formula),
        'service': service_name,
    }
    if softness is not None:
        params['text'] += ' softness:{}'.format(softness)

    if how is not None:
        params['how'] = how

    url = 'http://{}:17000'.format(proxy)
    req = requests.get(
        url,
        params=params,
    )
    if verbose > 1:
        print(req.url)
    response = req.json()
    if saas_output:
        with open(saas_output, 'a') as f:
            f.write(json.dumps(response, ensure_ascii=False) + '\n')

    return __extract(response)


def ask_saas(text, verbose=0, **args):
    def __map(__r):
        __skill_id = __r['skill_id']
        __relevance = 'OK' if __r['relevance'] >= SAAS_CONFIG['threshold'] else 'Thr'
        return (__skill_id, __relevance)

    override_with_args(**args)
    response, formula = __curl_saas(text, verbose=verbose, **SAAS_CONFIG)
    response = map(__map, response)
    response = {__id: __relev for __id, __relev in response}
    if formula == SAAS_CONFIG['formula']:
        same_formula = 'OK'
    else:
        same_formula = 'ERR'
    return response, same_formula

def override_with_args(**args):
    for key in SAAS_CONFIG:
        args_key = "saas_" + key
        if args_key in args and args[args_key]:
            SAAS_CONFIG[key] = args[args_key]