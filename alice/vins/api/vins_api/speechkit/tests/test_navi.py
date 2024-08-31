# coding: utf-8
from __future__ import unicode_literals

from uuid import uuid4

import mongomock
import pytest
import json
import mock
import falcon.testing

from vins_core.utils.strings import smart_utf8

from vins_api.speechkit.api import make_app
from vins_api.speechkit import settings

from navi_app.lib.geosearch import geosearch_geo_mock, geosearch_biz_mock_many
from navi_app.lib.mockdata import FAVOURITES_MOCK


@pytest.fixture
def navi_client(mocker):
    settings.CONNECTED_APPS.update({
        'navi': {
            'path': 'navi_app/config/Vinsfile.json',
            'class': 'navi_app.app.NaviApp'
        }
    })
    mocker.patch('vins_api.common.resources.get_db_connection', return_value=mongomock.MongoClient().test_db)
    return falcon.testing.TestClient(make_app()[0])


def _voice_input(utterance, words=None, confidence=1.0):
    res = {
        'type': 'voice_input',
        'asr_result': [
            {
                'confidence': confidence,
                'utterance': utterance,
            }
        ]
    }
    if words is not None:
        res['asr_result'][0]['words'] = [{'confidence': confidence, 'value': word} for word in words]
    return res


def _generate_device_state_4_navi(
    home=None, work=None, favourites=FAVOURITES_MOCK, points=None
):
    work = work or {
        'lat': 15.34,
        'lon': 36.78,
        'arrival_points': []
    }

    home = home or {
        'lat': 12.34,
        'lon': 56.78,
        'arrival_points': []
    }

    if points is None:
        current_route = {}
    else:
        current_route = {
            'points': points,
            'distance_to_destination': 7093.17,
            'raw_time_to_destination': 723,
            'arrival_timestamp': 1486736347,
            'time_to_destination': 1200,
            'time_in_traffic_jam': 600,
            'distance_in_traffic_jam': 2000
        }

    device_state = {
        'navigator': {
            'map_view': {
                'tl_lat': 55.7307,
                'tl_lon': 37.6351,
                'tr_lat': 55.7158,
                'tr_lon': 37.6058,
                'br_lat': 55.7434,
                'br_lon': 37.5701,
                'bl_lat': 55.7536,
                'bl_lon': 37.5903
            },
            'home': home,
            'work': work,
            'user_favorites': [
                {'name': f[0], 'lat': f[2], 'lon': f[1]} for f in favourites
            ],
            'current_route': current_route,
        }
    }
    return device_state


def _request(
    client, request, uuid=None, device_id=None, header=None, platform='android',
    app_id='ru.yandex.mobile.navigator', lang='ru-RU', content_type=b'application/json'
):
    default_header = {'request_id': '123'}
    default_header.update(header or {})

    return client.simulate_post(
        path='/speechkit/app/navi',
        headers={'content-type': content_type},
        body=smart_utf8(json.dumps({
            'header': default_header,
            'application': {
                'uuid': str(uuid or uuid4()),
                'device_id': str(device_id or uuid4()),
                'client_time': '20161213T151558',
                'app_id': app_id,
                'app_version': '1.2.3',
                'os_version': '5.0',
                'platform': platform,
                'lang': lang,
                'timezone': 'Europe/Moscow',
                'timestamp': '1481631358',
            },
            'request': request,
        })))


def get_response_header(request_id='123', dialog_id=None, prev_req_id=None, sequence_number=None, **kwargs):
    mock_resp_id = mock.MagicMock()
    mock_resp_id.__eq__.return_value = True
    return {
        'request_id': request_id,
        'dialog_id': dialog_id,
        'prev_req_id': prev_req_id,
        'sequence_number': sequence_number,
        'response_id': mock_resp_id,
    }


