# coding: utf-8

import pytest
import requests_mock

from ..taggers_scores.prepare_requests_from_nlu import _request_generator, _iterate_nlu


@pytest.mark.parametrize('nlu_file, intent, response', [
    (
        u'# got from Toloka markup graph (the format is changed, honeypots are removed)\n'
        u'\'димитрова сорок пять\'(where)\n'
        u'все \'банкоматы клюква\'(what) \'город пермь\'(where)\n'
        u'\'москва\'(where)',
        u'find_poi',
        [{'app': None,
          'device_state': None,
          'experiments': {'debug_tagger_scores': True, 'force_intent': u'find_poi'},
          'request_id': '',
          'slots': {'where': [{'end': 3, 'is_continuation': False, 'start': 0}]},
          'text': 'димитрова сорок пять'},
         {'app': None,
          'device_state': None,
          'experiments': {'debug_tagger_scores': True, 'force_intent': u'find_poi'},
          'request_id': '',
          'slots': {'what': [{'end': 3, 'is_continuation': False, 'start': 1}],
                    'where': [{'end': 5, 'is_continuation': False, 'start': 3}]},
          'text': 'все банкоматы клюква город пермь'},
         {'app': None,
          'device_state': None,
          'experiments': {'debug_tagger_scores': True, 'force_intent': u'find_poi'},
          'request_id': '',
          'slots': {'where': [{'end': 1, 'is_continuation': False, 'start': 0}]},
          'text': 'москва'},
         ]
    ),
])
def test_requests_generator(nlu_file, intent, response):
    with requests_mock.mock() as m:
        test_url = 'mock://nlu_file'
        m.get(test_url, text=nlu_file)

        data = [result for result in _request_generator(test_url, intent, _iterate_nlu)]

    for object in data:
        object['request_id'] = ''
        object['slots'] = dict(object['slots'])

    assert response == data
