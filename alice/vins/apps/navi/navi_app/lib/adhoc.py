# coding: utf-8
from __future__ import unicode_literals

import string

from vins_core.common.sample import Sample

from vins_core.dm.form_filler.models import Form

from navi_app.lib import callbacks
from navi_app.lib.navirequest import normalize_lang

INTENT_KEYWORDS = {
    'tr': {
        'route': ['nasıl', 'gidilir', 'gitmek', 'istiyorum', 'yol', 'tarifi', 'yol', 'güzergahı', 'gideceğim',
                  'git', 'gidecem', 'yolu', 'yol', 'durumu', 'gitmek', 'gidelim', 'yol', 'yolu', 'haritası', 'gidelim',
                  'güzergah', 'oluştur', 'sür', 'rota'],
        'cancel_route': ['iptal', 'vazgeç', 'vazgeçtim', 'durdur', 'ertele', 'bekle', 'gitmeyelim', 'gitme',
                         'istemiyorum', 'istemem', 'yok', 'olmaz', 'no', 'hayır', 'götürme'],
        'confirm_route': ['gidelim', 'haydi', 'hadi', 'başla', 'okey', 'git', 'bakalım', 'evet', 'olur',
                          'peki', 'tamam', 'oldu', 'gidek', 'götür', 'götürsene', 'hazırım', 'çıkalım', 'başlayalım', 'gidebiliriz'],
        'add_camera': ['radar', 'radarları', 'kamera', 'kamerası', 'kontrol'],
        'add_roadworks': ['çalışması', 'çalışma'],
        'show_traffic': ['trafik', 'trafiği'],
        'add_accident': ['kaza', 'kazası', 'kazaları'],
    },
    'en': {
        'route': ['route', 'go', 'to'],
        'cancel_route': ['cancel', 'no'],
        'confirm_route': ['go', 'yes'],
        'add_camera': ['speed', 'camera', 'cameras'],
        'add_roadworks': ['roadwork', 'roadworks'],
        'show_traffic': ['traffic'],
        'add_accident': ['accident', 'accidents'],
    },
    'fr': {
        'route': ['y', 'go'],
        'cancel_route': ['annuler', 'non'],
        'confirm_route': ['allons', 'allons-y'],
        'add_camera': ['caméra', 'caméras', 'camera', 'cameras'],
        'add_roadworks': ['travaux'],
        'show_traffic': ['trafic', 'traffic'],
        'add_accident': ['accident', 'accidents'],
    },
    'he': {
        'route': [],
        'cancel_route': [],
        'confirm_route': [],
        'add_camera': [],
        'add_roadworks': [],
        'show_traffic': [],
        'add_accident': [],
    }
}

PUNCTUATION = set(string.punctuation)


def _build_route_form():
    return Form.from_dict({
        'name': 'route',
        'slots': [
            {
                'slot': 'location_to',
                'types': ['string'],
                'optional': True,
            },
            {
                'slot': 'location_from',
                'types': ['string'],
                'optional': True
            },
            {
                'slot': 'location_via',
                'types': ['string'],
                'optional': True
            },
            {
                'slot': 'route_action',
                'types': ['route_marker'],
                'optional': True
            },
            {
                'slot': 'nearest',
                'types': ['nearest_marker'],
                'optional': True
            }
        ]
    })


def _build_show_layer_form():
    return Form.from_dict({
        'name': 'show_layer',
        'slots': [
            {
                'slot': 'cameras',
                'types': ['string'],
                'optional': True,
            },
            {
                'slot': 'traffic',
                'types': ['string'],
                'optional': True
            }
        ]
    })


def _build_add_point_form():
    return Form.from_dict({
        'name': 'add_point',
        'slots': [
            {
                "slot": "accident",
                "types": ["string"],
                "optional": True
            },
            {
                "slot": "camera",
                "types": ["string"],
                "optional": True
            },
            {
                "slot": "roadworks",
                "types": ["string"],
                "optional": True
            },
            {
                "slot": "other",
                "types": ["string"],
                "optional": True
            },
        ]
    })


def _build_search_form():
    return Form.from_dict({
        'name': 'search',
        'slots': [
            {
                'slot': 'query',
                'types': ['string'],
                'optional': True,
            }
        ]
    })


def _last_word_in_keywords(keywords, text):
    words = text.split(' ')
    return len(words) > 0 and words[-1].lower() in keywords


def _keywords_in_text(keywords, text):
    words = text.split(' ')
    return len(words) > 0 and any([word.lower() in keywords for word in words])


def _strip_stopwords(stopwords, text):
    toks = text.split(' ')
    while toks[0].lower() in stopwords and len(toks) > 1:
        toks = toks[1:]
    while toks[-1].lower() in stopwords and len(toks) > 1:
        toks = toks[:-1]
    return ' '.join(toks)


def _waiting_for_route_confirmation(req_info):
    device_state = req_info.device_state or {}
    states = device_state.get('navigator', {}).get('states', [])
    return 'waiting_for_route_confirmation' in states


def _normalize_utterance_text(utterance_text):
    tokens = utterance_text.split()
    if tokens:
        return ' '.join(t for t in tokens if t not in PUNCTUATION)
    return utterance_text


def adhoc_nlu_handle(req_info, **kwargs):
    lang = req_info.lang
    keywords = INTENT_KEYWORDS[normalize_lang(lang)]
    kwargs['sample'] = Sample.from_utterance(req_info.utterance)
    utterance_text = _normalize_utterance_text(req_info.utterance.text)

    if _waiting_for_route_confirmation(req_info):
        if _keywords_in_text(keywords['cancel_route'], utterance_text):
            return callbacks.cancel(None, req_info, **kwargs)
        elif _keywords_in_text(keywords['confirm_route'], utterance_text):
            return callbacks.confirmation(None, req_info, **kwargs)

    if _keywords_in_text(keywords['add_accident'], utterance_text):
        form = _build_add_point_form()
        form.get_slot_by_name('accident').set_value('accident', 'string')
        return callbacks.add_point(form, req_info, sub_intent='accident', **kwargs)
    elif _keywords_in_text(keywords['add_camera'], utterance_text):
        form = _build_add_point_form()
        form.get_slot_by_name('camera').set_value('camera', 'string')
        return callbacks.add_point(form, req_info, sub_intent='camera', **kwargs)
    elif _keywords_in_text(keywords['add_roadworks'], utterance_text):
        form = _build_add_point_form()
        form.get_slot_by_name('roadworks').set_value('roadworks', 'string')
        return callbacks.add_point(form, req_info, sub_intent='roadworks', **kwargs)
    elif _keywords_in_text(keywords['show_traffic'], utterance_text):
        form = _build_show_layer_form()
        form.get_slot_by_name('traffic').set_value('traffic', 'string')
        return callbacks.switch_layers(form, req_info, action='show', **kwargs)
    elif _keywords_in_text(keywords['route'], utterance_text):
        form = _build_route_form()
        form.get_slot_by_name('location_to').set_value(
            _strip_stopwords(keywords['route'], utterance_text), 'string'
        )
        form.get_slot_by_name('route_action').set_value('route_action', 'string')
        return callbacks.route(form, req_info, **kwargs)
    else:
        form = _build_search_form()
        form.get_slot_by_name('query').set_value(_strip_stopwords(keywords['route'], utterance_text), 'string')
        return callbacks.map_search(form, req_info, **kwargs)
