# coding: utf-8
from __future__ import unicode_literals

import logging
import re
import marisa_trie

from vins_core.utils.lemmer import Lemmer, Inflector
from vins_core.utils.config import get_setting
from vins_core.utils.data import get_resource_full_path

from navi_app.lib.geosearch import GeosearchAPI, gps_distance
from navi_app.lib.navirequest import normalize_lang
from navi_app.lib.localization import FAV_SYNONYMS

from library.python import func

logger = logging.getLogger(__name__)

RE_PUNCT = re.compile(r'[`~!@#$%^&*()\-_=\+\[\]\{\}\\|;:\'",\<\.\>\?«»]', re.U)
RE_NUMBER = re.compile(r'[0-9]+', re.U)

CUT_OFF_DISTANCE_MULTIPLIER = 2

GEOSEARCH_TIMEOUT = float(get_setting('GEOSEARCH_TIMEOUT', yenv={
    'production': '0.3',
    'testing': '0.3',
    'development': '1'
}))

# https://a.yandex-team.ru/arc/trunk/arcadia/extsearch/geo/README.md
GEOSEARCH_URL = get_setting('GEOSEARCH_URL', yenv={
    'production': 'http://addrs.yandex.ru:17140/yandsearch',
    'testing': 'http://addrs-testing.search.yandex.net/search/stable/yandsearch',
    'development': 'http://addrs-testing.search.yandex.net/search/stable/yandsearch',
})
GEOSEARCH_TVM2_NAME = get_setting('TVM2_NAME', default='navi_tvm')

_geosearch_api = GeosearchAPI(
    url=GEOSEARCH_URL,
    timeout=GEOSEARCH_TIMEOUT,
    tvm2_client_name=GEOSEARCH_TVM2_NAME
)

_morphology = {
    'ru': Lemmer(['ru', 'en']),
    'en': Lemmer(['en']),
    'uk': Lemmer(['uk', 'ru', 'en']),
    'tr': Lemmer(['tr', 'en']),
    'fr': Lemmer(['fr', 'en']),
}
_inflector = {
    'ru': Inflector('ru'),
    'en': Inflector('en'),
    'uk': Inflector('uk'),
    'tr': Inflector('tr'),
    'fr': Inflector('fr'),
}


@func.memoize()
def _get_geonorm_stoplist():
    geonorm_stoplist = marisa_trie.RecordTrie('<f')
    geonorm_stoplist.load(get_resource_full_path('resource://marisa_trie/geonorm_stoplist.marisa'))
    return geonorm_stoplist


def _is_address(name):
    return bool(RE_NUMBER.search(name))


def _normalize_morph_syn(name, synonyms, lang):
    name = re.sub(RE_PUNCT, ' ', name)
    name = ' '.join(filter(lambda x: x not in [u'в', u'к', u'на', u'до'], name.lower().split()))
    words = name.split()

    for i in range(0, len(words)):
        words[i] = _morphology[lang].parse(words[i])[0].normal_form
        for syn_group in synonyms[lang]:
            if words[i] in synonyms[lang][syn_group]:
                words[i] = syn_group
    result = ' '.join(words)

    for syn_group in synonyms[lang]:
        if result in synonyms[lang][syn_group]:
            result = syn_group

    return result


def _weak_compare(x, y, synonyms, lang):
    return _normalize_morph_syn(x, synonyms, lang) == _normalize_morph_syn(y, synonyms, lang)


# TODO: remove according to [DIALOG-947]
def _translate_title(title, lang):
    if 'ru' in lang.lower():
        if title.lower() == 'home':
            return 'Дом'
        if title.lower() == 'work':
            return 'Работа'
    return title


def _find_favourites(value, req_info):
    result = []
    normalized_lang = normalize_lang(req_info.lang)
    for f in req_info.additional_options.get('favourites', []):
        if (_weak_compare(f['title'], value, FAV_SYNONYMS, normalized_lang) or
                ('subtitle' in f and _weak_compare(f['subtitle'], value, FAV_SYNONYMS, normalized_lang))):
            result.append({
                'name': _translate_title(f['title'], req_info.lang),
                'description': f.get('subtitle', ''),
                'lat': f['lat'],
                'lon': f['lon'],
                'is_favourite': True,
                'is_address': True,
            })
    return result


def heuristic_rescore(targets, req_info):
    for target in targets:
        target['distance'] = gps_distance(
            req_info.location['lat'],
            req_info.location['lon'],
            target['lat'],
            target['lon']
        )

    # do nothing if there are no targets
    if len(targets) == 0:
        return targets

    # do nothing if we have relevant geo region at the first place
    if 'kind' in targets[0] and targets[0]['kind'] in ['country', 'city', 'district', 'province', 'locality']:
        return targets
    return sorted(targets, key=lambda x: x['distance'])


def _strip_farther(targets, req_info):
    for n, target in enumerate(heuristic_rescore(targets, req_info)):
        if target['distance'] > CUT_OFF_DISTANCE_MULTIPLIER * targets[0]['distance']:
            return targets[:n]
    return targets


def find_geo_targets(address, req_info, results=15):
    """
    Uses geosearch api, finds geo and biz objects, but priority is for geo objects.
    If any geo objects exists, then drops biz part to build route to the geo immediately.
    """
    address = normalize_name(address, normalize_lang(req_info.lang))
    favourites_targets = _find_favourites(address, req_info)
    # return favourites if found
    if favourites_targets:
        return favourites_targets

    # use geosearch if no favourites found
    if 'max_lat' in req_info.additional_options:
        spn = '%.5f,%.5f' % (
            (req_info.additional_options['max_lat'] - req_info.additional_options['min_lat']),
            (req_info.additional_options['max_lon'] - req_info.additional_options['min_lon'])
        )
    else:
        spn = '0.004,0.004'

    geo_biz_targets = _geosearch_api.find(
        text=address,
        lat=req_info.location['lat'],
        lon=req_info.location['lon'],
        lang=normalize_lang(req_info.lang),
        spn=spn,
        results=results
    )

    only_geo_targets = []
    for target in geo_biz_targets:
        target['is_favourite'] = False
        target['is_address'] = _is_address(address)
        if target['type'] == 'geo':
            only_geo_targets.append(target)

    if only_geo_targets:
        if _is_address(address):
            return _strip_farther(only_geo_targets, req_info)
        else:
            return only_geo_targets
    else:
        return geo_biz_targets


def normalize_name(name, lang):
    #  https://st.yandex-team.ru/DIALOG-532#1498765365000
    if (not _is_address(name)) and len(name.split(' ')) < 3 and name not in _get_geonorm_stoplist():
        inflected = _inflector[lang].inflect(name, {'nomn'})
        return inflected
    else:
        return name
