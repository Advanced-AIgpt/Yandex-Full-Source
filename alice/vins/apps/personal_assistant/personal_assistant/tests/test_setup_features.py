# -*- coding: utf-8 -*-

import pytest
import numpy as np

from uuid import uuid4

from personal_assistant.bass_result import BassFormSetupMeta
from personal_assistant.setup_features import get_setup_features
from personal_assistant.app import FormSetup, PersonalAssistantApp
from vins_core.dm.form_filler.models import Form
from personal_assistant.setup_feature_updater import BassSetupFeatureUpdater
from vins_core.dm.form_filler.form_candidate import FormCandidate
from vins_core.nlu.intent_candidate import IntentCandidate
from vins_core.dm.request import create_request
from vins_core.common.sample import Sample
from vins_core.nlu.features.base import SampleFeatures

from itertools import izip


def video_setup_meta():
    return {
        "factors_data": {
            "video_web_search": {
                "documents": [
                    {
                        "url": "https://zvooq.online/collections/games/ost-final-fantasy-xv",
                        "pos": 0,
                        "markers": {
                            "RelevPrediction": "0.100544666132",
                            "DomainHasMetrika": "1",
                            "Handle": "10-0-ZCDAD9263E2FCC27A"
                        },
                        "doctitle": "Музыка из игры Final Fantasy XV в MP3 - скачать бесплатно, слушать саундтрек из игры Final Fantasy XV - 96 песен/песни онлайн на Zvooq.online!",  # noqa: F501
                        "host": "zvooq.online",
                        "relevance": "105328432"
                    },
                    {
                        "url": "https://myzcloud.me/album/3553165/final-fantasy-xv-ost-2016",
                        "pos": 2,
                        "markers": {
                            "RelevPrediction": "0.088148013413",
                            "DomainHasMetrika": "1",
                            "Handle": "15-0-ZA54F329B5DA18B47"
                        },
                        "doctitle": "FINAL FANTASY XV - OST (2016) - Скачать саундтрек бесплатно и слушать онлайн в mp3 - Myzcloud.me",  # noqa: F501
                        "host": "myzcloud.me",
                        "relevance": "104685680"
                    },
                    {
                        "url": "https://www.youtube.com/playlist?list=PLw942RSR38BoCxyYkf0S5j03KqBUtw5yk",
                        "pos": 3,
                        "markers": {
                            "RelevPrediction": "0.097482253593",
                            "OwnerWebsiteAttention": "1",
                            "Handle": "12-3-Z1DE512583ED0F278",
                            "HostPlayerViewDepth": "0.4287"
                        },
                        "doctitle": "Final Fantasy XV OST - YouTube",
                        "host": "www.youtube.com",
                        "relevance": "104583248"
                    },
                    {
                        "url": "https://my.mail.ru/music/search/FINAL%20FANTASY%20XV%20OST",
                        "pos": 5,
                        "markers": {
                            "RelevPrediction": "0.111481151409",
                            "Handle": "10-13-Z6C1AB0B9E7FA3217"
                        },
                        "doctitle": "FINAL FANTASY XV OST слушать онлайн. Музыка Mail.Ru",
                        "host": "my.mail.ru",
                        "relevance": "104364128"
                    },
                    {
                        "url": "https://muzofond.fm/search/final%20fantasy%2015",
                        "pos": 6,
                        "markers": {
                            "RelevPrediction": "0.047664377881",
                            "DomainHasMetrika": "1",
                            "Handle": "6-13-2-ZFC5D69E79252B282"
                        },
                        "doctitle": "final fantasy 15 MP3 скачать бесплатно, слушать музыку final fantasy 15 - 2 песен/песни онлайн",  # noqa: F501
                        "host": "muzofond.fm",
                        "relevance": "103608192"
                    },
                    {
                        "url": "http://www.game-ost.ru/albums/105083/final_fantasy_xv_original_soundtrack",
                        "pos": 9,
                        "markers": {
                            "RelevPrediction": "0.129497464839",
                            "DomainHasMetrika": "1",
                            "Handle": "6-7-11-Z30DDC591FAEBF43C"
                        },
                        "doctitle": "FINAL FANTASY XV музыка из игры | FINAL FANTASY XV Original Soundtrack",
                        "host": "www.game-ost.ru",
                        "relevance": "104316040"
                    },
                    {
                        "url": "http://buben.fm/a-pesni-iz-igry-final-fantasy-xv-wrzmowp",
                        "pos": 10,
                        "markers": {
                            "RelevPrediction": "0.096851426885",
                            "Handle": "16-16-Z5DDB95AB867D71FF"
                        },
                        "doctitle": "Песни Из Игры \"Final Fantasy Xv\" – Скачать бесплатно и слушать онлайн – Buben.fm",
                        "host": "buben.fm",
                        "relevance": "104550136"
                    },
                    {
                        "url": "http://zaycev.net/pages/44121/4412135.shtml",
                        "pos": 11,
                        "markers": {
                            "RelevPrediction": "0.072852754815",
                            "DomainHasMetrika": "1",
                            "Handle": "16-8-ZF4557DAAE619E615"
                        },
                        "doctitle": "Epic Rock - Fight ( OST Final Fantasy XV ) в MP3 - слушать музыку онлайн на Зайцев.нет без регистрации",  # noqa: F501
                        "host": "zaycev.net",
                        "relevance": "104235808"
                    },
                    {
                        "url": "https://zvuk.top/tracks/final-fantasy-15",
                        "pos": 12,
                        "markers": {
                            "RelevPrediction": "0.048043282445",
                            "DomainHasMetrika": "1",
                            "Handle": "6-4-13-Z80F14DE525121B07"
                        },
                        "doctitle": "final fantasy 15 музыка в MP3 - скачать бесплатно, слушать музыку final fantasy 15 - 2 песен/песни онлайн на Zvuk.top!",  # noqa: F501
                        "host": "zvuk.top",
                        "relevance": "103844632"
                    },
                    {
                        "url": "http://xn--80adhccsnv2afbpk.xn--p1ai/saundtrek-k-igre/6420-2016-final-fantasy-xv.html",
                        "pos": 13,
                        "markers": {
                            "RelevPrediction": "0.092183831943",
                            "Handle": "14-7-Z6084C178ACC2B647"
                        },
                        "doctitle": "Саундтрек к игре \"Final Fantasy XV\" | Вся музыка и песни из игры \"Final Fantasy XV\" - Final Fantasy XV. Live at Abbey Road Studios (неоф. релиз)\"",  # noqa
                        "host": "xn--80adhccsnv2afbpk.xn--p1ai",
                        "relevance": "104199880"
                    }
                ],
                "wizards": {
                    "musicplayer": {
                        "document": {
                            "doctitle": "Yoko Shimomura Final Fantasy XV",
                            "relevance": "114328712",
                            "markers": {
                                "WizardPos": "1",
                                "Rule": "Vertical/dup"
                            }
                        },
                        "show": True
                    },
                    "videowiz": {
                        "document": {
                            "doctitle": "Yandex.Video",
                            "relevance": "-1",
                            "markers": {
                                "blndrViewType": "vital",
                                "WizardPos": "7",
                                "Rule": "Vertical/video_rus"
                            }
                        },
                        "show": True
                    }
                },
                "snippets": {
                    "entity_search": {
                        "found": True,
                        "data": {
                            "base_info": {
                                "type": "Soft",
                                "ids": {
                                    "kinopoisk": None
                                },
                                "title": "Final Fantasy XV"
                            }
                        }
                    }
                }
            }
        },
        "is_feasible": True
    }


