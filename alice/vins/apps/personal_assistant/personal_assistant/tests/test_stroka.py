# coding: utf-8
from __future__ import unicode_literals

import pytest
import logging
import functools
from uuid import uuid4 as gen_uuid
from vins_core.dm.request import AppInfo

logger = logging.getLogger(__name__)


def assert_dc(dict_a, dict_b):
    for k in dict_b:
        assert k in dict_a, 'No key "%s" in %r' % (k, dict_a)
        if isinstance(dict_a[k], dict):
            assert_dc(dict_a[k], dict_b[k])
        else:
            assert dict_a[k] == dict_b[k], '"%r" != "%r"' % (dict_a[k], dict_b[k])


@pytest.fixture(scope='module')
def app_info():
    return AppInfo(
        app_id='winsearchbar',
        app_version='10.00',
        os_version='5.1.1',
        platform='windows'
    )


@pytest.fixture(scope='module', params=['winsearchbar', 'YaBro.dev'])
def app_info_with_yabro(request):
    return AppInfo(
        app_id=request.param,
        app_version='10.00',
        os_version='5.1.1',
        platform='windows'
    )


@pytest.fixture(scope='function')
def f_txt(vins_app, app_info):
    return functools.partial(vins_app.handle_utterance, gen_uuid(), app_info=app_info)


@pytest.fixture(scope='function')
def f(vins_app, app_info):
    return functools.partial(vins_app.handle_utterance, gen_uuid(), text_only=False, app_info=app_info)


@pytest.fixture(scope='function')
def f_txt_with_yabro(vins_app, app_info_with_yabro):
    return functools.partial(
        vins_app.handle_utterance, gen_uuid(),
        app_info=app_info_with_yabro,
        experiments=['stroka_yabro']
    )


@pytest.mark.parametrize("utterance,text_response", [
    ('выключи комп', 'Выключаю компьютер'),
    ('спящий режим', 'Ок, засыпаю'),
    ('перезагрузись', 'Перезагружаю'),
    ('включи звук', 'OK, сейчас я включу звук'),
    ('выключи звук', 'OK, сейчас я выключу звук'),
    ('открой пуск', 'OK, открываю меню пуск'),
    ('открой настройки', 'OK, открываю настройки'),
    ('открой браузер', 'OK, открываю браузер'),
    ('открой яндекс браузер', 'OK, открываю Яндекс Браузер'),
    ('открой папку мультики', 'Открываю'),
    ('открой флешкарту', 'OK, открываю флешку'),
    ('открой диск цэ', 'OK, открываю диск c'),
    ('открой файл выпускной', 'Открываю')
])
def test_nlg(f_txt_with_yabro, utterance, text_response):
    assert f_txt_with_yabro(utterance) == text_response


@pytest.mark.parametrize("utterance, cmd_name", [
    ('открой новую вкладку', 'new_tab'),
    pytest.param('вернись назад', 'go_back', marks=pytest.mark.skip),
    ('алиса почисть историю плиз', 'clear_history'),
    pytest.param('закрыть вкладку', 'close_tab', marks=pytest.mark.skip),
    ('новую вкладку открыла', 'new_tab'),
    ('на домашнюю страницу', 'go_home'),
    ('в скрытный режим перейди', 'open_incognito_mode'),
    ('открой мои закладки', 'open_bookmarks_manager'),
    ('открой историю браузера', 'open_history'),
    ('браузер закрыть', 'close_browser'),
    pytest.param('обнови страничку', 'reload_page', marks=pytest.mark.skip),
    ('верни вкладку', 'restore_tab')
])
def test_navigate_browser(f, utterance, cmd_name):
    assert_dc(
        f(utterance),
        {
            'directives': [{
                'type': 'client_action',
                'name': 'navigate_browser',
                'sub_name': 'pc_navigate_browser' + cmd_name,
                'payload': {
                    'command_name': cmd_name
                }
            }]
        }
    )


