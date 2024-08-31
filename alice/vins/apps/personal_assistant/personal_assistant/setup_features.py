# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import itertools
import numpy as np
import logging


logger = logging.getLogger(__name__)

_setup_features_classes = {}


def register_setup_features(cls, type_):
    assert issubclass(cls, BaseSetupFeatures)
    _setup_features_classes[type_] = cls


def get_setup_features(type_):
    return _setup_features_classes[type_]


def has_setup_features(type_):
    return type_ in _setup_features_classes


def get_by_path(data, path, default_value=None):
    keys = path.split('/')
    rv = data
    for key in keys:
        if rv is None:
            return default_value
        rv = rv.get(key)
    return rv


def path_not_null(data, path):
    return get_by_path(data, path) is not None


class BaseSetupFeatures(object):
    def __init__(self):
        pass

    @staticmethod
    def name():
        raise NotImplementedError('BaseSetupFeatures has no name')

    @staticmethod
    def from_dict(data):
        raise NotImplementedError('Extract not implemented')


class VideoSetupFeatures(BaseSetupFeatures):
    NAME = 'video_play_setup'

    _MUSIC_SITES = set([
        'zvooq.online', 'zaycev.net', 'drivemusic.me', 'megapesni.me', 'muzmo.ru', 'music.yandex.ru',
        'muzofond.fm', 'ipleer.fm',
        'myzcloud.me', 'patefon.net', 'muzlo.me', 'mp3party.net', 'luxmp3.net', 'mp3-tut.mobi', 'sefon.me',
        'www.amalgama-lab.com', '5music.ru', 'muzlain.ru', 'x-minus.me', 'qmusic.me', 'cool.dj',
        'genius.com',
        'm.z1.fm', 'mp3-muzyka.ru', 'music.xn--41a.ws', 'en.lyrsense.com', 'zoop.su', 'www.gl5.ru',
        'mvclip.ru',
        'mychords.net', 'lsdmusic.ru'
    ])

    _VIDEO_SITES = set([
        'www.kinopoisk.ru', 'www.ivi.ru', 'hdlava.me', 'OnlineMultfilmy.ru', 'filmshd.club', 'rutube.ru',
        'IceAge-mult.ru', 'kino-fs.ucoz.net', 'mega-mult.ru', 'detskie-multiki.ru', 'seasonvar.ru',
        'w6.zona.plus', 'kinobos.net', 'kinohabr.net', 'kinotan.ru', 'www.moscatalogue.net', 'filmzor.net',
        'www.film.ru', 'www.ivi.tv', 'www.kino-teatr.ru', 'cinema-24.tv', 'kinoserial.tv', 'multsforkids.ru',
        'fryd.ru', 'kinoshack.net', 'FanSerials.mobi', 'videomeg.ru', 'smotret-multiki.net', 'hd-club.su',
        'vseseriipodriad.ru', 'ctc.ru', 'mvclip.ru'
    ])

    _FEATURE_COUNT = 87
    # TODO(the0): Consider features (e.g. positional ones) having non-trivial semantics for zero value.
    #             Somehow rewrite straightforward zero-based default value policy both in training and runtime.
    _ZERO_FEATURE_VALUES = np.zeros(_FEATURE_COUNT)

    @staticmethod
    def name():
        return VideoSetupFeatures.NAME

    @classmethod
    def from_dict(cls, data):
        try:
            return cls._ZERO_FEATURE_VALUES + cls._from_dict_impl(data)
        except Exception as e:
            logger.error("Failed to process VideoSetupFeatures: %s", e, exc_info=True)
        return cls._ZERO_FEATURE_VALUES

    @classmethod
    def _is_music_site(cls, host):
        return host in cls._MUSIC_SITES

    @classmethod
    def _is_video_site(cls, host):
        return host in cls._VIDEO_SITES

    @staticmethod
    def _extract_doc_factors(doc):
        relev_prediction = 0
        if 'RelevPrediction' in doc['markers']:
            relev_prediction = float(doc['markers']['RelevPrediction'])
        return (doc['pos'], int(doc['relevance']), relev_prediction)

    @staticmethod
    def _convert_doc_factors(factors):
        if factors is None:
            return [999, 0, 0.0]  # pos, relevance, relevpredict
        else:
            return list(factors)

    @classmethod
    def _from_dict_impl(cls, data):
        music_sites_count = 0
        video_sites_count = 0

        music_in_top_1 = False
        video_in_top_1 = False
        music_in_top_3 = False
        video_in_top_3 = False
        music_in_top_5 = False
        video_in_top_5 = False

        first_music_site = None
        first_video_site = None

        video_kinopoisk_data = None
        video_ivi_data = None
        video_hdlava_data = None
        video_onlinemultfilmy_data = None
        video_filmshd_data = None
        video_rutube_data = None

        music_zvooq_data = None
        music_zaycev_data = None
        music_drivemusic_data = None
        music_megapesni_data = None
        music_muzmo_data = None
        music_yandex_data = None
        music_muzofond_data = None

        youtube_data = None
        wikipedia_data = None

        music_wizard_present = False
        music_wizard_show = False
        music_wizard_relevance = 0
        music_wizard_pos = 999

        video_wizard_present = False
        video_wizard_show = False
        video_wizard_yandex_video = False
        video_wizard_pos = 999

        entity_search_present = False
        entity_search_found = False
        entity_search_anim = False
        entity_search_auto = False
        entity_search_band = False
        entity_search_chemical_compound = False
        entity_search_device = False
        entity_search_drugs = False
        entity_search_event = False
        entity_search_film = False
        entity_search_food = False
        entity_search_geo = False
        entity_search_hum = False
        entity_search_hum1 = False
        entity_search_music = False
        entity_search_org = False
        entity_search_picture = False
        entity_search_site = False
        entity_search_soft = False
        entity_search_text = False

        if 'video_web_search' in data:
            video_web_search = data['video_web_search']
            for index, doc in enumerate(video_web_search['documents']):
                doc_factors = cls._extract_doc_factors(doc)

                if cls._is_music_site(doc['host']):
                    music_sites_count += 1

                    if index == 0:
                        music_in_top_1 = True
                    if index < 3:
                        music_in_top_3 = True
                    if index < 5:
                        music_in_top_5 = True

                    if first_music_site is None:
                        first_music_site = doc_factors

                if cls._is_video_site(doc['host']):
                    video_sites_count += 1

                    if index == 0:
                        video_in_top_1 = True
                    if index < 3:
                        video_in_top_3 = True
                    if index < 5:
                        video_in_top_5 = True

                    if first_video_site is None:
                        first_video_site = doc_factors

                if doc['host'] == 'www.kinopoisk.ru':
                    video_kinopoisk_data = doc_factors
                if doc['host'] == 'www.ivi.ru':
                    video_ivi_data = doc_factors
                if doc['host'] == 'hdlava.me':
                    video_hdlava_data = doc_factors
                if doc['host'] == 'OnlineMultfilmy.ru':
                    video_onlinemultfilmy_data = doc_factors
                if doc['host'] == 'filmshd.club':
                    video_filmshd_data = doc_factors
                if doc['host'] == 'rutube.ru':
                    video_rutube_data = doc_factors

                if doc['host'] == 'zvooq.online':
                    music_zvooq_data = doc_factors
                if doc['host'] == 'zaycev.net':
                    music_zaycev_data = doc_factors
                if doc['host'] == 'drivemusic.me':
                    music_drivemusic_data = doc_factors
                if doc['host'] == 'megapesni.me':
                    music_megapesni_data = doc_factors
                if doc['host'] == 'muzmo.ru':
                    music_muzmo_data = doc_factors
                if doc['host'] == 'music.yandex.ru':
                    music_yandex_data = doc_factors
                if doc['host'] == 'muzofond.fm':
                    music_muzofond_data = doc_factors

                if doc['host'] == 'www.youtube.com':
                    youtube_data = doc_factors
                if doc['host'] == 'ru.wikipedia.org':
                    wikipedia_data = doc_factors

            if 'wizards' in video_web_search:
                for wizard_type, value in video_web_search['wizards'].items():
                    if wizard_type == 'musicplayer':
                        music_wizard_present = True
                        music_wizard_show = value['show']
                        music_wizard_relevance = int(value['document']['relevance'])
                        music_wizard_pos = int(value['document']['markers']['WizardPos'])

                    if wizard_type == 'videowiz':
                        video_wizard_present = True
                        video_wizard_show = value['show']
                        video_wizard_yandex_video = (value['document']['doctitle'] == 'Yandex.Video')
                        video_wizard_pos = int(value['document']['markers']['WizardPos'])

            if 'snippets' in video_web_search and 'entity_search' in video_web_search['snippets']:
                entity_search_present = True
                es = video_web_search['snippets']['entity_search']
                entity_search_found = es['found']
                entity_search_anim = (es['data']['base_info']['type'] == 'Anim')
                entity_search_auto = (es['data']['base_info']['type'] == 'Auto')
                entity_search_band = (es['data']['base_info']['type'] == 'Band')
                entity_search_chemical_compound = (es['data']['base_info']['type'] == 'ChemicalCompound')
                entity_search_device = (es['data']['base_info']['type'] == 'Device')
                entity_search_drugs = (es['data']['base_info']['type'] == 'Drugs')
                entity_search_event = (es['data']['base_info']['type'] == 'Event')
                entity_search_film = (es['data']['base_info']['type'] == 'Film')
                entity_search_food = (es['data']['base_info']['type'] == 'Food')
                entity_search_geo = (es['data']['base_info']['type'] == 'Geo')
                entity_search_hum = (es['data']['base_info']['type'] == 'Hum')
                entity_search_hum1 = (es['data']['base_info']['type'] == 'Hum1')
                entity_search_music = (es['data']['base_info']['type'] == 'Music')
                entity_search_org = (es['data']['base_info']['type'] == 'Org')
                entity_search_picture = (es['data']['base_info']['type'] == 'Picture')
                entity_search_site = (es['data']['base_info']['type'] == 'Site')
                entity_search_soft = (es['data']['base_info']['type'] == 'Soft')
                entity_search_text = (es['data']['base_info']['type'] == 'Text')

        features = []

        features.extend([music_sites_count, video_sites_count,
                         music_in_top_1, video_in_top_1,
                         music_in_top_3, video_in_top_3,
                         music_in_top_5, video_in_top_5])

        site_features = [first_music_site, first_video_site,
                         video_kinopoisk_data, video_ivi_data, video_hdlava_data,
                         video_onlinemultfilmy_data, video_filmshd_data, video_rutube_data,
                         music_zvooq_data, music_zaycev_data, music_drivemusic_data,
                         music_megapesni_data, music_muzmo_data, music_yandex_data, music_muzofond_data,
                         youtube_data, wikipedia_data]

        features.extend(list(itertools.chain.from_iterable(map(cls._convert_doc_factors, site_features))))

        features.extend([music_wizard_present, music_wizard_show, music_wizard_relevance, music_wizard_pos,
                         video_wizard_present, video_wizard_show, video_wizard_yandex_video, video_wizard_pos])

        features.extend([entity_search_present, entity_search_found, entity_search_anim,
                         entity_search_auto, entity_search_band, entity_search_chemical_compound,
                         entity_search_device,
                         entity_search_drugs, entity_search_event, entity_search_film, entity_search_food,
                         entity_search_geo, entity_search_hum, entity_search_hum1, entity_search_music,
                         entity_search_org, entity_search_picture, entity_search_site, entity_search_soft,
                         entity_search_text])

        return np.array(features)


