# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging
import re

from requests import RequestException

from vins_core.common.annotations.entitysearch import EntitySearchAnnotation, Entity, EntityFeatures
from vins_core.nlu.sample_processors.base import BaseSampleProcessor
from vins_core.ext.entitysearch import EntitySearchHTTPAPI
from rtlog import AsJson


logger = logging.getLogger(__name__)


class EntitySearchSampleProcessor(BaseSampleProcessor):
    """
    Entity search sample processor extracts EntitySearch annotation and adds it to a sample.
    **Requires** WizardAnnotation in a sample (uses 'EntityFinder' rule to extract entity ids).

    This annotation is used later to possibly import entities in form slots.
    """

    def __init__(self, url=None, **kwargs):
        super(EntitySearchSampleProcessor, self).__init__(**kwargs)

        self._entitysearch = EntitySearchHTTPAPI(url=url)

    @property
    def is_normalizing(self):
        return False

    def _get_annotation(self, entity_infos, features, not_use_entity_from_mm_flag):
        try:
            entity_cards = self._entitysearch.get_response(entity_infos.keys(), features, not_use_entity_from_mm_flag)

            parsed_entities = []
            for ent_id, card in entity_cards.iteritems():
                base_info = card.get('base_info', {})
                type = base_info.get('type', '')
                subtype = base_info.get('wsubtype', [''])[0]
                subtype = re.sub('@on$', '', subtype)

                if subtype:
                    type = '{}/{}'.format(type, subtype)

                if ent_id not in entity_infos:
                    continue

                start, end = entity_infos[ent_id]
                parsed_entities.append(Entity(
                    id=ent_id,
                    tags=card.get('tags', []),
                    name=base_info.get('name'),
                    type=type,
                    start=int(start),
                    end=int(end)
                ))

            entity_features = self._extract_entity_features(entity_cards)
            return EntitySearchAnnotation(entities=parsed_entities, entity_features=entity_features)
        except RequestException:
            return EntitySearchAnnotation()

    def _extract_entity_features(self, entity_cards):
        tags = tuple({tag for card in entity_cards.itervalues() for tag in card.get('tags', [])})
        base_infos = [card['base_info'] for card in entity_cards.itervalues()]

        site_ids = tuple({id for base_info in base_infos for id in base_info.get('ids', {}).keys()})
        has_music_info = any('music_info' in base_info for base_info in base_infos)
        types = tuple({base_info['type'] for base_info in base_infos if 'type' in base_info})
        subtypes = tuple({subtype for base_info in base_infos for subtype in base_info.get('wsubtype', ())})

        return EntityFeatures(
            tags=tags, site_ids=site_ids, has_music_info=has_music_info, types=types, subtypes=subtypes
        )

    def _process(self, sample, session, is_inference, *args, **kwargs):
        # EntitySearch annotations are used only during inference time.
        if not is_inference:
            return sample

        if 'wizard' not in sample.annotations:
            logger.warning('Can\'t extract EntitySearchAnnotation. Not found "wizard" annotation in a sample.')
            return sample

        entity_finder_rule = sample.annotations['wizard'].rules['EntityFinder']

        winners = entity_finder_rule.get('Winner', [])
        if isinstance(winners, basestring):
            winners = [winners]

        if len(winners) == 0:
            logger.info('Haven\'t found any entity ids in EntityFinder rule. EntitySearchAnnotation not extracted.')
            return sample

        # Dict (entity_id, (start, end))
        entity_infos = {}
        for winner in winners:
            # Winner format: "<entity_text>\t<start>\t<end>\t<entity_id>\t....".
            start, end, entity_id = winner.split('\t')[1:4]
            entity_infos[entity_id] = (start, end)

        logger.info('Found entity_ids in EntityFinder rule: %s', ', '.join(entity_infos.keys()))

        features = kwargs.get('features')
        not_use_entity_from_mm_flag = kwargs.get('not_use_entity_from_mm_flag')
        annotation = self._get_annotation(entity_infos, features, not_use_entity_from_mm_flag)

        logger.info('Extracted EntitySearchAnnotation: %(annotation)s', {'annotation': AsJson(annotation.to_dict())})
        sample.annotations['entitysearch'] = annotation

        return sample