@pytest.mark.skip(reason='This command was temporarily removed. See DIALOG-1021.')
@pytest.mark.parametrize("utterance, tab_number", [
    ('открой первую вкладку', 1),
    ('перейди на вторую вкладку', 2),
    ('вкладка номер три', 3),
    ('зайди на вкладку четыре', 4),
    ('вкладка пять', 5),
    ('вкладка 6', 6),
    ('на вкладку семь', 7),
    ('на восьмую вкладку', 8)
])
def test_select_tab(f, utterance, tab_number):
    assert_dc(
        f(utterance),
        {
            'directives': [{
                'type': 'client_action',
                'name': 'navigate_browser',
                'sub_name': 'pc_navigate_browser_select_tab',
                'payload': {
                    'command_name': 'select_tab',
                    'tab_number': str(tab_number)
                }
            }]
        }
    )


@pytest.mark.parametrize("utterance", [
    'выключи комп',
    'выруби комп',
    'отключить комп',
    'завершить работу компьютера',
    'выключение компьютера',
    'выключение',
    'выключи компьютер',
    'выключи компьютер пожалуйста милый',
    'закрой пожалуйста компьютер',
    'пожалуйста пожалуйста закрой компьютер',
    'закрой пожалуйста компьютер',
    'включить выключить компьютер',
])
def test_power_off(f, utterance):
    assert_dc(
        f(utterance),
        {
            'directives': [{
                'type': 'client_action',
                'name': 'power_off',
                'sub_name': 'pc_power_off',
                'payload': {},
            }],
        }
    )


@pytest.mark.parametrize("utterance", [
    'усыпи компьютер',
    'переведи в спящий режим',
    'уйди в спящий режим',
    'уйди в спячку',
    'поспи немного',
    'сон на компьютере',
    'сон пожалуйста',
    'спящий режим',
])
def test_hibernate(f, utterance):
    assert_dc(
        f(utterance),
        {
            'directives': [{
                'type': 'client_action',
                'name': 'hibernate',
                'sub_name': 'pc_hibernate',
                'payload': {},
            }],
        }
    )


@pytest.mark.parametrize("utterance", [
    'перезагрузи комп',
    'перезагрузись',
    'ребутнись',
    'ребут',
    'перезапустись',
    'перезапусти комп',
    'перезагрузи комп',
    'перезагрузка',
    'давай перезагрузим компьютер',
    'давай ка перезагрузим компьютер'
])
def test_restart_pc(f, utterance):
    assert_dc(
        f(utterance),
        {
            'directives': [{
                'type': 'client_action',
                'name': 'restart_pc',
                'sub_name': 'pc_restart',
                'payload': {},
            }],
        }
    )


@pytest.mark.parametrize("utterance,expected", [
    ('найди фильм аватар на компьютере', 'фильм аватар'),
    ('поищи на компьютере фильм аватар 2', 'фильм аватар 2'),
    ('поищи фильм на компе', 'фильм'),
    ('фотки на ноуте', 'фотки'),
    ('игры на ноутбуке', 'игры'),
    ('косынку на компьютере', 'косынку'),
    ('найди на компьютере ауслоджик', 'ауслоджик'),
    ('найди фотографии на компьютере', 'фотографии')
])
def test_search_local(f, utterance, expected):
    expected = dict(text=expected)
    assert f(utterance)['directives'] == [{
        'type': 'client_action',
        'name': 'search_local',
        'sub_name': 'pc_search_local',
        'payload': expected,
    }]


@pytest.mark.parametrize("utterance,expected", [
    ('покажи папку мои документы', 'мои документы'),
    ('папку мои фото покажи мне пожалуйста', 'мои фото'),
    ('открой папку фильмы', 'фильмы'),
    ('открой папку испания 2017', 'испания 2017'),
    ('открой корзину', 'корзина'),
    ('открой мои документы', 'мои документы'),
    ('открой мой комп', 'мой компьютер'),
    ('открыть корзину', 'корзина'),
    ('открой корзину пожалуйста', 'корзина'),
    ('корзину открой', 'корзина'),
    ('открой загрузки', 'загрузки'),
    ('открой мои загрузки', 'загрузки'),
    ('открой мои закачки', 'загрузки'),
    ('открой папку закачки', 'загрузки'),
    ('открой документы', 'мои документы'),
    ('открой документы пожалуйста', 'мои документы'),
    ('документы открой', 'мои документы')
])
def test_open_folder(f, utterance, expected):
    expected = dict(folder=expected)
    assert f(utterance)['directives'] == [{
        'type': 'client_action',
        'name': 'open_folder',
        'sub_name': 'pc_open_folder',
        'payload': expected,
    }]