def get_response_body(card_text, sub_name, uri):
    return {
        'card': {'text': card_text, 'type': 'simple_text', 'tag': None},
        'cards': [{'text': card_text, 'type': 'simple_text', 'tag': None}],
        'directives': [{
            'type': 'client_action',
            'name': 'open_uri',
            'sub_name': sub_name,
            'payload': {
                'uri': 'yandexnavi://' + uri
            }
        }],
        'suggest': None,
        'meta': [],
        'features': {},
        'experiments': {}
    }


def get_sk_response(text, sub_name, uri):
    return {
        'header': get_response_header('123'),
        'response': get_response_body(
            card_text=text,
            sub_name=sub_name,
            uri=uri
        ),
        'voice_response': {
            'output_speech': None,
            'should_listen': False,
            'directives': [],
        }
    }


def test_search(navi_client):
    mock_data = {
        'results': [
            {'name': 'Лукоил', 'street': 'Строителей', 'house': 19},
            {'name': 'BP', 'street': 'Воителей', 'house': 20},
            {'name': 'Газпром', 'street': 'Врачей', 'house': 21},
        ]
    }
    with geosearch_biz_mock_many(**mock_data):
        response = _request(
            navi_client,
            {
                'event': _voice_input('найди заправку'),
                'location': {
                    'lat': 55.732616,
                    'lon': 37.585889,
                    'accuracy': 26.63999938964844,
                    'recency': 388950,
                    'speed': 23.5
                },
                'device_state': _generate_device_state_4_navi(),
            },
        )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Поиск на карте 'заправку'",
        sub_name='navi_map_search',
        uri='map_search?text=%D0%B7%D0%B0%D0%BF%D1%80%D0%B0%D0%B2%D0%BA%D1%83'
    )


def test_route(navi_client):
    mock_data = {
        'street': 'проспект Ленина',
        'house': '43',
        'name': 'проспект Ленина, 43',
        'lat': 55.797771,
        'lon': 37.945368,
        'description': 'Балашиха, Московская область, Россия'
    }
    with geosearch_geo_mock(**mock_data):
        response = _request(
            navi_client,
            {
                'event': _voice_input('маршрут до ленина 43'),
                'location': {
                    'lat': 55.732616,
                    'lon': 37.585889,
                    'accuracy': 26.63999938964844,
                    'recency': 388950,
                    'speed': 23.5
                },
                'device_state': _generate_device_state_4_navi(),
            },
        )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Едем до 'проспект Ленина, 43'",
        sub_name='navi_build_route_on_map',
        uri='build_route_on_map?lat_to=55.797771&lon_to=37.945368'
    )


def test_route_with_confirmation_support(navi_client):
    mock_data = {
        'street': 'проспект Ленина',
        'house': '43',
        'name': 'проспект Ленина, 43',
        'lat': 55.797771,
        'lon': 37.945368,
        'description': 'Балашиха, Московская область, Россия'
    }

    with geosearch_geo_mock(**mock_data):
        response = _request(
            navi_client,
            {
                'event': _voice_input('маршрут до ленина 43'),
                'location': {
                    'lat': 55.732616,
                    'lon': 37.585889,
                    'accuracy': 26.63999938964844,
                    'recency': 388950,
                    'speed': 23.5
                },
                'additional_options': {
                    'supported_features': ['confirmation']
                },
                'device_state': _generate_device_state_4_navi(),
            },
        )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Едем до 'проспект Ленина, 43'",
        sub_name='navi_build_route_on_map',
        uri='build_route_on_map?confirmation=1&lat_to=55.797771&lon_to=37.945368'
    )


