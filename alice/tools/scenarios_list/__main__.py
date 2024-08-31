# coding: utf-8

"""
Yt data prepared by YQL script https://yql.yandex-team.ru/Operations/YEDxI9K3DI_DLoDlQuisl0ftHdOyB8zwovxTfxheTPc=

"""

import io
import logging
import os
import sys
import re

import attr
import click
import requests
import yt.wrapper as yt


logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

WIKI_PAGE = 'alice/scenarios/auto-list'

description_fixlist = {
    'onboarding': 'Что ты умеешь? Во что можно поиграть? Что нового?',
    'weather': 'Погода и дождь',
    'route': 'Построение маршрута',
    'images_what_is_this': 'Распознавание изображений.',
    'feedback': 'Возможность дать фидбек: лайк/дизлайк трека, нажатие на пальцы',
    'player_commands': 'Управление воспроизведением (музыки, видео, радио и т.д)',
    'commands_other': 'Сюда попали все специфичные для разных поверхностей команды (строка, автоголова и т.д.)',
    'sound_commands': 'Команды управления громкостью',
    'nav_url': 'Навигационный сценарий, Открытие сайтов и приложений.',
    'serp': "Открытие SERP'а",
    'alarm': 'Будильники',
    'music_what_is_playing': 'Что сейчас играет',
    'quasar_commands': 'Управление экраном на станции',
    'music_fairy_tale': 'Сказки',
    'direct_gallery': 'Показ рекламы',
    'convert': 'Курс и конвертер валют',
    'get_time': 'Сколько времени',
    'video': 'Сценарий видеосмотрения, который отвечает за поиск видео и фильмов, их проигрывание и покупку.',
    'timer': 'Установка таймеров',
    'repeat': 'Повтори',
    'tv_broadcast': 'Телепрограмма',
    'call': 'Звонилка в экстренные службы и организации',
    'get_date': 'Какое сегодня число',
    'taxi': 'Заказ такси',
    'identity_commands': 'Сценарий регистрации и идентификации пользователей на Станции (aka сценайри знакомства)',
    'navi_commands': 'Команды управления навигатором (включить слой, добавить точку и т.д.)',
    'reminder': 'Напоминания',
    'todo': 'Тудушки',
    'music_ambient_sound': 'Звуки природы',
    'tv_stream': 'ТВ-онлайн (ТВ-Эфир)',
    'sleep_timer': 'Таймер сна',
    'repeat_after_me': 'Повтори за мной',
    'placeholders': 'Тестовые сценарии',
    'music_podcast': 'Покасты',
    'bugreport': 'Отправка багрепорта',
    'avia': 'Авиабилеты',
    'promo': 'Промо сценарии',
    'find_poi': "Сценарий <Поиск по организациям>: просим Алису найти какую-то организацию, получаем ответ с информацией о ней: в частности, адрес и часы работы. Примеры запросов: 'кафе пушкин', 'адрес мрт 24'",
    'video_commands': 'Сценарий различных команд для видео.',
    'bluetooth': "Сценарий выключения/включения блютуза. Просим Алису выключить/включить блютуз. Примеры запросов: 'Выключи блютуз', 'Вруби блютуз'",
}


ignored_product_scenario = {
    '',
    'personal_assistant.scenarios.common.irrelevant',
    'personal_assistant.scenarios.player_dislike',
    'personal_assistant.scenarios.image_what_is_this__ocr_voice',
    'personal_assistant.scenarios.common.irrelevant.external_skill',
}


@attr.s
class Conf:
    name = attr.ib()
    owners = attr.ib()
    description = attr.ib()


@attr.s
class YtLine:
    product_scenario = attr.ib(type=str)
    mm_scenario = attr.ib(type=list)
    requests = attr.ib(type=int)
    is_pp_like_ratio = attr.ib(type=float)
    is_station_ratio = attr.ib(type=float)
    vins_ratio = attr.ib(type=float)


@attr.s
class Scenario:
    product_scenario = attr.ib(type=str)
    mm_scenario = attr.ib(type=list)
    requests = attr.ib(type=int)
    owners = attr.ib()
    description = attr.ib()
    is_pp_like_ratio = attr.ib(type=float)
    is_station_ratio = attr.ib(type=float)
    vins_ratio = attr.ib(type=float)


def iter_files(path):
    for fn in os.listdir(path):
        if fn.endswith('pb.txt'):
            yield (fn, open(os.path.join(path, fn)).read())


def parse_list(name, data):
    ptrn = r'^%s: ?\[(?P<list>[^$.]*?)\]$' % name
    raw = re.search(ptrn, data, flags=re.MULTILINE).groupdict()['list']
    return [i.strip()[1:-1] for i in raw.split(',')]


def parse_string(name, data):
    ptrn = r'^%s: ?"(?P<data>.*)"' % name
    return re.search(ptrn, data, flags=re.MULTILINE).groupdict()['data']


def parse_conf(fn, data):
    data = re.sub(r'#.*', '', data)
    name = parse_string('Name', data)
    owners = parse_list('Owners', data)
    description = parse_string('Description', data)
    return Conf(name, owners, description)