@pytest.mark.parametrize("utterance,expected", [
    ('открой файл диплом', 'диплом'),
    ('открой файл выпускной пожалуйста', 'выпускной')
])
def test_open_file(f, utterance, expected):
    expected = dict(file=expected)
    assert f(utterance)['directives'] == [{
        'type': 'client_action',
        'name': 'open_file',
        'sub_name': 'pc_open_file',
        'payload': expected,
    }]


@pytest.mark.parametrize("utterance,text_response", [
    ('открой файл', 'Я могу найти или открыть файл, если сформулируете запрос точнее. Например, "открой файл диплом" или "найди файл договор"'),  # noqa
    ('открой папку', 'Я могу найти или открыть папку, если сформулируете запрос точнее. Например, "открой папку загрузки" или "найди папку фото"'),  # noqa
])
def test_ask_slot(f_txt, utterance, text_response):
    assert f_txt(utterance) == text_response


@pytest.mark.parametrize("utterance", [
    'выключи звук пожалуйста',
    'выруби звук',
    'отруби звук',
    'звук выключи',
    'заглуши звук',
    'отключи звук',
])
def test_mute(f, utterance):
    assert_dc(
        f(utterance),
        {
            'voice_text': 'OK, сейчас я выключу звук',
            'directives': [{
                'type': 'client_action',
                'name': 'mute',
                'sub_name': 'pc_mute',
                'payload': {},
            }],
        }
    )


@pytest.mark.parametrize("utterance", [
    'включи звук пожалуйста',
    'вруби звук',
    'включите звук',
])
def test_unmute(f, utterance):
    assert_dc(
        f(utterance),
        {
            'voice_text': 'OK, сейчас я включу звук',
            'directives': [{
                'type': 'client_action',
                'name': 'unmute',
                'sub_name': 'pc_unmute',
                'payload': {},
            }],
        }
    )


@pytest.mark.parametrize("utterance", [
    'открой браузер',
    'яндекс открой браузер',
    'включи браузер на компьютере',
    'запусти браузер',
    'браузер открой',
])
def test_open_default_browser(f, utterance):
    assert_dc(
        f(utterance),
        {
            'voice_text': 'OK, открываю браузер',
            'directives': [{
                'type': 'client_action',
                'name': 'open_default_browser',
                'sub_name': 'pc_open_default_browser',
                'payload': {},
            }],
        }
    )


@pytest.mark.parametrize("utterance", [
    'открой яндекс браузер',
    'открыть открыть открыть открыть браузер яндекс',
    'запустить яндекс',
    'слушай яндекс запустить яндекс',
    'яндекс слушай запустить яндекс',
    'перейди в браузер яндекса',
    'запусти яндекс браузер',
    'запусти браузер яндекса'
])
def test_open_ya_browser(f, utterance):
    assert_dc(
        f(utterance),
        {
            'voice_text': 'OK, открываю Яндекс Браузер',
            'directives': [{
                'type': 'client_action',
                'name': 'open_ya_browser',
                'sub_name': 'pc_open_ya_browser',
                'payload': {},
            }],
        }
    )


@pytest.mark.parametrize("utterance", [
    'открой флешку',
    'флешку',
    'флешка',
])
def test_open_flash_card(f, utterance):
    assert_dc(
        f(utterance),
        {
            'voice_text': 'OK, открываю флешку',
            'directives': [{
                'type': 'client_action',
                'name': 'open_flash_card',
                'sub_name': 'pc_open_flash_card',
                'payload': {},
            }],
        }
    )


@pytest.mark.parametrize("utterance", [
    'открой пуск',
    'открой меню пуск',
    'покажи меню пуск',
    'меню',
    'меню открой пожалуйста'
])
def test_open_start(f, utterance):
    assert_dc(
        f(utterance),
        {
            'voice_text': 'OK, открываю меню пуск',
            'directives': [{
                'type': 'client_action',
                'name': 'open_start',
                'sub_name': 'pc_open_start',
                'payload': {},
            }],
        }
    )


