# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging
from vins_core.ext.base import BaseHTTPAPI
from vins_core.utils.config import get_setting
from vins_core.utils.metrics import sensors

logger = logging.getLogger(__name__)


class EntitySearchHTTPAPI(BaseHTTPAPI):
    MAX_RETRIES = 3
    TIMEOUT = float(get_setting('ENTITYSEARCH_TIMEOUT', 0.2))
    ENTITYSEARCH_URL = get_setting('ENTITYSEARCH_URL', yenv={
        'testing': 'http://entitysearch-test.yandex.net/get',
        'production': 'http://entitysearch.yandex.net/get'
    })

    def __init__(self, url=None, **kwargs):
        super(EntitySearchHTTPAPI, self).__init__(**kwargs)
        self._url = url or self.ENTITYSEARCH_URL

    @sensors.with_timer('entitysearch_response_time')
    def get_response(self, entity_ids, features, not_use_entity_from_mm_flag):
        """Get information about entity(ies) by their ids.

        Parameters
        ----------
        entity_ids : {str, list of str}
            Entity identifier or list of them.

        Returns
        -------
        Dict with items (id, card) where keys are entity ids and values are corresponding entity cards.

        Notes
        -----
        Returned dict may not contain all ids passed to the `get_response` function.

        """
        if not isinstance(entity_ids, basestring):
            entity_ids = ','.join(entity_ids)

        params = {
            'obj': entity_ids,
        }
        cards = None

        if features is not None and isinstance(features, dict) and not_use_entity_from_mm_flag is None:
            entity_search_response = features.get('entity_search', None)
            if entity_search_response is not None and isinstance(entity_search_response, dict):
                cards = entity_search_response.get('cards', [{}])

        if cards is None:
            resp = self.get(self._url, params=params, request_label='entitysearch:{0}'.format(entity_ids))
            sensors.inc_counter('entitysearch_response', labels={'status_code': resp.status_code})
            resp.raise_for_status()
            cards = resp.json().get('cards', [{}])

        res = {}

        for card in cards:
            entity_id = card.get('base_info', {}).get('id')
            if not entity_id:
                continue
            res[entity_id] = card

        return res


entitysearch_http_api = EntitySearchHTTPAPI()
