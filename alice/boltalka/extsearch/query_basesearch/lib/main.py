import sys
import json
import requests
from urllib3.util.retry import Retry
from requests.adapters import HTTPAdapter


class QueryBasesearch(object):
    def get_params(self, context, extra_params=None):
        relev_params = dict(self.relev_params)
        if extra_params:
            relev_params.update(extra_params)
        relev_params = ';'.join(k + '=' + str(v) for k, v in relev_params.items())
        if isinstance(self.extra_params, str):
            relev_params += ';' + self.extra_params
        if isinstance(context, list):
            context = '\n'.join(context)
        return dict(
            relev=relev_params,
            text=context
        )

    def get_params_with_entity(self, context, entity=None):
        return self.get_params(context, {"Entities": entity} if entity else {})

    def __init__(self, host='general-conversation-hamster.yappy.beta.yandex.ru', port=80,
                 experiments='', max_results=1, min_ratio=1.0, context_weight=1.0,
                 ranker='catboost', extra_params=None,
                 pool_size=5, max_retries=5, deterministic=True, output_attr='reply'):
        experiments = experiments.split(',')
        experiments = [el for el in experiments if el]
        if deterministic:
            experiments.append('gc_random_salt_0')
        experiments = ''.join('&pron=exp_{}'.format(exp) for exp in experiments)
        self.url = 'http://{}:{}/yandsearch?g=0..100&ms=proto&hr=json{}&fsgta=_Seq2SeqResult'.format(host, port, experiments)
        self.pool_size = pool_size
        self.output_attr = output_attr

        self.relev_params = {
            'MaxResults': max_results,
            'MinRatioWithBestResponse': min_ratio,
            'Seq2SeqMaxResponses': 1,
            'SearchFor': 'context_and_reply',
            'SearchBy': 'context',
            'ContextWeight': context_weight,
            'RankerModelName': ranker,
        }
        self.extra_params = extra_params
        if isinstance(self.extra_params, dict):
            self.relev_params.update(extra_params)
        self.session = requests.Session()
        retries = Retry(total=max_retries, backoff_factor=1.0, status_forcelist=[500, 502, 503, 504])
        self.session.mount('http://', HTTPAdapter(max_retries=retries))

    def parse_response(self, response):
        replies = []
        d = json.loads(response)
        if 'Grouping' not in d:
            print(d, file=sys.stderr)
            return [dict(reply='', relevance=0, source='', index_context=[])]
        if not d['Grouping']:
            return replies
        groups = d['Grouping'][0]['Group']
        for g in groups:
            doc = g['Document'][0]
            relevance = doc['Relevance']
            text = None
            source = None
            index_context = []
            if 'FirstStageAttribute' in doc:
                for attr in doc['FirstStageAttribute']:
                    if doc['FirstStageAttribute'][0]['Key'] == "_Seq2SeqResult" and doc['FirstStageAttribute'][0]['Value'] != '':
                        text = doc['FirstStageAttribute'][0]['Value']
                        source = 'seq2seq'
                        break
            if source != 'seq2seq':
                if 'GtaRelatedAttribute' not in doc['ArchiveInfo']:
                    text = ''
                    source = 'UNKNOWN'
                else:
                    attrs = doc['ArchiveInfo']['GtaRelatedAttribute']
                    for attr in attrs:
                        if attr['Key'] == self.output_attr:
                            assert text is None
                            text = attr['Value']
                        if attr['Key'] == 'source':
                            source = attr['Value']
                        if attr['Key'] == 'context':
                            index_context = attr['Value'].split(' _EOS_ ')
            replies.append(dict(
                reply=text,
                relevance=relevance,
                source=source,
                index_context=index_context,
            ))
        replies.sort(key=lambda x: -x['relevance'])
        return replies

    def get_replies(self, context, entity=None):
        params = self.get_params_with_entity(context, entity)
        print(params)
        response = self.session.get(self.url, params=params)
        return self.parse_response(response.content)
