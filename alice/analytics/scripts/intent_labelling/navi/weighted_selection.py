#!/usr/bin/env python
# encoding: utf-8
import re
import os.path
from random import random, randrange

from nile.api.v1 import clusters, Record
from nile.api.v1.filters import equals

from utils.nirvana.op_caller import call_as_operation


def rx(pat):
    return re.compile(pat).search


INTENT_WEIGHTS = [
    (rx('.*(session_start|onboarding|general_conversation_dummy|rude|harassment).*'), 0),
    ('external_skill', 0),
    ('map_search_url', 0.25),
    ('find_poi', 0.003),
    ('show_route', 0.06),
    ('general_conversation', 0.15),
    ('user_reactions', 0.3),
    ('get_weather', 0.4),
    ('cancel', 0.7),
    (rx('.*(what_can_you_do|hello|ok|goodbye|how_are_you).*'), 0.1),
]


def selector(records):
    for rec in records:
        n = randrange(0, len(rec.session))
        r = rec.session[n]

        intent = r.get('restored') or r['intent']
        get = True
        for pat, w in INTENT_WEIGHTS:
            if isinstance(pat, basestring):
                if pat in intent:
                    get = (random() < w)
                    break
            else:
                if pat(intent):
                    get = (random() < w)
                    break

        if get:
            if n == 0:
                dialog = [r['_query']]
            else:
                prev = rec.session[n-1]
                dialog = [prev['_query'], prev['_reply'], r['_query']]
            reqid = abs(hash(tuple(dialog)))
            yield Record(dialog=dialog, directive=intent, reqid=reqid)


def select_dialogs(date,
                   out_root,
                   out_prefix='',
                   row_limit=5000,
                   app='navigator',
                   session_root='//home/voice/dialog/sessions'):
    """
    Выборка диалогов для разметки интентов
    :param str date: Дата, за которую нужно выгружать
    :param str out_root: В какую YT-папку выгружать
    :param str out_prefix: Префикс будет дописан к имени выгруженной таблицы
    :param int row_limit: Сколько диалогов выгружать.
        Не рекомендуется загружать одним пулом больше 10k,
          т.к. агрегация оценок будет работать слишком долго и может даже упасть по таймауту.
        Если всё же нужно выгрузить много строк за один день, лучше потом их разбить на пулы по несколько тысяч
    :param str app: Фильтр по приложению
    :param str session_root: YT-папка где лежат диалоговые сессии
    :return:
    """
    out_path = os.path.join(out_root, out_prefix + date)
    fraction = 1e-04 * row_limit  # Для нормальных табличек с сессиями это доля с запасом (в случае app='navigator')
    job = clusters.Hahn().job()
    inp = job.table(
        os.path.join(session_root, date)
    ).project(
        'session', 'app',
    ).filter(
        equals('app', app)
    ).random(
        fraction=fraction,  # Дешёвым способом отрезаем кусочек, чтобы не обрабатывать маппером всё подряд
    ).map(
        selector,
    ).unique(
        'reqid',
    ).random(
        count=row_limit,  # Дорогим способом отрезаем ровно столько строк, сколько нужно
    ).put(
        out_path,
        schema={
            'dialog': [str],
            'directive': str,
            'reqid': int,
        })
    with job.driver.transaction():
        job.run()
    return {"cluster": "hahn", "table": out_path}


if __name__ == '__main__':
    call_as_operation(select_dialogs)
    #select_dialogs('2018-06-02', out_root='//home/voice/yoschi/navi/src/', out_prefix='pa_navi_', row_limit=5000)  # Local test