def test_route2(navi_client):
    mock_data = {
        'street': 'Москва, Рублевское ш.',
        'house': '135',
        'name': 'ФГБНУ Научный центр сердечно-сосудистой хирургии им. А.Н. Бакулева',
        'lat': 55.761112,
        'lon': 37.377472,
        'description': 'Москва, Рублевское ш., 135'
    }

    with geosearch_geo_mock(**mock_data):
        response = _request(
            navi_client,
            {
                'event': _voice_input('институт сердца поехали'),
                'location': {
                    'lat': 55.732616,
                    'lon': 37.585889,
                    'accuracy': 26.63999938964844,
                    'recency': 388950,
                    'speed': 23.5
                },
                'device_state': _generate_device_state_4_navi(),
            },
        )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Едем до 'ФГБНУ Научный центр сердечно-сосудистой хирургии им. А.Н. Бакулева'",
        sub_name='navi_build_route_on_map',
        uri='build_route_on_map?lat_to=55.761112&lon_to=37.377472'
    )


def test_favourites(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('поехали домой'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Едем до 'дом'",
        sub_name='navi_build_route_on_map',
        uri='build_route_on_map?lat_to=12.34&lon_to=56.78'
    )


def test_confirmation(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('да'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text='Подтверждение принято',
        sub_name='navi_external_confirmation',
        uri='external_confirmation?confirmed=1'
    )


def test_cancel(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('отмена'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text='Отменяю действие',
        sub_name='navi_external_confirmation',
        uri='external_confirmation?confirmed=0'
    )


def test_add_point(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('авария в правом ряду'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Поставить точку: 'ДТП', 'правый ряд'",
        sub_name='navi_add_point',
        uri='add_point?category=0&comment=&where=%D0%BF%D1%80%D0%B0%D0%B2%D1%8B%D0%B9%20%D1%80%D1%8F%D0%B4'
    )


def test_add_point_with_confirmation_support(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('авария в правом ряду'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'additional_options': {
                'supported_features': ['confirmation']
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    uri = 'add_point?category=0&comment=&confirmation=1&where=%D0%BF%D1%80%D0%B0%D0%B2%D1%8B%D0%B9%20%D1%80%D1%8F%D0%B4'
    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Поставить точку: 'ДТП', 'правый ряд'",
        sub_name='navi_add_point',
        uri=uri
    )


def test_add_point1(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('поставить точку ремонт в правом ряду'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Поставить точку: 'Дорожные работы', 'правый ряд'",
        sub_name='navi_add_point',
        uri='add_point?category=1&comment=&where=%D0%BF%D1%80%D0%B0%D0%B2%D1%8B%D0%B9%20%D1%80%D1%8F%D0%B4'
    )


def test_bug_report(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('отсутствует разворот'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Поставить точку: 'Ошибка', с комментарием: 'Нет поворота'",
        sub_name='navi_add_point',
        uri='add_point?category=7&comment=%D0%9D%D0%B5%D1%82%20%D0%BF%D0%BE%D0%B2%D0%BE%D1%80%D0%BE%D1%82%D0%B0'
    )


def test_show_layer(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('покажи пробки'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Показать 'пробки'",
        sub_name='navi_traffic',
        uri='traffic?traffic_on=1'
    )


def test_hide_layer(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('скрой пробки'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Скрыть 'пробки'",
        sub_name='navi_traffic',
        uri='traffic?traffic_on=0'
    )


def test_add_talk(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('кинь разговорчик привет всем'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Поставить точку: 'Разговорчики', с комментарием: 'привет всем'",
        sub_name='navi_add_point',
        uri='add_point?category=6&comment=%D0%BF%D1%80%D0%B8%D0%B2%D0%B5%D1%82%20%D0%B2%D1%81%D0%B5%D0%BC'
    )


def test_route_reset(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('сбрось маршрут'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text='Маршрут сброшен',
        sub_name='navi_clear_route',
        uri='clear_route'
    )


def test_bad_favourites(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('проверим некорректный favourites'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(
                favourites=[('Дом Бабушки', '55.3', 'unknow', 'адрес дома бабушки')]),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': {
                'text': 'Извините, непонятно',
                'type': 'simple_text',
                'tag': None
            },
            'cards': [{
                'text': 'Извините, непонятно',
                'type': 'simple_text',
                'tag': None
            }],
            'directives': [{
                'type': 'client_action',
                'name': 'open_uri',
                'sub_name': 'navi_map_search',
                'payload': {
                    'uri': 'yandexnavi://map_search?text=%D0%BF%D1%80%D0%BE%D0%B2%D0%B5%D1%80%D0%B8%D0%BC%20%D0%BD%D0%B5%D0%BA%D0%BE%D1%80%D1%80%D0%B5%D0%BA%D1%82%D0%BD%D1%8B%D0%B9%20favourites',
                }
            }],
            'suggest': None,
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {
            'should_listen': False,
            'output_speech': None,
            'directives': []
        },
    }


def test_route_route(navi_client):
    mock_data = {
        'street': 'улица Льва Толстого',
        'house': '16',
        'name': 'улица Льва Толстого, 16',
        'lat': 55.733842,
        'lon': 37.588144,
        'description': 'Москва, Россия'
    }

    with geosearch_geo_mock(**mock_data):
        response = _request(
            navi_client,
            {
                'event': _voice_input('Льва Толстого 16'),
                'location': {
                    'lat': 55.732616,
                    'lon': 37.585889,
                    'accuracy': 26.63999938964844,
                    'recency': 388950,
                    'speed': 23.5
                },
                'device_state': _generate_device_state_4_navi(points=[]),
            },
        )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Едем до 'улица Льва Толстого, 16'",
        sub_name='navi_build_route_on_map',
        uri='build_route_on_map?lat_to=55.733842&lon_to=37.588144'
    )


def test_route_search(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('Льва Толстого 16'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Поиск на карте 'льва толстого 16'",
        sub_name='navi_map_search',
        uri='map_search?text=%D0%BB%D1%8C%D0%B2%D0%B0%20%D1%82%D0%BE%D0%BB%D1%81%D1%82%D0%BE%D0%B3%D0%BE%2016'
    )


def test_show_me(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('где я'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text='Поиск текущего местоположения',
        sub_name='navi_show_user_position',
        uri='show_user_position'
    )


def test_show_parking(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('покажи парковки'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Показать 'парковки'",
        sub_name='navi_show_ui_map',
        uri='show_ui/map?carparks_enabled=1'
    )


def test_find_parking(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('найди парковку'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text='Поиск парковок',
        sub_name='navi_carparks_route',
        uri='carparks_route'
    )


def test_route_view(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('обзор маршрута'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text='Обзор маршрута',
        sub_name='navi_show_route_overview',
        uri='show_route_overview'
    )


def test_empty(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input(''),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
    )

    assert response.status_code == 200
    assert json.loads(response.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': {
                'text': 'Извините, непонятно',
                'type': 'simple_text',
                'tag': None
            },
            'cards': [{
                'text': 'Извините, непонятно',
                'type': 'simple_text',
                'tag': None
            }],
            'directives': [{
                'type': 'client_action',
                'name': 'open_uri',
                'sub_name': 'navi_map_search',
                'payload': {
                    'uri': 'yandexnavi://map_search?text=',
                }
            }],
            'suggest': None,
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {
            'should_listen': False,
            'output_speech': None,
            'directives': []
        },
    }


def test_parse_error(navi_client):
    response = navi_client.simulate_post(
        path='/speechkit/app/navi',
        headers={'content-type': b'application/json'},
        body=''
    )
    assert response.status_code == 400
    assert 'X-Yandex-Vins-OK' not in response.headers


def test_wrong_content_type(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('Льва Толстого 16'),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        content_type=b'application/kuku'
    )
    assert response.status_code == 400
    assert 'X-Yandex-Vins-OK' not in response.headers


def test_bad_request(navi_client):
    response = navi_client.simulate_post(
        path='/speechkit/app/navi',
        headers={'content-type': b'application/json'},
        body='{"a": 1}'
    )

    assert response.status_code == 400
    assert 'X-Yandex-Vins-OK' not in response.headers


def test_server_action(navi_client):
    mock_data = {
        'street': 'проспект Ленина',
        'house': '43',
        'name': 'проспект Ленина, 43',
        'lat': 55.797771,
        'lon': 37.945368,
        'description': 'Балашиха, Московская область, Россия'
    }
    with geosearch_geo_mock(**mock_data):
        response = _request(
            navi_client,
            {
                'event': {
                    'name': 'on_reset_session',
                    'payload': {
                        'mode': 'clear_history'
                    },
                    'type': 'server_action'
                },
                'location': {
                    'lat': 55.732616,
                    'lon': 37.585889,
                    'accuracy': 26.63999938964844,
                    'recency': 388950,
                    'speed': 23.5
                },
                'device_state': _generate_device_state_4_navi(),
            },
        )

    assert response.status_code == 200
    assert json.loads(response.content) == {
        'header': get_response_header('123'),
        'response': {
            "cards": [],
            "suggest": None,
            "meta": [],
            "directives": [],
            "card": None,
            'features': {},
            'experiments': {}
        },
        'voice_response': {
            'output_speech': None,
            'should_listen': False,
            'directives': [],
        }
    }


def test_route_turkish(navi_client):
    mock_data = {
        'street': 'Lenin Bulvarı',  # проспект Ленина
        'house': '43',
        'name': 'Lenin Bulvarı, 43',  # проспект Ленина, 43
        'lat': 55.797771,
        'lon': 37.945368,
        'description': 'Balaşiha, Moskova Bölgesi, Rusya'  # Балашиха, Московская область, Россия
    }
    with geosearch_geo_mock(**mock_data):
        response = _request(
            navi_client,
            {
                'event': _voice_input('Lenin 43 rota'),  # маршрут до ленина 43
                'location': {
                    'lat': 55.732616,
                    'lon': 37.585889,
                    'accuracy': 26.63999938964844,
                    'recency': 388950,
                    'speed': 23.5
                },
                'device_state': _generate_device_state_4_navi(),
            },
            lang='tr-TR',
        )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="'Lenin Bulvarı, 43' noktasına rota hesaplanıyor",
        sub_name='navi_build_route_on_map',
        uri='build_route_on_map?lat_to=55.797771&lon_to=37.945368'
    )


def test_route_with_confirmation_support_turkish(navi_client):
    mock_data = {
        'street': 'Lenin Bulvarı',  # проспект Ленина
        'house': '43',
        'name': 'Lenin Bulvarı, 43',  # проспект Ленина, 43
        'lat': 55.797771,
        'lon': 37.945368,
        'description': 'Balaşiha, Moskova Bölgesi, Rusya'  # Балашиха, Московская область, Россия
    }

    with geosearch_geo_mock(**mock_data):
        response = _request(
            navi_client,
            {
                'event': _voice_input('Lenin 43 rota'),  # маршрут до ленина 43
                'location': {
                    'lat': 55.732616,
                    'lon': 37.585889,
                    'accuracy': 26.63999938964844,
                    'recency': 388950,
                    'speed': 23.5
                },
                'additional_options': {
                    'supported_features': ['confirmation']
                },
                'device_state': _generate_device_state_4_navi(),
            },
            lang='tr-TR',
        )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="'Lenin Bulvarı, 43' noktasına rota hesaplanıyor",
        sub_name='navi_build_route_on_map',
        uri='build_route_on_map?confirmation=1&lat_to=55.797771&lon_to=37.945368'
    )


def test_search_turkish(navi_client):
    mock_data = {
        'results': [
            {'name': 'Лукоил', 'street': 'Строителей', 'house': 19},
            {'name': 'BP', 'street': 'Воителей', 'house': 20},
            {'name': 'Газпром', 'street': 'Врачей', 'house': 21},
        ]
    }
    with geosearch_biz_mock_many(**mock_data):
        response = _request(
            navi_client,
            {
                'event': _voice_input('benzin istasyonu bulmak'),  # найди заправку
                'location': {
                    'lat': 55.732616,
                    'lon': 37.585889,
                    'accuracy': 26.63999938964844,
                    'recency': 388950,
                    'speed': 23.5
                },
                'device_state': _generate_device_state_4_navi(),
            },
            lang='tr-TR',
        )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Haritada bul 'benzin istasyonu bulmak'",
        sub_name='navi_map_search',
        uri='map_search?text=benzin%20istasyonu%20bulmak'
    )


def test_search_turkish_with_normalized_utterance(navi_client):
    mock_data = {
        'results': [
            {'name': 'Лукоил', 'street': 'Строителей', 'house': 19},
            {'name': 'BP', 'street': 'Воителей', 'house': 20},
            {'name': 'Газпром', 'street': 'Врачей', 'house': 21},
        ]
    }
    with geosearch_biz_mock_many(**mock_data):
        response = _request(
            navi_client,
            {
                'event': _voice_input('Uhuvvet Sokak Numara 12', ['Uhuvvet', 'Sokak', 'Numara', 'on', 'iki']),
                'location': {
                    'lat': 55.732616,
                    'lon': 37.585889,
                    'accuracy': 26.63999938964844,
                    'recency': 388950,
                    'speed': 23.5
                },
                'device_state': _generate_device_state_4_navi(),
            },
            lang='tr-TR',
        )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Haritada bul 'Uhuvvet Sokak Numara 12'",
        sub_name='navi_map_search',
        uri='map_search?text=Uhuvvet%20Sokak%20Numara%2012'
    )


def test_route2_turkish(navi_client):
    mock_data = {
        'street': 'Moskova, Rublevskoe sh.',  # Москва, Рублевское ш.
        'house': '135',
        # ФГБНУ Научный центр сердечно-сосудистой хирургии им. А.Н. Бакулева
        'name': 'FSBI Kalp ve Damar Cerrahisi Bilimsel Merkezi. BİR Bakuleva',
        'lat': 55.761112,
        'lon': 37.377472,
        'description': 'Moskova, Rublevskoe ş. 135'  # Москва, Рублевское ш., 135
    }

    with geosearch_geo_mock(**mock_data):
        response = _request(
            navi_client,
            {
                'event': _voice_input('kalp enstitüsü git'),  # институт сердца поехали
                'location': {
                    'lat': 55.732616,
                    'lon': 37.585889,
                    'accuracy': 26.63999938964844,
                    'recency': 388950,
                    'speed': 23.5
                },
                'device_state': _generate_device_state_4_navi(),
            },
            lang='tr-TR',
        )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="'FSBI Kalp ve Damar Cerrahisi Bilimsel Merkezi. BİR Bakuleva' noktasına rota hesaplanıyor",
        sub_name='navi_build_route_on_map',
        uri='build_route_on_map?lat_to=55.761112&lon_to=37.377472'
    )


def test_favourites_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('на дачу gidelim'),  # let's go 'на дачу'
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="'на дачу' noktasına rota hesaplanıyor",
        sub_name='navi_build_route_on_map',
        uri='build_route_on_map?lat_to=37.5&lon_to=55.7'
    )


def test_home_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('eve gidelim'),  # let's go home
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="'ev' noktasına rota hesaplanıyor",
        sub_name='navi_build_route_on_map',
        uri='build_route_on_map?lat_to=12.34&lon_to=56.78'
    )


def test_work_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('işe gidelim'),  # Let's go to work
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="'iş' noktasına rota hesaplanıyor",
        sub_name='navi_build_route_on_map',
        uri='build_route_on_map?lat_to=15.34&lon_to=36.78'
    )


def test_work_turkish2(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('iş'),  # work
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="'iş' noktasına rota hesaplanıyor",
        sub_name='navi_build_route_on_map',
        uri='build_route_on_map?lat_to=15.34&lon_to=36.78'
    )


def test_confirmation_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('evet'),  # да
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Haritada bul 'evet'",
        sub_name='navi_map_search',
        uri='map_search?text=evet'
    )


def test_cancel_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('iptal'),  # отмена
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Haritada bul 'iptal'",
        sub_name='navi_map_search',
        uri='map_search?text=iptal'
    )


def test_add_point_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('sağ şeritte kaza'),  # авария в правом ряду
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Yol uyarısı ekle: 'Trafik kazası'",
        sub_name='navi_add_point',
        uri='add_point?category=0&comment='
    )


def test_add_point_with_confirmation_support_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('sağ şeritte kaza'),  # авария в правом ряду
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'additional_options': {
                'supported_features': ['confirmation']
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Yol uyarısı ekle: 'Trafik kazası'",
        sub_name='navi_add_point',
        uri='add_point?category=0&comment=&confirmation=1'
    )


def test_add_point1_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('onarımı doğru şeride koyun'),  # поставить точку ремонт в правом ряду
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Haritada bul 'onarımı doğru şeride koyun'",
        sub_name='navi_map_search',
        uri='map_search?text=onar%C4%B1m%C4%B1%20do%C4%9Fru%20%C5%9Feride%20koyun'
    )


def test_bug_report_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('dönüş yok'),  # отсутствует разворот
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Haritada bul 'dönüş yok'",
        sub_name='navi_map_search',
        uri='map_search?text=d%C3%B6n%C3%BC%C5%9F%20yok'
    )


def test_show_layer_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('trafik sıkışıklığını göster'),  # покажи пробки
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Göster 'Trafik durumunu'",
        sub_name='navi_traffic',
        uri='traffic?traffic_on=1'
    )


def test_hide_layer_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('trafik sıkışıklığını gizle'),  # скрой пробки
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Göster 'Trafik durumunu'",
        sub_name='navi_traffic',
        uri='traffic?traffic_on=1'
    )