def get_data_from_configs(path):
    data = {}
    for fn, content in iter_files(path):
        msg = parse_conf(fn, content)
        data[msg.name] = msg

    return data


def get_data_from_yt(path, yt_token, yt_proxy):
    yt.config['proxy']['url'] = yt_proxy
    yt.config['token'] = yt_token

    data = {}

    for line in yt.read_table(path, format=yt.YsonFormat(encoding=None)):
        psn = (line[b'product_scenario'] or b'').decode('utf-8')
        data[psn] = YtLine(
            product_scenario=psn,
            mm_scenario=dict(map(
                lambda x: (x[0][0].decode('utf-8'), x[1]),
                line[b'mm_scenario']
            )),
            requests=line[b'cnt'],
            is_pp_like_ratio=line[b'is_pp_like_ratio'],
            is_station_ratio=line[b'is_station_ratio'],
            vins_ratio=line[b'vins_ratio'],
        )

    return data


def merge_data(conf_data, yt_data):
    data = []

    def get_owners_and_description(mm_sc_name):
        conf = conf_data[mm_sc_name]
        owners = conf.owners
        description = conf.description
        return owners, description

    for product_scenario, yt_item in yt_data.items():
        if product_scenario in ignored_product_scenario:
            continue

        mm_scenarios = yt_item.mm_scenario.copy()
        mm_scenarios.pop('Vins', None)
        mm_scenarios.pop('alice.vins', None)
        mm_scenarios.pop('', None)

        owners = []
        description = ""

        if len(mm_scenarios) == 1:
            sc = list(mm_scenarios)[0]
            owners, description = get_owners_and_description(sc)
        elif len(mm_scenarios) == 0:
            description = description_fixlist.get(product_scenario, "")
        else:
            owners = set(sum([conf_data[i].owners for i in mm_scenarios if i in conf_data], []))
            best_scenario = sorted(mm_scenarios.items(), key=lambda x: -x[1])[0][0]
            _, description = get_owners_and_description(best_scenario)

        data.append(Scenario(
            product_scenario=product_scenario,
            mm_scenario=yt_item.mm_scenario,
            requests=yt_item.requests,
            owners=owners,
            description=description_fixlist.get(product_scenario, description),
            is_pp_like_ratio=yt_item.is_pp_like_ratio,
            is_station_ratio=yt_item.is_station_ratio,
            vins_ratio=yt_item.vins_ratio,
        ))

    return data


def render_owners(owners_list):
    res = []
    for owner in sorted(owners_list, reverse=True):
        if owner.startswith('abc:'):
            res.append('* https://abc.yandex-team.ru/services/{}/'.format(owner[4:]))
        else:
            res.append(f'* кто:{owner}')
    return '\n'.join(res)


def render_mm_scenario(mm_scenarios):
    res = []
    total = sum(mm_scenarios.values())
    vins_scenarios = {'Vins', 'alice.vins'}
    vins_total = 0

    for name, cnt in mm_scenarios.items():
        prc = cnt / total * 100
        if name not in vins_scenarios:
            res.append((name, prc))
        else:
            vins_total += cnt

    res.append(('Vins', vins_total / total * 100))
    res.sort(key=lambda x: x[1], reverse=True)

    return '\n'.join(f'* %%{name}%%: {prc:.2f}%' for name, prc in res)


def upload_to_wiki(data, wiki_page, wiki_token):
    res = []
    url = os.path.join(
        'https://wiki-api.yandex-team.ru/_api/frontend/',
        wiki_page,
        '.grid/replace'
    )
    headers = {'Authorization': f"OAuth {wiki_token}"}

    for item in data:
        row = {
            '100': item.product_scenario,
            '101': render_owners(item.owners),
            '102': item.description,
            '103': render_mm_scenario(item.mm_scenario),
            '104': int(item.is_station_ratio * 100),
            '105': int(item.is_pp_like_ratio * 100),
            '106': int(item.vins_ratio * 100),
            '107': item.requests,
        }
        res.append(row)

    headers = {'Authorization': f"OAuth {wiki_token}"}

    resp = requests.post(url, json={'data': res}, headers=headers)
    import json
    req_body = json.dumps(json.loads(resp.request.body), ensure_ascii=False, indent=2)
    if not resp.ok:
        raise RuntimeError(f'Failed to post wiki page on path "{wiki_page}". Response: {resp.text}. Request: {req_body}')


@click.command()
@click.argument('config_dir', required=True)
@click.argument('yt_table', required=True)
@click.option('--yt-token')
@click.option('--yt-proxy')
def main(config_dir, yt_table, yt_token, yt_proxy):
    yt_token = yt_token or os.environ['YT_TOKEN']
    yt_proxy = yt_proxy or os.environ['YT_PROXY']

    conf_data = get_data_from_configs(config_dir)
    yt_data = get_data_from_yt(yt_table, yt_token, yt_proxy)

    merged = merge_data(conf_data, yt_data)
    upload_to_wiki(merged, WIKI_PAGE, yt_token)


if __name__ == '__main__':
    main()
