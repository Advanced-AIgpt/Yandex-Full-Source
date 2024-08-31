# coding: utf-8
from __future__ import unicode_literals

import copy
import json
from uuid import uuid4

import pytest
import requests_mock
import attr

from vins_core.common.annotations import WizardAnnotation, BaseAnnotation, register_annotation
from vins_core.common.annotations.entitysearch import Entity
from vins_core.common.sample import Sample
from vins_core.common.utterance import Utterance
from vins_core.dm.formats import FuzzyNLUFormat
from vins_core.ner.fst_normalizer import DEFAULT_RU_NORMALIZER_NAME
from vins_core.dm.session import Session
from vins_core.ext.entitysearch import EntitySearchHTTPAPI
from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.nlu.sample_processors.registry import create_sample_processor
from vins_core.nlu.sample_processors.base import BaseSampleProcessor
from vins_core.nlu.sample_processors.misspell import MisspellSamplesProcessor
from vins_core.nlu.sample_processors.wizard import WizardSampleProcessor
from vins_core.utils.misc import ParallelItemError
from vins_core.ext.wizard_api import WizardHTTPAPI


# Test base class methods.


class IdentitySampleProcessor(BaseSampleProcessor):
    NAME = 'identity'

    @property
    def is_normalizing(self):
        return True

    def _process(self, sample, *args, **kwargs):
        return sample


@pytest.fixture(scope='module')
def identity_sp():
    return IdentitySampleProcessor()


@pytest.fixture(scope='module')
def dummy_items(nlu_demo_data):
    return FuzzyNLUFormat.parse_iter(sum(nlu_demo_data.itervalues(), [])).items


@pytest.fixture
def session():
    return Session(app_id='123', uuid=uuid4())


@pytest.fixture
def test_sample():
    return Sample.from_string('un sample por prueba')


def test_identity_sample_processor(identity_sp, test_sample):
    old_sample = copy.deepcopy(test_sample)
    new_sample = identity_sp(test_sample)

    assert old_sample == new_sample


# Test misspell processor.

class TestMisspellProcessor(object):
    @pytest.fixture(scope='class')
    def misspell_extractor(self):
        return SamplesExtractor([create_sample_processor('misspell')], allow_wizard_request=True)

    @classmethod
    def _spell_mocked(cls, answer):
        mock = requests_mock.mock()
        mock.get(MisspellSamplesProcessor.URL, json=answer)

        return mock

    @pytest.mark.parametrize('utt,reference_utt,mock_answer', [
        ('слыш ты', 'слыш ты', {'text': 'слыш\xad(ь)\xad ты', 'r': 8000}),
        ('свинка пепа', 'свинка пеппа', {'text': 'свинка пе\xad(пп)\xadа', 'r': 10000}),
        ('владимир путни', 'владимир путин', {'text': 'владимир пут\xad(ин)\xad', 'r': 10000}),
        ('влдимир путни', 'владимир путин', {'text': 'вл\xad(а)\xadдимир пут\xad(ин)\xad', 'r': 10000}),
        ('влдмир путни', 'владимир путин', {'text': 'вл\xad(а)\xadд\xad(и)\xadмир пут\xad(ин)\xad', 'r': 10000}),
        ('не, серьезно', 'не, серьезно', {'text': 'н\xad(ес)\xaddерьезно', 'r': 8000}),
        ('найди парк поблизости', 'найди парк поблизости', {'text': 'найди пар\xad(у)\xad поблизости', 'r': 8000}),
        ('найди аптеку поблизости', 'найди аптеку поблизости', {'text': 'най\xad(т)\xadи аптеку поблизости', 'r': 8000}),  # noqa
        ('погода назавтра', 'погода на завтра', {'text': 'погода на завтра', 'r': 10000})
    ])
    def test_misspell_processor(self, misspell_extractor, utt, reference_utt, mock_answer):
        with self._spell_mocked(mock_answer):
            # Test if `utt` is string.
            new_utt = misspell_extractor([utt])[0]
            assert new_utt.text == reference_utt

            # Test if `utt` is embedded in Utterance with input_source='text'
            new_utt = misspell_extractor([Utterance(utt, input_source=Utterance.TEXT_INPUT_SOURCE)])[0]
            assert new_utt.text == reference_utt

            # Test if `utt` is embedded in Utterance with input_source='voice'
            # In this case utterance should not be changed, so equal to original `utt`.
            new_utt = misspell_extractor([Utterance(utt, input_source=Utterance.VOICE_INPUT_SOURCE)])[0]
            assert new_utt.text == utt