def test_add_talk_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('Herkese ufak bir konuşma atın'),  # кинь разговорчик привет всем
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Haritada bul 'Herkese ufak bir konuşma atın'",
        sub_name='navi_map_search',
        uri='map_search?text=Herkese%20ufak%20bir%20konu%C5%9Fma%20at%C4%B1n'
    )


def test_route_reset_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('sıfırlama rotası'),  # сбрось маршрут
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Haritada bul 'sıfırlama rotası'",
        sub_name='navi_map_search',
        uri='map_search?text=s%C4%B1f%C4%B1rlama%20rotas%C4%B1'
    )


def test_bad_favourites_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('yanlış favorileri kontrol et'),  # проверим некорректный favourites
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(
                favourites=[('Büyükannenin Evi', '55.3', 'unknow', 'адрес дома бабушки')]),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': {
                'text': 'Üzgünüm, anlayamadım',
                'type': 'simple_text',
                'tag': None
            },
            'cards': [{
                'text': 'Üzgünüm, anlayamadım',
                'type': 'simple_text',
                'tag': None
            }],
            'directives': [{
                'type': 'client_action',
                'name': 'open_uri',
                'sub_name': 'navi_map_search',
                'payload': {
                    'uri': 'yandexnavi://map_search?text=yanl%C4%B1%C5%9F%20favorileri%20kontrol%20et',
                }
            }],
            'suggest': None,
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {
            'output_speech': None,
            'should_listen': False,
            'directives': [],
        }
    }