@pytest.fixture(name='video_setup_meta')
def video_setup_meta_fixture():
    return video_setup_meta()


def music_setup_meta():
    return {
        "factors_data": {
            "music_web_search": {
                "wizards": {
                    "musicplayer": {
                        "document": {
                            "doctitle": "The Beatles — слушать онлайн на Яндекс.Музыке",
                            "markers": {
                                "ChatScore": "0.9953",
                                "DomainHasMetrika": "1",
                                "HostPlayerViewDepth": "0.4087",
                                "OwnerWebsiteAttention": "1",
                                "RelevPrediction": "0.247478555959",
                                "WizardPos": "0"
                            },
                            "relevance": "105110272"
                        },
                        "show": True
                    }
                }
            }
        },
        "is_feasible": True
    }


@pytest.fixture(name='music_setup_meta')
def music_setup_meta_fixture():
    return music_setup_meta()


@pytest.mark.parametrize("setup_meta,intent,expected", [
    # geo object is presented
    (video_setup_meta, 'personal_assistant.scenarios.video_play', [
        4.00000000e+00,   0.00000000e+00,   1.00000000e+00,   0.00000000e+00,
        1.00000000e+00,   0.00000000e+00,   1.00000000e+00,   0.00000000e+00,
        0.00000000e+00,   1.05328432e+08,   1.00544666e-01,   9.99000000e+02,
        0.00000000e+00,   0.00000000e+00,   9.99000000e+02,   0.00000000e+00,
        0.00000000e+00,   9.99000000e+02,   0.00000000e+00,   0.00000000e+00,
        9.99000000e+02,   0.00000000e+00,   0.00000000e+00,   9.99000000e+02,
        0.00000000e+00,   0.00000000e+00,   9.99000000e+02,   0.00000000e+00,
        0.00000000e+00,   9.99000000e+02,   0.00000000e+00,   0.00000000e+00,
        0.00000000e+00,   1.05328432e+08,   1.00544666e-01,   1.10000000e+01,
        1.04235808e+08,   7.28527548e-02,   9.99000000e+02,   0.00000000e+00,
        0.00000000e+00,   9.99000000e+02,   0.00000000e+00,   0.00000000e+00,
        9.99000000e+02,   0.00000000e+00,   0.00000000e+00,   9.99000000e+02,
        0.00000000e+00,   0.00000000e+00,   6.00000000e+00,   1.03608192e+08,
        4.76643779e-02,   3.00000000e+00,   1.04583248e+08,   9.74822536e-02,
        9.99000000e+02,   0.00000000e+00,   0.00000000e+00,   1.00000000e+00,
        1.00000000e+00,   1.14328712e+08,   1.00000000e+00,   1.00000000e+00,
        1.00000000e+00,   1.00000000e+00,   7.00000000e+00,   1.00000000e+00,
        1.00000000e+00,   0.00000000e+00,   0.00000000e+00,   0.00000000e+00,
        0.00000000e+00,   0.00000000e+00,   0.00000000e+00,   0.00000000e+00,
        0.00000000e+00,   0.00000000e+00,   0.00000000e+00,   0.00000000e+00,
        0.00000000e+00,   0.00000000e+00,   0.00000000e+00,   0.00000000e+00,
        0.00000000e+00,   1.00000000e+00,   0.00000000e+00
    ]),
    (music_setup_meta, 'personal_assistant.scenarios.music_play', [0, 1, 0, 105110272, 0.247478555959])
])
def test_extracting_setup_features(setup_meta, intent, expected):
    bass_form_setup_meta = BassFormSetupMeta.from_dict(setup_meta())
    feature_extractor = get_setup_features(intent)
    features = feature_extractor.from_dict(bass_form_setup_meta.factors_data)

    assert np.allclose(features, expected)