class TestClip(object):

    @pytest.fixture(scope='class')
    def clip_extractor(self):
        return SamplesExtractor([create_sample_processor('clip', max_tokens=3)], allow_wizard_request=True)

    @pytest.mark.parametrize('input_source', (Utterance.TEXT_INPUT_SOURCE, Utterance.VOICE_INPUT_SOURCE))
    def test_input_sources_unchanged(self, clip_extractor, input_source):
        utt = '1 2 3 4 5'
        input_utterance = Utterance(utt, input_source=input_source)
        new_utt = clip_extractor([input_utterance])[0]
        assert new_utt.text == '1 2 3'
        assert new_utt.utterance == input_utterance


class TestStripActivationsProcessor(object):
    @pytest.fixture(scope='class')
    def extractor(self):
        return SamplesExtractor([create_sample_processor('strip_activation')], allow_wizard_request=True)

    @pytest.fixture(scope='class')
    def custom_extractor(self):
        return SamplesExtractor(
            [
                create_sample_processor(
                    'strip_activation',
                    custom_front_activations=(
                        'алиса', 'эй алиса', 'слушай алиса', 'привет алиса'
                    ),
                    custom_back_activations=(
                        'алиса',
                    )
                )
            ],
            allow_wizard_request=True
        )

    @pytest.mark.parametrize('utt,normalized_utt,use_custom_activations', [
        ('едем в сад', 'едем в сад', 0),
        ('привет яндекс едем в сад', 'едем в сад', 0),
        ('хэй яндекс едем в сад', 'едем в сад', 0),
        ('ok google едем в сад', 'едем в сад', 0),
        ('едем в привет яндекс', 'едем в привет яндекс', 0),
        ('едем в яндекс или в сад', 'едем в яндекс или в сад', 0),
        ('едем в ok google или в сад', 'едем в ok google или в сад', 0),
        ('алиса едем в сад', 'алиса едем в сад', 0),
        ('едем в сад яндекс', 'едем в сад яндекс', 0),
        ('едем в сад окей яндекс', 'едем в сад окей яндекс', 0),
        ('едем в сад слушай яндекс пожалуйста', 'едем в сад слушай яндекс пожалуйста', 0),
        ('едем в сад алиса', 'едем в сад алиса', 0),
        ('окей яндекс', 'окей яндекс', 0),
        ('алиса едем в сад', 'едем в сад', 1),
        ('едем в сад алиса', 'едем в сад', 1),
        ('алиса', 'алиса', 1),
        ('окей яндекс', 'окей яндекс', 1),
        ('привет алиса едем в сад', 'едем в сад', 1),
        ('эй алиса едем в сад', 'едем в сад', 1),
        ('эй едем в сад', 'эй едем в сад', 1),
        ('едем в привет яндекс', 'едем в привет яндекс', 1),
        ('едем в ok google или в сад', 'едем в ok google или в сад', 1),
    ])
    def test_strip_activation_processor(self, extractor, custom_extractor, utt, normalized_utt, use_custom_activations):
        extractor = custom_extractor if use_custom_activations else extractor
        # Activation is not stripped for text input by default
        new_utt = extractor([Utterance(utt, input_source=Utterance.TEXT_INPUT_SOURCE)])[0]
        assert new_utt.text == utt

        # But it is stripped for voice
        new_utt = extractor([Utterance(utt, input_source=Utterance.VOICE_INPUT_SOURCE)])[0]
        assert new_utt.text == normalized_utt