def test_route_route_turkish(navi_client):
    mock_data = {
        'street': 'Lev Tolstoy Caddesi, 16',  # улица Льва Толстого, 16
        'house': '16',
        'name': 'Lev Tolstoy Caddesi, 16',  # улица Льва Толстого, 16
        'lat': 55.733842,
        'lon': 37.588144,
        'description': 'Moskova, Rusya'  # Москва, Россия
    }

    with geosearch_geo_mock(**mock_data):
        response = _request(
            navi_client,
            {
                'event': _voice_input('Lev Tolstoy 16'),  # Льва Толстого 16
                'location': {
                    'lat': 55.732616,
                    'lon': 37.585889,
                    'accuracy': 26.63999938964844,
                    'recency': 388950,
                    'speed': 23.5
                },
                'device_state': _generate_device_state_4_navi(points=[]),
            },
            lang='tr-TR',
        )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="'Lev Tolstoy Caddesi, 16' noktasına rota hesaplanıyor",
        sub_name='navi_build_route_on_map',
        uri='build_route_on_map?lat_to=55.733842&lon_to=37.588144'
    )


def test_route_search_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('Lev Tolstoy 16'),  # Льва Толстого 16
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Haritada bul 'Lev Tolstoy 16'",
        sub_name='navi_map_search',
        uri='map_search?text=Lev%20Tolstoy%2016'
    )


