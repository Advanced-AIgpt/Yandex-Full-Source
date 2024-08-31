# coding: utf-8
from __future__ import unicode_literals

from uuid import uuid4

import os
import mock
import pytest
import yatest.common

from vins_core.dm.request import ReqInfo
from vins_core.common.utterance import Utterance
from vins_core.utils.data import find_vinsfile
from vins_core.utils.datetime import utcnow
from vins_core.utils.strings import smart_unicode

from vins_sdk.app import VinsApp

from navi_app.app import NaviApp, base_handler_fail_mock
from navi_app.lib.geosearch import geosearch_geo_mock, geosearch_geo_mock_many
from navi_app.lib.mockdata import choose_location, generate_options


@pytest.fixture(scope='session', autouse=True)
def init_env():
    os.environ['VINS_RESOURCES_PATH'] = yatest.common.binary_path('alice/vins/resources')


@pytest.fixture(scope='module')
def vins_app():
    from vins_sdk.connectors import TestConnector
    return TestConnector(vins_app=NaviApp(vins_file=find_vinsfile('navi_app')))


def process_pair(app, text, response_text, lang='ru', state={}):
    location = choose_location(lang)
    req_info_kwargs = app.get_default_reqinfo_kwargs()
    req_info = ReqInfo(
        uuid=uuid4(),
        utterance=Utterance(smart_unicode(text)),
        client_time=utcnow(),
        lang=lang,
        location=location,
        additional_options=generate_options(lang, location),
        app_info=req_info_kwargs['app_info'],
        device_state=state
    )
    result = app.handle_reqinfo(req_info)
    result_text = result['cards'][0]['text'] if len(result['cards']) > 0 else ''
    assert result_text == response_text


@pytest.mark.parametrize('utterance, result', [
    ('kfc', "Поиск на карте 'kfc'"),
    ('челябинск', "Поиск на карте 'челябинск'"),
    ('найди kfc', "Поиск на карте 'kfc'"),
    ('окей гугл kfc', "Поиск на карте 'kfc'")
])
def test_search(vins_app, utterance, result):
    with geosearch_geo_mock_many(
        [{'name': 'kfc'}] * 5
    ):
        process_pair(vins_app, utterance, result)


@pytest.mark.parametrize('mock_kwargs, utterance, result', [
    (
        {
            'name': 'ФГБНУ Научный центр сердечно-сосудистой хирургии им. А.Н. Бакулева',
            'lat': 55.725420,
            'lon': 37.606717,
        },
        'институт сердца поехали',
        "Едем до 'ФГБНУ Научный центр сердечно-сосудистой хирургии им. А.Н. Бакулева'"
    ),
    (
        {
            'name': 'Сбербанк России, банкомат',
            'lat': 55.735707,
            'lon': 37.592303
        },
        'заехать в сбербанк банкомат',
        "Едем через 'Сбербанк России, банкомат'"
    ),
    (
        {
            'name': 'Челябинск',
            'description': 'Россия, Челябинск',
            'lat': 61.402590,
            'lon': 55.160026,
        },
        'построить маршрут до челябинска',
        "Едем до 'Челябинск (Россия, Челябинск)'"
    ),
    (
        {
            'name': 'Львов',
            'description': 'Украина, Львов',
            'lat': 24.029709,
            'lon': 49.839678,
        },
        'львов поехали',
        "Едем до 'Львов (Украина, Львов)'"
    ),
])
def test_route(vins_app, mock_kwargs, utterance, result):
    with geosearch_geo_mock(**mock_kwargs):
        process_pair(vins_app, utterance, result)


def test_route_reset(vins_app):
    process_pair(vins_app, 'сбросить маршрут', 'Маршрут сброшен')


def test_confirmation(vins_app):
    process_pair(vins_app, 'ок', 'Подтверждение принято')
    process_pair(vins_app, 'да', 'Подтверждение принято')


def test_cancel(vins_app):
    process_pair(vins_app, 'нет', 'Отменяю действие')
    process_pair(vins_app, 'отменить', 'Отменяю действие')


def test_show_layer(vins_app):
    process_pair(vins_app, 'покажи пробки', "Показать 'пробки'")
    process_pair(vins_app, 'включи камеры', "Показать 'камеры'")


def test_hide_layer(vins_app):
    process_pair(vins_app, 'скрой пробки', "Скрыть 'пробки'")
    process_pair(vins_app, 'убери камеры', "Скрыть 'камеры'")