class MusicSetupFeatures(BaseSetupFeatures):
    NAME = 'music_play_setup'

    @staticmethod
    def name():
        return MusicSetupFeatures.NAME

    @staticmethod
    def from_dict(data):
        entity_search_found = 0
        music_wizard = 0
        music_wizard_pos = -1
        music_wizard_relevance = -1
        music_wizard_relev_prediction = -1.0

        try:
            entity_search_found = int(
                get_by_path(data, 'music_web_search/snippets/entity_search/found') is True
            )
            music_wizard = int(
                get_by_path(data, 'music_web_search/wizards/musicplayer/show') is True
            )
            music_wizard_pos = int(
                get_by_path(data, 'music_web_search/wizards/musicplayer/document/markers/WizardPos', -1)
            )
            music_wizard_relevance = int(
                get_by_path(data, 'music_web_search/wizards/musicplayer/document/relevance', -1)
            )
            music_wizard_relev_prediction = float(
                get_by_path(data, 'music_web_search/wizards/musicplayer/document/markers/RelevPrediction', -1)
            )
        except Exception as e:
            logger.error("Failed to process MusicSetupFeatures: %s", e, exc_info=True)

        return np.array([
            entity_search_found,
            music_wizard,
            music_wizard_pos,
            music_wizard_relevance,
            music_wizard_relev_prediction
        ])


register_setup_features(VideoSetupFeatures, 'personal_assistant.scenarios.video_play')
register_setup_features(MusicSetupFeatures, 'personal_assistant.scenarios.music_play')
