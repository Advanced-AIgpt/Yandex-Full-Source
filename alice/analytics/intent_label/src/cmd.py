#!/usr/bin/env python
# encoding: utf-8
"""
Функции для локальной отладки
"""
from parse_layout import LayoutConf
from parse_prj import ProjectConf
from sync_prj import IntentProjectRequester
from run_pool import IntentPoolRequester
from tree import LayoutIntentTree


def consistency_test(prj_key):
    """Валидирует консистентность настроек проекта.
    Фактически выполняет те же действия, что и при синхронизации, но не без внесения актуальных изменений.
    В случае проблем, падает с соответствующим сообщением."""

    # Проверяем, что дерево может быть смёрджено и записано в свойства
    prj_conf = ProjectConf(prj_key)
    mt = prj_conf.get_merged_tree()
    mt.to_script()
    mt.to_html()

    # Валидируем хинты и ханипоты
    pool_req = IntentPoolRequester(prj_key)
    pool_req.list_honeypots()


def create_skill(layout_key, toloka_env, owner):
    """
    Создаёт скилл и вставляет его в соответствующий конфиг (не забудь его закоммитить)
    :param str layout_key:
    :param str toloka_env: sandbox, prod or yang
    :param str owner:
    :return:
    """
    return LayoutConf(layout_key).create_skill(toloka_env=toloka_env, owner=owner)


def print_html_tree(prj_key):
    """
    Генерация дерева в html разметке.
    Можно скопипействить в конец инструкции в толоке и она будет автоматом обновляться при изменениях в дереве.
    Важно при этом не вносить в html самостоятельных правок, иначе есть риск, что автоматическое обновление сломается.
    :param prj_key:
    :return:
    """
    print ProjectConf(prj_key).get_merged_tree().to_html()


def print_merged_tree(prj_key):
    """
    Печать смёрдженного дерева для всего проекта.
    :param prj_key:
    :return:
    """
    print ProjectConf(prj_key).get_merged_tree().to_yaml()


def map_layout_intents(layout_key):
    """
    Получить интенты лэйаута.
    Сами интенты будут ключами в словаре.
    Значением служит текстовое обозначение одного из путей в дереве, ведущего к этому интенту.
    :param prj_key:
    :return:
    """
    cnf = LayoutConf(layout_key)
    return LayoutIntentTree(cnf).map_intents_to_path()


def map_prj_intents(prj_key):
    """
    Получить все интенты проекта.
    Сами интенты будут ключами в словаре.
    Значением служит текстовое обозначение одного из путей в дереве, ведущего к этому интенту.
    :param prj_key:
    :return:
    """
    return ProjectConf(prj_key).get_merged_tree().map_intents_to_path()


def diff_intents(layout_key):
    """
    Находит диффы между интентами используемыми в дереве, в хинтах и ханипотах.
    :param str layout_key:
    :rtype: dict[(str, str), set[str]]
    :return: {("есть в...", "нет в..."): {набор интентов}}
        В вывод попадают только непустые диффы.
        Смотреть глазами проще через pprint.
    """
    l_conf = LayoutConf(layout_key)
    sets = {
        'tree': set(map_layout_intents(layout_key)),
        'hp': l_conf.intents_in_gs('honeypots.tsv'),
        'hints': l_conf.intents_in_gs('hints.tsv'),
    }
    diffs = {}
    for f1, set1 in sets.iteritems():
        for f2, set2 in sets.iteritems():
            if f1 == f2:
                continue

            diff1 = set1 - set2
            if diff1:
                diffs[(f1, f2)] = diff1

            diff2 = set2 - set1
            if diff2:
                diffs[(f2, f1)] = diff2

    return diffs


def size_of(prj_key):
    """
    Получить статистику по размерам смёрдженного дерева для проекта prj_key
    :param prj_key:
    :return:
    """
    return ProjectConf(prj_key).get_merged_tree().get_size()


#from pprint import pprint
#pprint(diff_intents('market'))

#print_merged_tree('general')

#print size_of('general')