def test_route_view(vins_app):
    process_pair(vins_app, 'покажи маршрут', "Обзор маршрута")


def test_add_point(vins_app):
    process_pair(vins_app, 'Вижу аварию в левом ряду', "Поставить точку: 'ДТП', 'левый ряд'")
    process_pair(vins_app, 'здесь дорожные работы', "Поставить точку: 'Дорожные работы'")


def test_add_camera(vins_app):
    process_pair(vins_app, 'здесь камера', "Поставить точку: 'Камера'")
    process_pair(vins_app, 'камера на 60', "Поставить точку: 'Камера', с комментарием: 'на 60'")


def test_ignore(vins_app):
    process_pair(vins_app, ' ', "Извините, непонятно")
    process_pair(vins_app, '<censored> ', "Извините, непонятно")


def test_add_talk(vins_app):
    process_pair(
        vins_app,
        'разговорчик могу на троссе дотащить',
        "Поставить точку: 'Разговорчики', с комментарием: 'могу на троссе дотащить'"
    )
    process_pair(
        vins_app,
        'разговорчики что блин опять поворот не тот показывает',
        "Поставить точку: 'Разговорчики', с комментарием: 'блин опять поворот не тот показывает'"
    )

    process_pair(vins_app, 'блин ну опять застряли на 3 часа', "Поиск на карте 'блин ну опять застряли на 3 часа'")


@pytest.mark.parametrize('mock_kwargs, utterance, lang, result', [
    (
        {
            'name': 'Yandex LLC',
            'lat': 55.735707,
            'lon': 37.592303,
        },
        'yandex gidilir',
        'tr',
        "'Yandex LLC' noktasına rota hesaplanıyor",
    ),
    (
        {
            'name': 'Yandex LLC',
            'description': 'Russia, Moscow',
            'lat': 55.735707,
            'lon': 37.592303,
        },
        'route to yandex',
        'en',
        "Go to 'Yandex LLC (Russia, Moscow)'",
    ),
    (
        {
            'name': 'Yandex LLC',
            'description': 'Russia, Moscow',
            'lat': 55.735707,
            'lon': 37.592303,
        },
        'yandex go',
        'fr',
        "On y va à 'Yandex LLC (Russia, Moscow)'",
    )
])
def test_route_foreign(vins_app, mock_kwargs, utterance, lang, result):
    with geosearch_geo_mock(**mock_kwargs):
        process_pair(vins_app, utterance, result, lang)


@pytest.mark.parametrize('query, lang, result', [
    ('trafiği', 'tr', "Göster 'Trafik durumunu'"),
    ('traffic', 'en', "Show 'traffic jams'"),
    ('trafic', 'fr', "Montrer 'trafic'"),
    ('kazaları', 'tr', "Yol uyarısı ekle: 'Trafik kazası'"),
    ('accident', 'en', "Add point: 'Accident'"),
    ('accident', 'fr', "Placer un point: 'Accident'"),
    ('radarları', 'tr', "Yol uyarısı ekle: 'Hız kamerası'"),
    ('speed camera', 'en', "Add point: 'Speed camera'"),
    ('camera', 'fr', "Placer un point: 'caméra'"),
    ('roadworks', 'en', "Add point: 'Road works'"),
    ('yol çalışması bildir', 'tr', "Yol uyarısı ekle: 'Yol çalışması'"),
    ('travaux routiers', 'fr', "Placer un point: 'Travaux sur la chaussée'"),
])
def test_queries_foreign(vins_app, query, lang, result):
    process_pair(vins_app, query, result, lang)


@pytest.mark.parametrize('query, lang, result', [
    ('gidelim', 'tr', "Onay kabul"),
    ('Git', 'tr', "Onay kabul"),
    ('Durdur', 'tr', "iptal"),
    ('allons', 'fr', "confirmer"),
    ('Annuler', 'fr', "J'annule l'action"),
    ('go', 'en', "confirmed"),
    ('cancel', 'en', "canceled")
])
def test_confirmation_foreign(vins_app, query, lang, result):
    device_state = {'navigator': {'states': ["waiting_for_route_confirmation"]}}
    process_pair(vins_app, query, result, lang, device_state)


def test_handler_exception(vins_app):
    with mock.patch.object(VinsApp, 'handle_request', base_handler_fail_mock):
        process_pair(vins_app, 'поехали в парк', 'Извините, непонятно', 'ru')
