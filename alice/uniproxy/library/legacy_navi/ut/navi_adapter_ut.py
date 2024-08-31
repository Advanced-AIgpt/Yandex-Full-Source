# coding: utf-8

import base64

from alice.uniproxy.library.legacy_navi.navi_adapter import dont_understand_command, NaviAPIResponse


def _check(actual_command, expected_data_command):
    assert actual_command == {
        'app': 'navi',
        'cmd': 'exec',
        'data': base64.b64encode(str.encode('<yari>' + expected_data_command + '</yari>', encoding='utf-8')).decode(),
    }


def test_map_search():
    url = 'yandexnavi://map_search?text=%D0%B7%D0%B0%D0%BF%D1%80%D0%B0%D0%B2%D0%BA%D1%83'
    text = 'найди заправку'
    expected_data = '<map_search description="Поиск на карте \'заправку\'">' \
                    '<text>заправку</text>' \
                    '</map_search>'

    navi_response = NaviAPIResponse(url, text, "Поиск на карте 'заправку'")
    actual_command = navi_response.build_commands([])
    _check(actual_command, expected_data)


def test_build_route_on_map():
    url = 'yandexnavi://build_route_on_map?lat_to=15.34&lon_to=36.78'
    text = 'едем до работы'

    expected_data = '<route description="Едем до \'работа\'">' \
                    '<to>' \
                    '<address lat="15.34" lon="36.78" title="" subtitle=""/>' \
                    '</to>' \
                    '</route>'

    navi_response = NaviAPIResponse(url, text, "Едем до 'работа'")
    actual_command = navi_response.build_commands([])
    _check(actual_command, expected_data)


def test_confirmation():
    url = 'yandexnavi://external_confirmation?confirmed=1'
    text = 'да'

    expected_data = '<confirmation>Yes</confirmation>'

    navi_response = NaviAPIResponse(url, text, 'Подтверждение принято')
    actual_command = navi_response.build_commands([])
    _check(actual_command, expected_data)


def test_cancel():
    url = 'yandexnavi://external_confirmation?confirmed=0'
    text = 'отмена'

    expected_data = '<confirmation>No</confirmation>'

    navi_response = NaviAPIResponse(url, text, 'Отменяю действие')
    actual_command = navi_response.build_commands([])
    _check(actual_command, expected_data)


def test_add_point():
    url = 'yandexnavi://add_point?category=0&where=%D0%BF%D1%80%D0%B0%D0%B2%D1%8B%D0%B9%20%D1%80%D1%8F%D0%B4&comment='
    text = 'авария в правом ряду'

    expected_data = '<add_point description="Поставить точку: \'ДТП\', \'правый ряд\'"' \
                    ' category="0" where="правый ряд">' \
                    '</add_point>'

    navi_response = NaviAPIResponse(url, text, "Поставить точку: 'ДТП', 'правый ряд'")
    actual_command = navi_response.build_commands([])
    _check(actual_command, expected_data)


def test_bug_report():
    url = 'yandexnavi://add_point?category=7&' \
          'comment=%D0%9D%D0%B5%D1%82%20%D0%BF%D0%BE%D0%B2%D0%BE%D1%80%D0%BE%D1%82%D0%B0'
    text = 'отсутствует разворот'

    expected_data = '<add_point description="Поставить точку: \'Ошибка\', с комментарием: \'Нет поворота\'"' \
                    ' category="7">Нет поворота</add_point>'

    navi_response = NaviAPIResponse(url, text, "Поставить точку: 'Ошибка', с комментарием: 'Нет поворота'")
    actual_command = navi_response.build_commands([])
    _check(actual_command, expected_data)


def test_show_layer():
    url = 'yandexnavi://traffic?traffic_on=1'
    text = 'покажи пробки'

    expected_data = '<traffic description="Показать пробки">on</traffic>'

    navi_response = NaviAPIResponse(url, text, "Показать пробки")
    actual_command = navi_response.build_commands([])
    _check(actual_command, expected_data)


def test_hide_layer():
    url = 'yandexnavi://traffic?traffic_on=0'
    text = 'скрой пробки'

    expected_data = '<traffic description="Скрыть пробки">off</traffic>'

    navi_response = NaviAPIResponse(url, text, "Скрыть пробки")
    actual_command = navi_response.build_commands([])
    _check(actual_command, expected_data)


def test_parking():
    url = 'yandexnavi://show_ui/map?carparks_enabled=1'
    text = 'покажи парковки'

    expected_data = '<parking description="Показать парковки">on</parking>'

    navi_response = NaviAPIResponse(url, text, "Показать парковки")
    actual_command = navi_response.build_commands(['parking'])
    _check(actual_command, expected_data)


def test_parking_no_support():
    url = 'yandexnavi://show_ui/map?carparks_enabled=1'
    text = 'покажи парковки'

    expected_data = '<map_search description="Показать парковки"><text>покажи парковки</text></map_search>'

    navi_response = NaviAPIResponse(url, text, "Показать парковки")
    actual_command = navi_response.build_commands([])
    _check(actual_command, expected_data)


def test_show_me():
    url = 'yandexnavi://show_user_position'
    text = 'где я'

    expected_data = '<show_me description="Поиск текущего местоположения"/>'

    navi_response = NaviAPIResponse(url, text, "Поиск текущего местоположения")
    actual_command = navi_response.build_commands(['show_me'])
    _check(actual_command, expected_data)


def test_show_me_no_support():
    url = 'yandexnavi://show_user_position'
    text = 'где я'

    expected_data = '<ignore/>'

    navi_response = NaviAPIResponse(url, text, "Поиск текущего местоположения")
    actual_command = navi_response.build_commands([])
    _check(actual_command, expected_data)


def test_parking_route():
    url = 'yandexnavi://carparks_route'
    text = 'найди парковку'

    expected_data = '<parking_route description="Поиск парковок"/>'

    navi_response = NaviAPIResponse(url, text, "Поиск парковок")
    actual_command = navi_response.build_commands(['parking'])
    _check(actual_command, expected_data)


def test_parking_route_no_support():
    url = 'yandexnavi://carparks_route'
    text = 'найди парковку'

    expected_data = '<map_search description="Поиск парковок"><text>найди парковку</text></map_search>'

    navi_response = NaviAPIResponse(url, text, "Поиск парковок")
    actual_command = navi_response.build_commands([])
    _check(actual_command, expected_data)


def test_ignore_command():
    url = 'yandexnavi://unsupported_command?param1=value1&param2=value2'
    text = 'Не поддерживаемая комманда'

    expected_data = '<ignore/>'

    navi_response = NaviAPIResponse(url, text, "Не поддерживаемая комманда")
    actual_command = navi_response.build_commands([])
    _check(actual_command, expected_data)


def test_dont_understand():
    expected_data = '<dont_understand>Извините, непонятно</dont_understand>'

    actual_command = dont_understand_command('Извините, непонятно')
    _check(actual_command, expected_data)