class TestEntitySearchSampleProcessor(object):
    @pytest.fixture(scope='class')
    def processor(self):
        return create_sample_processor('entitysearch')

    @classmethod
    def _make_card_and_entity(cls, id, name, type, tags=None, subtype=None, start=None, end=None):
        card = {
            'base_info': {
                'type': type,
                'name': name,
                'id': id
            }
        }
        if tags:
            card['tags'] = tags
        if subtype:
            card['base_info']['wsubtype'] = [subtype + '@on']

        if subtype:
            ent_type = type + '/' + subtype
        else:
            ent_type = type

        return card, Entity(
            id=id,
            tags=tags,
            name=name,
            type=ent_type,
            start=start,
            end=end
        )

    @classmethod
    def _entitysearch_mocked(cls, cards):
        mock = requests_mock.Mocker()
        mock.get(EntitySearchHTTPAPI.ENTITYSEARCH_URL, json={'cards': cards})

        return mock

    @classmethod
    def _sample_with_wizard(cls, text, entity_infos=None):
        sample = Sample.from_string(text)

        if entity_infos is not None:
            rules = {
                'EntityFinder': {
                    'Winner': ['data\t{}\t{}\t{}\tother\tdata'.format(start, end, id) for (start, end, id) in entity_infos]  # noqa
                }
            }
            sample.annotations['wizard'] = WizardAnnotation(markup=None, rules=rules)

        return sample

    def test_annotation_added_on_inference(self, processor):
        sample = self._sample_with_wizard('кто такой высоцкий', entity_infos=[(2, 3, 'ruw24738')])
        card, entity = self._make_card_and_entity(
            id='ruw24738',
            name='Владимир Высоцкий',
            type='Hum',
            tags=['Actor', 'Musician', 'Creative', 'Writer'],
            start=2,
            end=3
        )
        with self._entitysearch_mocked([card]):
            new_sample = processor(sample, None, is_inference=True)
            assert 'entitysearch' in new_sample.annotations
            assert new_sample.annotations['entitysearch'].entities == [entity]

    def test_annotation_not_added_not_inference(self, processor):
        sample = self._sample_with_wizard('кто такой высоцкий', entity_infos=[(2, 3, 'ruw24738')])
        card, entity = self._make_card_and_entity(
            id='ruw24738',
            name='Владимир Высоцкий',
            type='Hum',
            tags=['Actor', 'Musician', 'Creative', 'Writer'],
            start=2,
            end=3
        )
        with self._entitysearch_mocked([card]):
            new_sample = processor(sample, None, is_inference=False)
            assert 'entitysearch' not in new_sample.annotations

    def test_geo_entity(self, processor):
        sample = self._sample_with_wizard('где кинешма находится', entity_infos=[(1, 2, 'ruw172758')])
        card, entity = self._make_card_and_entity(
            id='ruw172758',
            name='Кинешма',
            type='Geo',
            tags=['City'],
            subtype='Locality',
            start=1,
            end=2
        )

        with self._entitysearch_mocked([card]):
            new_sample = processor(sample, None)
            assert 'entitysearch' in new_sample.annotations
            assert new_sample.annotations['entitysearch'].entities == [entity]