def make_setup_forms(forms_info):
    def setup_forms(forms, **kwargs):
        return [
            FormSetup(
                meta=BassFormSetupMeta.from_dict(setup_meta),
                form=form,
                precomputed_data={'index': index + 1}
            )
            for index, (form, setup_meta) in enumerate(forms_info)
        ]
    return setup_forms


def make_feasible_setup_forms(feasible_form_names):
    def setup_forms(forms, **kwargs):
        return [
            FormSetup(
                meta=BassFormSetupMeta(is_feasible=(form.name in feasible_form_names)),
                form=form,
                precomputed_data={'index': index + 1}
            )
            for index, form in enumerate(forms)
        ]
    return setup_forms


def get_full_class_name(cls):
    return cls.__module__ + '.' + cls.__class__.__name__


def make_app(mocker, setup_side_effect):
    mock_app_class = mocker.patch(get_full_class_name(PersonalAssistantApp))
    app = mock_app_class.return_value
    app.setup_forms.side_effect = setup_side_effect
    return app


def update_features(form_candidates, sample_features, app):
    feature_updater = BassSetupFeatureUpdater()
    return feature_updater.update_features(app,
                                           sample_features=sample_features,
                                           session=None,
                                           req_info=create_request(str(uuid4()),
                                                                   experiments=['bass_setup_features']),
                                           form_candidates=form_candidates)