def test_show_me_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('ben neredeyim'),  # где я
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Haritada bul 'ben neredeyim'",
        sub_name='navi_map_search',
        uri='map_search?text=ben%20neredeyim'
    )


def test_show_parking_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('park yeri göster'),  # покажи парковки
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Haritada bul 'park yeri göster'",
        sub_name='navi_map_search',
        uri='map_search?text=park%20yeri%20g%C3%B6ster'
    )


def test_find_parking_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('park yeri bulmak'),  # найди парковку
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Haritada bul 'park yeri bulmak'",
        sub_name='navi_map_search',
        uri='map_search?text=park%20yeri%20bulmak'
    )


def test_route_view_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input('rotaya genel bakış'),  # обзор маршрута
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == get_sk_response(
        text="Haritada bul 'rotaya genel bakış'",
        sub_name='navi_map_search',
        uri='map_search?text=rotaya%20genel%20bak%C4%B1%C5%9F'
    )


def test_empty_turkish(navi_client):
    response = _request(
        navi_client,
        {
            'event': _voice_input(''),
            'location': {
                'lat': 55.732616,
                'lon': 37.585889,
                'accuracy': 26.63999938964844,
                'recency': 388950,
                'speed': 23.5
            },
            'device_state': _generate_device_state_4_navi(),
        },
        lang='tr-TR',
    )

    assert response.status_code == 200
    assert json.loads(response.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': {
                'text': 'Üzgünüm, anlayamadım',
                'type': 'simple_text',
                'tag': None
            },
            'cards': [{
                'text': 'Üzgünüm, anlayamadım',
                'type': 'simple_text',
                'tag': None
            }],
            'directives': [{
                'type': 'client_action',
                'name': 'open_uri',
                'sub_name': 'navi_map_search',
                'payload': {
                    'uri': 'yandexnavi://map_search?text=',
                }
            }],
            'suggest': None,
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {
            'output_speech': None,
            'should_listen': False,
            'directives': [],
        }
    }