@pytest.mark.parametrize("utterance,target", [
    ('открой мне настройки учётных записей пользователей', 'useraccount'),
    ('открой мне настройки учётной записи пользователя', 'useraccount'),
    ('открой настройки микрофона', 'microphone'),
    ('настройки микрофона покажи мне пожалуйста', 'microphone'),
    ('настройки индексирования', 'indexing'),
    ('настройки звука', 'sound'),
    ('настройки звук', 'sound'),
    ('настройки экрана блокировки покажи', 'lockscreen'),
    ('настройки громкости', 'volume'),
    ('настройки громкости звука покажи', 'volume'),
    ('открой настройки питания', 'power'),
    ('открой настройки электропитания', 'power'),
    ('открой настройки тем', 'themes'),
    ('открой настройки темы', 'themes'),
    ('открой настройки vpn', 'vpn'),
    ('открой настройки впн', 'vpn'),
    ('открой настройки цветов', 'colors'),
    ('покажи настройки защитника виндовс', 'defender'),
    ('покажи настройки защитника windows', 'defender'),
    ('открой настройки мышки', 'mouse'),
    ('покажи настройки мыши', 'mouse'),
    ('настройки мыши открой', 'mouse'),
    ('настройки меню пуск покажи', 'startmenu'),
    ('настройки система', 'system'),
    ('настройки системы', 'system'),
    ('настройки папки', 'folders'),
    ('настройки папок', 'folders'),
    ('открой настройки даты и времени', 'datetime'),
    ('открой настройки программ по умолчанию', 'defaultapps'),
    ('настройки принтера', 'print'),
    ('настройки дисплей', 'display'),
    ('настройки дисплея', 'display'),
    ('настройки уведомлений', 'notifications'),
    ('настройки обновлений', 'winupdate'),
    ('настройки клавиатуры', 'keyboard'),
    ('настройки архивации', 'archiving'),
    ('параметры архивации', 'archiving'),
    ('настройки режим планшета', 'tabletmode'),
    ('настройки режима планшета', 'tabletmode'),
    ('настройки специальных возможностей', 'accessibility'),
    ('настройки спец возможностей', 'accessibility'),
    ('настройки wifi', 'wifi'),
    ('настройки домашней группы', 'homegroup'),
    ('настройки рабочего стола', 'desktop'),
    ('настройки рабочий стол', 'desktop'),
    ('настройки домашней сети', 'network'),
    ('настройки сети', 'network'),
    ('параметры языка', 'language'),
    ('настройки винды', 'tablo'),
    ('настройки windows', 'tablo'),
    ('настройки брандмауера', 'firewall'),
    ('настройки брандмауэра', 'firewall'),
    ('настройки firewall', 'firewall'),
    ('настройки файервола', 'firewall'),
    ('настройки фаервола', 'firewall'),
    ('открой настройки конфиденциальности', 'privacy'),
    ('открыть параметры', 'tablo'),
    ('настройки принтера', 'print'),
    ('открой удаление программ', 'addremove'),
    ('удаление приложений открыть', 'addremove'),
    ('открой панель управления', 'tablo'),
    ('открой таск менеджер', 'taskmanager'),
    ('открой диспетчер задач', 'taskmanager'),
    ('открой диспетчер устройств', 'devicemanager')
])
def test_open_settings(f, utterance, target):
    assert_dc(
        f(utterance),
        {
            'directives': [{
                'type': 'client_action',
                'name': 'open_settings',
                'sub_name': 'pc_open_settings',
                'payload': {'target': target},
            }],
        }
    )


@pytest.mark.parametrize("utterance,disk", [
    ('открой мне диск це', 'c'),
    ('открой мне диск d', 'd'),
    ('яндекс открой диск ц', 'c'),
])
def test_open_disk(f, utterance, disk):
    assert_dc(
        f(utterance),
        {
            'directives': [{
                'type': 'client_action',
                'name': 'open_disk',
                'sub_name': 'pc_open_disk',
                'payload': {'disk': disk},
            }],
        }
    )