class TestMultiprocessing(object):

    @pytest.fixture(scope='function')
    def extractor(self):
        return SamplesExtractor([create_sample_processor(type_, **kwargs) for type_, kwargs in (
            ('expected_value', {}),
            ('strip_activation', {}),
            ('normalizer', {'normalizer': DEFAULT_RU_NORMALIZER_NAME})
        )], allow_wizard_request=True)

    def test_consistent_run(self, extractor, dummy_items):
        assert extractor(dummy_items, num_procs=1) == extractor(dummy_items, num_procs=2)

    class ExceptionSampleProcessor(BaseSampleProcessor):
        @property
        def is_normalizing(self):
            return True

        def _process(self, sample, session, *args, **kwargs):
            if sample.text.startswith('привет'):
                raise ValueError('test exception')
            return sample

    def test_failure_singleproc(self, extractor, dummy_items):

        extractor.add(self.ExceptionSampleProcessor())

        with pytest.raises(ValueError):
            extractor(dummy_items)

    def test_failure_multiproc(self, extractor, dummy_items):

        extractor.add(self.ExceptionSampleProcessor())

        with pytest.raises(ValueError):
            extractor(dummy_items, num_procs=2)

    def test_failure_multiproc_parallel_item_error(self, extractor, dummy_items):

        extractor.add(self.ExceptionSampleProcessor())
        samples = extractor(dummy_items, num_procs=2, raise_on_error=False)
        assert len(samples) == len(dummy_items)
        parallel_item_errors = [sample for sample in samples if isinstance(sample, ParallelItemError)]
        assert len(parallel_item_errors) == 2
        assert all(e.message == 'test exception' for e in parallel_item_errors)


@attr.s
class ExampleAnnotation(BaseAnnotation):
    pass


register_annotation(ExampleAnnotation, 'example')


class TestInferenceFlag(object):
    class AddAnnotationOnInference(BaseSampleProcessor):
        @property
        def is_normalizing(self):
            return False

        def _process(self, sample, session, is_inference, *args, **kwargs):
            if is_inference:
                sample.annotations['inference_annotation'] = ExampleAnnotation()
            return sample

    class AddAnnotationAlways(BaseSampleProcessor):
        @property
        def is_normalizing(self):
            return False

        def _process(self, sample, session, *args, **kwargs):
            sample.annotations['always_annotation'] = ExampleAnnotation()
            return sample

    def test_inference_flag(self):
        extractor = SamplesExtractor(pipeline=[self.AddAnnotationAlways(), self.AddAnnotationOnInference()],
                                     allow_wizard_request=True)

        sample = extractor(['test sample'], is_inference=True)[0]
        assert set(sample.annotations.keys()) == {'inference_annotation', 'always_annotation'}

        sample = extractor(['test sample'])[0]
        assert set(sample.annotations.keys()) == {'inference_annotation', 'always_annotation'}

        sample = extractor(['test sample'], is_inference=False)[0]
        assert set(sample.annotations.keys()) == {'always_annotation'}


class TestGranetSampleExtraction(object):
    @classmethod
    def _mocked(cls, misspell_answer, wizard_answer):
        m = requests_mock.mock()
        content = json.dumps(wizard_answer, ensure_ascii=False).encode('utf-8')
        m.get(WizardHTTPAPI.WIZARD_URL, content=content)
        m.get(MisspellSamplesProcessor.URL, json=misspell_answer)
        return m

    @pytest.mark.parametrize('utterance, misspell_answer, expected_granet_input, expected_wizard_input, max_tokens', [
        ('алиса павтори-ка за мной один алиса some trash', 'алиса повтори-ка за мной один алиса',
         'алиса повтори-ка за мной один алиса', 'повтори-ка за мной 1', 5),
        ('повтори за мной 1 2 3 4 5 6 7 8 9 10', 'повтори за мной 1 2 3 4 5 6 7 8 9 10',
         'повтори за мной 1 2 3 4 5 6 7 8 9 10', 'повтори за мной 1(234)567-89-10', 100)
    ])
    def test_preprocessing_pipeline(self, utterance, misspell_answer, expected_granet_input,
                                    expected_wizard_input, max_tokens):
        samples_extractor = SamplesExtractor.from_config(
        {
            'pipeline': [
                {'name': 'clip', 'max_tokens': max_tokens},
                {'name': 'misspell'},
                {'normalizer': 'normalizer_ru', 'name': 'normalizer'},
                {
                    'apply_to_text_input': True,
                    'name': 'strip_activation',
                    'custom_back_activations': ['алиса'],
                    'custom_front_activations': ['алиса']
                },
                {'name': 'wizard'}
            ],
            'allow_wizard_request': True
        })

        misspell_answer = {'text': misspell_answer, 'r': 10000}
        with self._mocked(misspell_answer, {}) as mock:
            sample = samples_extractor([utterance])[0]

        assert sample.text == expected_wizard_input
        assert sample.partially_normalized_text == expected_granet_input

        wizard_request = mock.last_request.qs
        assert wizard_request['text'][0].decode('utf8') == expected_wizard_input
        assert any(extra.decode('utf8') == 'alice_original_text=' + expected_granet_input
                   for extra in wizard_request['wizextra'])