def test_server_action_turkish(navi_client):
    mock_data = {
        'street': 'Lenin Bulvarı',  # проспект Ленина
        'house': '43',
        'name': 'Lenin Bulvarı, 43',  # проспект Ленина, 43
        'lat': 55.797771,
        'lon': 37.945368,
        'description': 'Balaşiha, Moskova Bölgesi, Rusya'  # Балашиха, Московская область, Россия
    }
    with geosearch_geo_mock(**mock_data):
        response = _request(
            navi_client,
            {
                'event': {
                    'name': 'on_reset_session',
                    'payload': {
                        'mode': 'clear_history'
                    },
                    'type': 'server_action'
                },
                'location': {
                    'lat': 55.732616,
                    'lon': 37.585889,
                    'accuracy': 26.63999938964844,
                    'recency': 388950,
                    'speed': 23.5
                },
                'device_state': _generate_device_state_4_navi(),
            },
            lang='tr-TR',
        )

    assert response.status_code == 200
    assert json.loads(response.content) == {
        'header': get_response_header('123'),
        'response': {
            "cards": [],
            "suggest": None,
            "meta": [],
            "directives": [],
            "card": None,
            'features': {},
            'experiments': {}
        },
        'voice_response': {
            'output_speech': None,
            'should_listen': False,
            'directives': [],
        }
    }
