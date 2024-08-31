# coding: utf-8
from __future__ import unicode_literals

from vins_core.ner.custom import CustomEntityParser
from vins_core.common.entity import Entity
from vins_core.common.sample import Sample

import os.path
import pytest
import yatest.common


@pytest.mark.parametrize('utterance, entity_names, expected_entities', [
    ('останови все будильники', ['selection', 'video_episode'], [
        Entity(type='SELECTION', value='all', substr='все', start=1, end=2),
        Entity(type='VIDEO_EPISODE', value='all', substr='все', start=1, end=2),
    ]),
    ('включи семейный плейлист', ['special_playlist'], [
        Entity(type=u'SPECIAL_PLAYLIST', value=u'family', substr=u'семейный', start=1, end=2),
        Entity(type=u'SPECIAL_PLAYLIST', value=u'family', substr=u'семейный плейлист', start=1, end=3),
    ])
])
def test_custom_entity_parser(utterance, entity_names, expected_entities):
    entity_path = os.path.join(
        yatest.common.binary_path('alice/vins/resources'),
        'personal_assistant_model_directory/custom_entities/all'
    )
    parser = CustomEntityParser(fst_name='test', fst_path=entity_path, entity_names=entity_names)
    entities = parser.parse(Sample.from_string(utterance))
    def key(e):
        return e.start, e.end, e.type, e.value
    assert sorted(expected_entities, key=key) == sorted(entities, key=key)