@pytest.mark.parametrize('sample_tokens, markup, expected_alignment', [
    (  # a token occurs for several times
        ['на', 'будильник', 'на', 'повторе'],
        {
            'Delimiters': [
                {},
                {'EndChar': 3, 'Text': ' ', 'EndByte': 5, 'BeginByte': 4, 'BeginChar': 2},
                {'EndChar': 13, 'Text': ' ', 'EndByte': 24, 'BeginByte': 23, 'BeginChar': 12},
                {'EndChar': 16, 'Text': ' ', 'EndByte': 29, 'BeginByte': 28, 'BeginChar': 15},
                {}
            ], 'Tokens': [
                {'EndChar': 2, 'Text': 'на', 'EndByte': 4, 'BeginByte': 0, 'BeginChar': 0},
                {'EndChar': 12, 'Text': 'будильник', 'EndByte': 23, 'BeginByte': 5, 'BeginChar': 3},
                {'EndChar': 15, 'Text': 'на', 'EndByte': 28, 'BeginByte': 24, 'BeginChar': 13},
                {'EndChar': 23, 'Text': 'повторе', 'EndByte': 43, 'BeginByte': 29, 'BeginChar': 16}
            ]
        },
        [0, 1, 2, 3]
    ),
    (  # wizard splits a token with empty delimiter
        ['самсунг', 'галакси', 'а5', '2017'],
        {
            'Delimiters': [
                {},
                {'EndChar': 8, 'Text': ' ', 'EndByte': 15, 'BeginByte': 14, 'BeginChar': 7},
                {'EndChar': 16, 'Text': ' ', 'EndByte': 30, 'BeginByte': 29, 'BeginChar': 15},
                {},
                {'EndChar': 19, 'Text': ' ', 'EndByte': 34, 'BeginByte': 33, 'BeginChar': 18},
                {}
            ], 'Tokens': [
                {'EndChar': 7, 'Text': 'самсунг', 'EndByte': 14, 'BeginByte': 0, 'BeginChar': 0},
                {'EndChar': 15, 'Text': 'галакси', 'EndByte': 29, 'BeginByte': 15, 'BeginChar': 8},
                {'EndChar': 17, 'Text': 'а', 'EndByte': 32, 'BeginByte': 30, 'BeginChar': 16},
                {'EndChar': 18, 'Text': '5', 'EndByte': 33, 'BeginByte': 32, 'BeginChar': 17},
                {'EndChar': 23, 'Text': '2017', 'EndByte': 38, 'BeginByte': 34, 'BeginChar': 19}
            ]
        },
        [0, 1, 2, 2, 3]
    ),
    (  # wizard treats token as a delimiter
        ['жургенова',  '28', '/',  '1'],
        {
            'Delimiters': [
                {},
                {'EndChar': 10, 'Text': ' ', 'EndByte': 19, 'BeginByte': 18, 'BeginChar': 9},
                {'EndChar': 15, 'Text': ' / ', 'EndByte': 24, 'BeginByte': 21, 'BeginChar': 12},
                {}
            ], 'Tokens': [
                {'EndChar': 9, 'Text': 'жургенова', 'EndByte': 18, 'BeginByte': 0, 'BeginChar': 0},
                {'EndChar': 12, 'Text': '28', 'EndByte': 21, 'BeginByte': 19, 'BeginChar': 10},
                {'EndChar': 16, 'Text': '1', 'EndByte': 25, 'BeginByte': 24, 'BeginChar': 15}
            ]
        },
        [0, 1, 3]
    ), (  # delimiter contains non-space character after space
        ['номер', '(123)456-78-90'],
        {
            'Delimiters': [
                {},
                {'EndChar': 7, 'Text': ' (', 'EndByte': 12, 'BeginByte': 10, 'BeginChar': 5},
                {'EndChar': 11, 'Text': ')', 'EndByte': 16, 'BeginByte': 15, 'BeginChar': 10},
                {'EndChar': 15, 'Text': '-', 'EndByte': 20, 'BeginByte': 19, 'BeginChar': 14},
                {'EndChar': 18, 'Text': '-', 'EndByte': 23, 'BeginByte': 22, 'BeginChar': 17},
                {}
            ], 'Tokens': [
                {'EndChar': 5, 'Text': 'номер', 'EndByte': 10, 'BeginByte': 0, 'BeginChar': 0},
                {'EndChar': 10, 'Text': '123', 'EndByte': 15, 'BeginByte': 12, 'BeginChar': 7},
                {'EndChar': 14, 'Text': '456', 'EndByte': 19, 'BeginByte': 16, 'BeginChar': 11},
                {'EndChar': 17, 'Text': '78', 'EndByte': 22, 'BeginByte': 20, 'BeginChar': 15},
                {'EndChar': 20, 'Text': '90', 'EndByte': 25, 'BeginByte': 23, 'BeginChar': 18}
            ]
        },
        [0, 1, 1, 1, 1]
    ),
    (  # text starts with a delimiter
        ['(123)456-78-90'],
        {
            'Delimiters': [
                {'EndChar': 1, 'Text': '(', 'EndByte': 1, 'BeginByte': 0, 'BeginChar': 0},
                {'EndChar': 5, 'Text': ')', 'EndByte': 5, 'BeginByte': 4, 'BeginChar': 4},
                {'EndChar': 9, 'Text': '-', 'EndByte': 9, 'BeginByte': 8, 'BeginChar': 8},
                {'EndChar': 12, 'Text': '-', 'EndByte': 12, 'BeginByte': 11, 'BeginChar': 11}, {}
            ], 'Tokens': [
                {'EndChar': 4, 'Text': '123', 'EndByte': 4, 'BeginByte': 1, 'BeginChar': 1},
                {'EndChar': 8, 'Text': '456', 'EndByte': 8, 'BeginByte': 5, 'BeginChar': 5},
                {'EndChar': 11, 'Text': '78', 'EndByte': 11, 'BeginByte': 9, 'BeginChar': 9},
                {'EndChar': 14, 'Text': '90', 'EndByte': 14, 'BeginByte': 12, 'BeginChar': 12}
            ]
        },
        [0, 0, 0, 0]
    ),
    (  # first vins token is wizard delimiter
        ['+', '1234567890'],
        {
            'Delimiters': [
                {'EndChar': 2, 'Text': '+ ', 'EndByte': 2, 'BeginByte': 0, 'BeginChar': 0},
                {}
            ], 'Tokens': [
                {'EndChar': 12, 'Text': '1234567890', 'EndByte': 12, 'BeginByte': 2, 'BeginChar': 2}
            ]
        },
        [1]
    )
])
def test_wizard_token_alignment(sample_tokens, markup, expected_alignment):
    alignment = WizardSampleProcessor.align_tokens(markup, sample_tokens)
    assert alignment == expected_alignment