def test_setup_feature_updater(mocker, video_setup_meta, music_setup_meta):
    forms = [Form('personal_assistant.scenarios.video_play'), Form('personal_assistant.scenarios.music_play')]
    setup_meta = [video_setup_meta, music_setup_meta]
    form_candidates = [FormCandidate(form=form, intent=IntentCandidate(form.name)) for form in forms]

    forms_info = zip(forms, setup_meta)

    app = make_app(mocker, make_setup_forms(forms_info))
    sample_features = SampleFeatures(Sample.from_string('вкличи слпин'))
    updater_result = update_features(form_candidates, sample_features, app)

    sample_features = updater_result.sample_features

    assert 'video_play_setup' in sample_features.dense
    assert np.allclose(
        sample_features.dense['video_play_setup'],
        [
            4.00000000e+00,   0.00000000e+00,   1.00000000e+00,   0.00000000e+00,
            1.00000000e+00,   0.00000000e+00,   1.00000000e+00,   0.00000000e+00,
            0.00000000e+00,   1.05328432e+08,   1.00544666e-01,   9.99000000e+02,
            0.00000000e+00,   0.00000000e+00,   9.99000000e+02,   0.00000000e+00,
            0.00000000e+00,   9.99000000e+02,   0.00000000e+00,   0.00000000e+00,
            9.99000000e+02,   0.00000000e+00,   0.00000000e+00,   9.99000000e+02,
            0.00000000e+00,   0.00000000e+00,   9.99000000e+02,   0.00000000e+00,
            0.00000000e+00,   9.99000000e+02,   0.00000000e+00,   0.00000000e+00,
            0.00000000e+00,   1.05328432e+08,   1.00544666e-01,   1.10000000e+01,
            1.04235808e+08,   7.28527548e-02,   9.99000000e+02,   0.00000000e+00,
            0.00000000e+00,   9.99000000e+02,   0.00000000e+00,   0.00000000e+00,
            9.99000000e+02,   0.00000000e+00,   0.00000000e+00,   9.99000000e+02,
            0.00000000e+00,   0.00000000e+00,   6.00000000e+00,   1.03608192e+08,
            4.76643779e-02,   3.00000000e+00,   1.04583248e+08,   9.74822536e-02,
            9.99000000e+02,   0.00000000e+00,   0.00000000e+00,   1.00000000e+00,
            1.00000000e+00,   1.14328712e+08,   1.00000000e+00,   1.00000000e+00,
            1.00000000e+00,   1.00000000e+00,   7.00000000e+00,   1.00000000e+00,
            1.00000000e+00,   0.00000000e+00,   0.00000000e+00,   0.00000000e+00,
            0.00000000e+00,   0.00000000e+00,   0.00000000e+00,   0.00000000e+00,
            0.00000000e+00,   0.00000000e+00,   0.00000000e+00,   0.00000000e+00,
            0.00000000e+00,   0.00000000e+00,   0.00000000e+00,   0.00000000e+00,
            0.00000000e+00,   1.00000000e+00,   0.00000000e+00
        ]
    )
    assert 'music_play_setup' in sample_features.dense
    assert np.allclose(sample_features.dense['music_play_setup'], [0, 1, 0, 105110272, 0.247478555959])


def test_setup_feature_updater_with_feasible_forms(mocker):
    form_candidates = [
        FormCandidate(Form('form_1'), IntentCandidate('intent_1')),
        FormCandidate(Form('form_2'), IntentCandidate('intent_2')),
        FormCandidate(Form('form_3'), IntentCandidate('intent_3')),
    ]
    app = make_app(mocker, make_feasible_setup_forms(feasible_form_names=['form_2', 'form_3']))
    sample_features = SampleFeatures(Sample.from_string('test'))
    updater_result = update_features(form_candidates, sample_features, app)

    assert len(updater_result.form_candidates) == len(form_candidates[1:])
    for result_form, candidate_form in izip(updater_result.form_candidates, form_candidates[1:]):
        assert result_form.form.name == candidate_form.form.name


def test_setup_feature_updater_without_feasible_forms(mocker):
    form_candidates = [
        FormCandidate(Form('form_1'), IntentCandidate('intent_1')),
        FormCandidate(Form('form_2'), IntentCandidate('intent_2')),
        FormCandidate(Form('form_3'), IntentCandidate('intent_3')),
    ]
    app = make_app(mocker, make_feasible_setup_forms(feasible_form_names=[]))
    sample_features = SampleFeatures(Sample.from_string('test'))
    updater_result = update_features(form_candidates, sample_features, app)

    assert len(updater_result.form_candidates) == len(form_candidates)
    for result_form, candidate_form in izip(updater_result.form_candidates, form_candidates):
        assert result_form.form.name == candidate_form.form.name
