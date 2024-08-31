# coding: utf-8

from builtins import object
import copy
import re

from alice.analytics.operations.priemka.alice_parser.visualize.visualize_video import (
    VIDEO_SEARCH_GALLERY_PATTERN,
    SHOW_TV_GALLERY_PATTERN,
)


REMOVE_ACTION_KEYS = ('voice_url', 'url')


class WeatherUnifier(object):
    first_punctuation_mark_re = re.compile(r'^([^.,]+)')
    date_with_month_re = re.compile(
        ur'^\d+ (января|февраля|марта|апреля|мая|июня|июля|августа|сентября|октября|ноября|декабря)', re.U)
    temperatures_interval_re = re.compile(ur'от [+\-\d]+ до [+\-\d]+$', re.U)
    temperature_re = re.compile(r'[+\-\d]+$')
    now_forecast_re = re.compile(ur'^(Сейчас в [\w\-]+, [+\-\d]+)', re.U)

    fixed_degrees = '<TEMPERATURE_DEGREES>'
    fixed_date = '<DATE>'
    alice_interjections = {u'По моим данным'}

    @classmethod
    def unify(cls, answer):
        mo = cls.now_forecast_re.match(answer)
        if not mo:
            mo = cls.first_punctuation_mark_re.match(answer)
            if not mo or mo.group(1) in cls.alice_interjections:
                return answer

        answer = mo.group(1)
        answer = cls.date_with_month_re.sub(cls.fixed_date, answer)
        answer = cls.temperatures_interval_re.sub(cls.fixed_degrees, answer)
        answer = cls.temperature_re.sub(cls.fixed_degrees, answer)

        return answer


def is_weather(record):
    return record['generic_scenario'] == 'weather'


def is_tv_stream(record):
    return record['intent'] == 'personal_assistant\tscenarios\ttv_stream'


def is_video_play(record):
    return record['generic_scenario'] == 'video'


# подготовка данных

def prepare_hashable_for_weather(data, req_id):
    if data.get('answer'):
        data['answer'] = WeatherUnifier.unify(data['answer'])
        data['req_id'] = req_id


def prepare_hashable_for_tv_stream(data):
    """
    Подготовливает данные, по которым считается hashsum для сценария "Список телеканалов"
    inplace заменяет часто меняющиеся данные на плейсхолдеры
    :param dict data:
    :return:
    """
    tv_channels_re = re.compile(SHOW_TV_GALLERY_PATTERN.format('\\d+'), re.U)

    if data.get('action') and tv_channels_re.match(data['action']):
        data['action'] = SHOW_TV_GALLERY_PATTERN.format('<CHANNELS_COUNT>') + '<CHANNELS_ITEMS>'


def prepare_hashable_for_video_play(data):
    video_play_re = re.compile(VIDEO_SEARCH_GALLERY_PATTERN.format('\\d+') + r'(.*?)\n1\. (.*?)\n2\. (.*?)\n3\. (.*?)\n', re.U)
    if data.get('action'):
        matches = video_play_re.match(data.get('action'))
        if not matches:
            return

        # первый документ плюс 2 и 3 в любом порядке
        data['action'] = [matches.group(2)] + sorted([matches.group(3), matches.group(4)])


def is_screenshot_answer(action):
    return action.get('answer', '') == '...'


def need_screenshot_hashsum(record, action, is_quasar):
    """
    Определяет, нужно ли учитывать в hashsum ещё и hashsum от данных на скриншоте
    Для колонки не нужен
    Для ПП:
        * в случае погоды отличной от '...', screenshot_hashsum не нужен
        * иначе нужен

    :param dict record:
    :param dict action:
    :param bool is_quasar:
    :return bool:
    """
    if is_quasar:
        return False

    if not record.get('screenshot_hashsum'):
        return False

    if is_weather(record):
        return is_screenshot_answer(action)

    return True


def get_hashable(record, action, is_quasar=True):
    """
    Формирует объект, по которому будет вычисляться hashsum для запроса
    Вызывается для всех запросов
    Для ПП вместо скриншота использует хэшсам от данных, по которым рисуется скриншот
    :param dict record:
    :param dict action:
    :param bool is_quasar:
    :return dict:
    """
    if not action:
        return ''

    hashable = copy.deepcopy(action)
    for key in REMOVE_ACTION_KEYS:
        if key in hashable:
            del hashable[key]

    if is_weather(record):
        prepare_hashable_for_weather(hashable, record['req_id'])
    elif is_tv_stream(record):
        prepare_hashable_for_tv_stream(hashable)
    elif is_video_play(record):
        prepare_hashable_for_video_play(hashable)

    if need_screenshot_hashsum(record, action, is_quasar):
        hashable['screenshot_hashsum'] = record['screenshot_hashsum']

    return hashable
