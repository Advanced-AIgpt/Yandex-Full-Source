import sys
import numpy as np
import requests
import json

class NlgsearchHttpSource:
    def __init__(self):
        self.host = 'general-conversation-hamster.yappy.beta.yandex.ru'
        self.port = 80
        self.search_for = 'context_and_reply'
        self.search_by = 'context'
        self.experiments = ''
        self.max_results = 10
        self.min_ratio = 0.5
        self.context_weight = 1.0
        self.ranker = 'catboost'
        self.extra_params = None

    def _parse_response(self, response, search_for):
        if search_for == 'context_and_reply':
            search_for = 'reply'
        #search_for = 'context'
        replies = []
        d = json.loads(response)
        if 'Grouping' not in d:
            return [(u'Что?', -1)] # FIXME: VINS hack
        if not d['Grouping']:
            return replies
        groups = d['Grouping'][0]['Group']
        for g in groups:
            doc = g['Document'][0]
            relevance = doc['Relevance']
            if not 'GtaRelatedAttribute' in doc['ArchiveInfo']:
                replies.append(('', relevance))
                continue
            attrs = doc['ArchiveInfo']['GtaRelatedAttribute']
            text = None
            for attr in attrs:
                if attr['Key'] == search_for:
                    assert text is None
                    text = attr['Value']
            assert text is not None
            replies.append(dict(text=text, relevance=relevance))
        replies.sort(key=lambda x: x['relevance'])
        return replies

    def get_candidates(self, context):
        text = '\n'.join(context)
        experiments = self.experiments.split(',')
        experiments = [el for el in experiments if el]
        experiments = ''.join('&pron=exp_{}'.format(exp) for exp in experiments)
        url = 'http://{}:{}/yandsearch?g=0..1000&ms=proto&hr=json{}'.format(self.host, self.port, experiments)
        relev_params = {
            'MaxResults': self.max_results,
            'MinRatioWithBestResponse': self.min_ratio,
            'SearchFor': self.search_for,
            'SearchBy': self.search_by,
            'ContextWeight': self.context_weight,
            'RankerModelName': self.ranker,
            'DssmModelName': 'insight_c3_rus_lister', # make sure that it doesn't affect anything
            'TfRankerAlpha': 1
        }
        relev_params = ';'.join(k + '=' + str(v) for k, v in relev_params.items())
        if self.extra_params:
            relev_params += ';' + self.extra_params
        params = {
            'text': text,
            'relev': relev_params
        }
        response = requests.get(url, params=params)
        return self._parse_response(response.content, self.search_for)


def main():
    lens = []
    source = NlgsearchHttpSource()

    start = input("Enter start phrase: ")
    context = [start]
    while True:
        candidates = source.get_candidates(context)
        reply = np.random.choice(candidates)['text']
        print(reply)
        context.append(reply)
        context = context[-3:]

