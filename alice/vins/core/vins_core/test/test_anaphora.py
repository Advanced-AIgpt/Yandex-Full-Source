# coding: utf-8
from __future__ import unicode_literals

from uuid import uuid4

import pytest
import re
import requests_mock

from vins_core.common.annotations import WizardAnnotation, AnnotationsBag
from vins_core.dm.response import VinsResponse
from vins_core.utils.data import load_data_from_file
from vins_core.dm.session import Session
from vins_core.nlu.anaphora.factory import AnaphoraResolverFactory
from vins_core.nlu.anaphora.context import AnaphoricContext
from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.nlu.sample_processors.registry import create_sample_processor
from vins_core.nlu.anaphora.mention import Mention
from vins_core.ext.entitysearch import EntitySearchHTTPAPI


@pytest.fixture
def session():
    return Session(app_id='123', uuid=uuid4())


class TestAnaphoraResolver(object):

    @staticmethod
    def es_callback(request, context, base):
        entity_ids = []
        for entity in re.finditer(r'(?P<entity_id>ruw\d+)', request.path_url,
                                  flags=re.UNICODE):
            entity_ids.append(entity.group('entity_id'))
        return {'cards': [base[entity_id] for entity_id in entity_ids if entity_id in base]}

    @pytest.fixture(scope="class")
    def entitysearch_base(self):
        return load_data_from_file('vins_core/test/test_data/entitysearch_base.json')

    @pytest.fixture(scope='class')
    def entitysearch_mock(self, entitysearch_base):
        mock = requests_mock.Mocker(real_http=True)
        mock.get(
            EntitySearchHTTPAPI.ENTITYSEARCH_URL,
            json=lambda request, context: TestAnaphoraResolver.es_callback(request, context, entitysearch_base)
        )

        return mock

    @staticmethod
    def _create_samples_extractor():
        pipeline = [create_sample_processor('wizard'), create_sample_processor('entitysearch')]
        return SamplesExtractor(pipeline=pipeline, allow_wizard_request=True)

    @pytest.fixture(scope='class')
    def anaphora_proc(self):
        samples_extractor = self._create_samples_extractor()
        factory = AnaphoraResolverFactory()
        factory.set_samples_extractor(samples_extractor)

        return factory.create_resolver({
            'matcher': {
                'name': 'simple'
            }
        })

    @pytest.fixture(scope='class',
                    params=[
                        {
                            'name': 'catboost',
                            'border': 0.7587,
                            'rank_border': 0.3324,
                            'rank_model_id': 'resource://anaphora/anaphora_model/rank.bin',
                            'border_model_id': 'resource://anaphora/anaphora_model/border.bin',
                        },
                        {
                            'name': 'simple'
                        }
                    ])
    def es_anaphora_proc(self, request):
        samples_extractor = self._create_samples_extractor()
        factory = AnaphoraResolverFactory()
        factory.set_samples_extractor(samples_extractor)
        model_config = request.param
        return factory.create_resolver({
            'matcher': model_config,
            'resolver': {
                'with_entitysearch': True
            }
        })

    @pytest.fixture(scope='class')
    def wiz_samples_extractor(self):
        pipeline = [create_sample_processor('wizard')]
        return SamplesExtractor(pipeline=pipeline, allow_wizard_request=True)

    @pytest.fixture(scope='class')
    def es_wiz_samples_extractor(self):
        return self._create_samples_extractor()

    def _process_anaphora(self, anaphora_proc, session, utts, samples_extractor):
        if not session.annotations:
            session.annotations = AnnotationsBag()
        # Process context and fill dialog history with it.
        for s in utts[:-1]:
            wizzard_processed = samples_extractor([s])[0]
            anaphora_proc(wizzard_processed, session)
            session.annotations.update(wizzard_processed.annotations)
            session.dialog_history.add(wizzard_processed.utterance, VinsResponse(),
                                       annotations=session.annotations.copy())
            session.annotations.clear()
        wizzard_processed = samples_extractor([utts[-1]])[0]
        resolved_final = anaphora_proc(wizzard_processed, session)
        return resolved_final

    def _test_anaphora(self, anaphora_proc, session, utts, reference_utt, samples_extractor):
        resolved_string = self._process_anaphora(anaphora_proc, session, utts, samples_extractor)
        if not resolved_string:
            resolved_string = utts[-1]

        assert resolved_string.lower() == reference_utt.lower()

    @pytest.mark.parametrize('utts,reference_utt', [
        (['расскажи про глубокое обучение', 'оно сильно развито?'], 'глубокое обучение сильно развито?'),
        (['что такое глубокое обучение', 'оно сильно развито?'], 'глубокое обучение сильно развито?'),
        (['скажи высоту эвереста', 'дата первого восхождения на него'], 'дата первого восхождения на эверест'),
        (['загадочная история бенджамина баттона фильм 2009', 'кто в нем режиссер'], 'кто в загадочной истории бенджамина баттона фильме режиссер'),  # noqa
        (['санкт-петербург крайне прекрасен', 'кто его основал?'], 'кто санкт петербург основал?'),
        (['кто был  отец пастернака', 'он писал романы?'], 'отец пастернака писал романы?'),
        (['расскажи про Борю', 'А сколько ему лет?'], 'А сколько боре лет?')

    ])
    def test_anaphora_processor(self, anaphora_proc, session, utts, reference_utt, wiz_samples_extractor):
        self._test_anaphora(anaphora_proc, session, utts, reference_utt, wiz_samples_extractor)

    @pytest.mark.parametrize('utts,reference_utt', [
        (['кто такой навальный', 'сколько ему лет?'], 'сколько Алексею Навальному лет?'),
        (['расскажи про билла гейтса', 'сколько у него денег?'], 'сколько у Билла Гейтса денег?'),
        (['кто такой сократ?', 'когда он родился'], 'когда Сократ родился'),
        (['кто такой витгенштейн', 'когда он умер?'], 'когда Витгенштейн умер?'),
        (['а что такое япония?', 'а какая там завтра погода?'], 'а какая в Японии завтра погода?'),
        (['рязань', 'погода там'], 'погода в Рязани'),
        (['рязань погода', 'погода там'], 'погода в Рязани'),
        (['рязань суббота', 'погода там'], 'погода в Рязани'),
        (['привет', 'кто такой путин?', 'сколько ему лет?'], 'сколько Путину Владимиру Владимировичу лет?'),
        (['звони ментам', 'покажи новости', 'что там с крымом'], 'что там с крымом'),
        (['??', '??'], '??'),
        (['коса юлии тимошенко', 'сколько ей лет?'], 'сколько Юлии Тимошенко лет?'),
        (['кто такая кэти перри', 'сколько ей лет?'], 'сколько кэти перри лет?'),
        (['кто такой брэд питт', 'как зовут его первую жену'], 'как зовут Брэда Питта первую жену'),
        (['а кто такой сергей дружко', 'его фото с женой'], 'Сергея Дружко фото с женой'),
        (['аркадий волож', 'когда он основал яндекс'], 'когда Аркадий Юрьевич Волож основал яндекс'),
        (['кто такой оксимирон', 'спой из него что-нибудь'], 'спой из Oxxxymiron что-нибудь'),
        (['кто написал про алису селезневу', 'из какой она книжки'], 'из какой Алиса Селезнёва книжки'),
        (['столица туркмении', 'какая там валюта?'], 'какая в Туркмении валюта?'),
        (['в какой стране гетеборг', 'сколько там сейчас времени'], 'сколько в Гетеборге сейчас времени'),
        (['андерсен ганс', 'его годы жизни'], 'Ханса Кристиана Андерсена годы жизни'),
        (['меч короля артура', 'в каком году он вышел'], 'в каком году Меч короля Артура вышел'),
    ])
    def test_es_anaphora_processor(self, es_anaphora_proc, session, utts,
                                   reference_utt, es_wiz_samples_extractor, entitysearch_mock):
        with entitysearch_mock:
            self._test_anaphora(es_anaphora_proc, session, utts, reference_utt, es_wiz_samples_extractor)

    def test_anaphora_processor_doesnt_call_wizard_if_annotations_provided_in_history(self,
                                                                                      mocker,
                                                                                      anaphora_proc,
                                                                                      session,
                                                                                      samples_extractor_with_wizard):
        context = samples_extractor_with_wizard(['кто такая мадонна', 'кто ее муж'])
        sample = samples_extractor_with_wizard(['сколько ей лет'])[0]

        assert 'wizard' in sample.annotations
        assert all('wizard' in c.annotations for c in context)
        for c in context:
            session.dialog_history.add(
                c.utterance,
                VinsResponse(),
                annotations=c.annotations
            )

        from vins_core.ext.wizard_api import WizardHTTPAPI
        wizard_mock = mocker.patch.object(WizardHTTPAPI, 'get_response', return_value=None)

        resolved_string = anaphora_proc(sample, session)

        assert resolved_string == 'сколько мадонне лет'
        assert wizard_mock.call_count == 0

    def test_wizard_empty_answer(self, mocker, wiz_samples_extractor):
        utterance = "всё взорвалось"

        from vins_core.ext.wizard_api import WizardHTTPAPI
        mocker.patch.object(WizardHTTPAPI, 'get_response', return_value={})
        sample = wiz_samples_extractor([utterance])[0]
        assert sample.annotations['wizard'] == WizardAnnotation(
            {},
            {
                'AliceAnaphoraSubstitutor': {},
                'AliceNormalizer': {},
                'AliceSession': {},
                'AliceTypeParserTime': {},
                'CustomEntities': {},
                'Date': {},
                'DirtyLang': {},
                'EntityFinder': {},
                'EntitySearch': {},
                'ExternalMarkup': {},
                'Fio': {},
                'GeoAddr': {},
                'Granet': {},
                'IsNav': {},
                'MusicFeatures': {},
                'Wares': {},
            },
            []
        )

    def test_mention_extract_returns_no_mentions_if_syntax_parser_failed(self, wiz_samples_extractor):
        utterance = u'\U0001f604нуиненада'
        sample = wiz_samples_extractor([utterance])[0]
        assert Mention.parse_mentions(sample) == []

    def test_anaphora_resolver_does_not_raise_when_advpro_is_not_in_dict(self, anaphora_proc, wiz_samples_extractor):
        m = Mention.parse_mentions(wiz_samples_extractor(['тамъ'])[0])[0]  # ADVPRO
        a = Mention.parse_mentions(wiz_samples_extractor(['во дни'])[0])[0]
        anaphora_proc._matcher.match(AnaphoricContext(m, [[a]], False, ['user']))

    def test_anaphora_resolver_does_not_raise_when_advpro_is_not_in_dict2(self, es_anaphora_proc,
                                                                          wiz_samples_extractor):
        ms = es_anaphora_proc._process(wiz_samples_extractor(['тамъ'])[0],
                                       [('user', wiz_samples_extractor(['во дни'])[0])])
        for m in ms:
            es_anaphora_proc._substitute(m)

    @pytest.mark.parametrize('utts,reference_utt', [
        (['расскажи про Стива Джобса', 'когда он родился?', 'сколько ему лет??', 'сколько ему лет??', 'сколько ему лет?', 'когда он умер?'], 'когда стив джобс умер?'),  # noqa
    ])
    def test_anaphora_with_long_context(self, anaphora_proc, session, utts,
                                        reference_utt, es_wiz_samples_extractor, entitysearch_mock):
        with entitysearch_mock:
            resolved_string = self._process_anaphora(anaphora_proc, session, utts, es_wiz_samples_extractor)
        assert resolved_string == reference_utt
