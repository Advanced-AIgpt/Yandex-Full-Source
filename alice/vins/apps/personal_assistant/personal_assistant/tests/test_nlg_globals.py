# coding: utf-8
from __future__ import unicode_literals

import pytest
from personal_assistant.nlg_globals import (
    username_prefix,
    username_infix,
    username_suffix,
    username_prefix_if_needed,
    geodesic_distance,
    render_traffic_forecast,
)
from personal_assistant.app import UserInfoContainer


@pytest.mark.parametrize('username, result', [
    ('Петя', ', Петя'),
    ('', ''),
    (None, '')
])
def test_username_suffix(username, result):
    context = {'context': {'userinfo': UserInfoContainer(username, is_silent=False)}}
    assert username_suffix(context) == result


@pytest.mark.parametrize('username, result', [
    ('Петя', ', Петя,'),
    ('', ''),
    (None, '')
])
def test_username_infix(username, result):
    context = {'context': {'userinfo': UserInfoContainer(username, is_silent=False)}}
    assert username_infix(context) == result


@pytest.mark.parametrize('username, begin, result', [
    ('Петя', 'Раз', 'Петя, раз'),
    ('', 'Два', 'Два'),
    (None, 'Три', 'Три'),
    ('Петя', '', 'Петя,'),
    ('', '', ''),
    (None, '', '')
])
def test_username_prefix(username, begin, result):
    context = {'context': {'userinfo': UserInfoContainer(username, is_silent=False)}}
    assert username_prefix(context, begin_text=begin) == result


@pytest.mark.parametrize('username, begin, is_used, is_silent, result', [
    ('Петя', 'Раз', True, False, 'Раз'),
    ('Петя', 'Раз', False, False, 'Петя, раз'),
    ('', 'Два', True, False, 'Два'),
    ('', 'Два', False, False, 'Два'),
    ('Петя', '', True, False, ''),
    ('Петя', '', False, False, 'Петя,'),
    ('Петя', 'Раз', False, True, '<vins_only_text>Петя, </vins_only_text>раз')
])
def test_username_prefix_if_needed(username, begin, is_used, is_silent, result):
    cont = UserInfoContainer(username, is_silent)
    if is_used:
        cont.username()
    context = {'context': {'userinfo': cont}}
    assert username_prefix_if_needed(context, begin_text=begin) == result


@pytest.mark.parametrize('point_a, point_b, expected_distance', [
    ({'location': {'lat': 55.7522200, 'lon': 37.6155600}},
     {'location': {'lat': 37.7749300, 'lon': -122.4194200}}, 9446000),  # moscow vs san francisco
    ({'location': {'lat': 55.7522200, 'lon': 37.6155600}},
     {'location': {'lat': 56.1365500, 'lon': 40.3965800}}, 178000),  # moscow vs vladimir
    ({'location': {'lat': -22.9027800, 'lon': -43.2075000}},
     {'location': {'lat': 56.1365500, 'lon': 40.3965800}}, 11725000),  # vladimir vs rio de janeiro
    ({'location': {'lat': 55.751999, 'lon': 37.617734}},
     {'location': {'lat': 55.734232, 'lon': 37.587665}}, 2720),  # kremlin vs yandex office
    ({}, {}, None)
])
def test_geodesic_distance(point_a, point_b, expected_distance):
    calculated_distance = geodesic_distance(point_a, point_b)
    if expected_distance is None:
        assert calculated_distance is None
    else:
        assert abs(calculated_distance-expected_distance) / (expected_distance + 1) < 0.99


@pytest.mark.parametrize('current, forecast, expected_text', [
    (None, [{'hour': 13, 'score': 8}, {'hour': 14, 'score': 7}, {'hour': 15, 'score': 6}],
     'К 15 часам пробки упадут до 6 баллов'),
    (None, [{'hour': 13, 'score': 8}, {'hour': 14, 'score': 7}, {'hour': 15, 'score': 9}],
     'К 15 часам пробки вырастут до 9 баллов'),
    (None, [{'hour': 13, 'score': 8}, {'hour': 14, 'score': 8}, {'hour': 15, 'score': 8}],
     'В ближайшие 3 часа пробки останутся на уровне 8 баллов'),
    (None, [{'hour': 13, 'score': 8}, {'hour': 14, 'score': 7}, {'hour': 15, 'score': 8}],
     'В ближайшие 3 часа пробки останутся на уровне 7-8 баллов'),
    (None, [], ''),
    (9, [{'hour': 13, 'score': 8}, {'hour': 14, 'score': 8}, {'hour': 15, 'score': 8}],
     'В ближайшие 3 часа пробки останутся на уровне 8-9 баллов'),
    (10, [{'hour': 13, 'score': 8}, {'hour': 14, 'score': 8}, {'hour': 15, 'score': 8}],
     'К 13 часам пробки упадут до 8 баллов'),
])
def test_render_traffic_forecast(current, forecast, expected_text):
    assert render_traffic_forecast(current, forecast) == expected_text
