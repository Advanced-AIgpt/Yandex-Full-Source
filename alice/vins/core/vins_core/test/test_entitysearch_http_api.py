# coding: utf-8
from __future__ import unicode_literals

import pytest
import requests_mock


from vins_core.ext.entitysearch import EntitySearchHTTPAPI


@pytest.fixture(scope='module')
def http_api():
    return EntitySearchHTTPAPI()


@pytest.fixture(scope='module')
def entity_cards():
    return {
        'ruw24738': {
            'base_info': {
                'id': 'ruw24738'
            }
        },
        'ruw324234': {
            'base_info': {
                'id': 'ruw324234'
            }
        },
        'invalid-card': {
            'base_info': {}
        },
        'invalid-card2': {}
    }


def test_entity_search_api(http_api, entity_cards):
    # Test call with one id & correct result.
    entity_id = 'ruw24738'
    with requests_mock.Mocker() as mock:
        mock.get(http_api.ENTITYSEARCH_URL, json={
            'cards': [entity_cards[entity_id]]
        })
        resp = http_api.get_response(entity_id, None, True)
        assert resp.keys() == [entity_id]
        assert resp.values() == [entity_cards[entity_id]]

    # Test call with several different ids.
    entity_ids = ['ruw24738', 'ruw324234']
    with requests_mock.Mocker() as mock:
        mock.get(http_api.ENTITYSEARCH_URL, json={
            'cards': [entity_cards[id] for id in entity_ids]
        })
        resp = http_api.get_response(entity_ids, None, True)
        assert set(resp.keys()) == set(entity_ids)
        for id in entity_ids:
            assert resp[id] == entity_cards[id]

    # Test call with two equal ids.
    entity_ids = ['ruw24738', 'ruw24738']
    with requests_mock.Mocker() as mock:
        mock.get(http_api.ENTITYSEARCH_URL, json={
            'cards': [entity_cards['ruw24738']]
        })
        resp = http_api.get_response(entity_ids, None, True)
        assert resp.keys() == ['ruw24738']
        assert resp['ruw24738'] == entity_cards['ruw24738']
