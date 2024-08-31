# coding: utf-8

import json


def get_file_content(filename):
    """
    Считываем содержимое файла `filename`
    :param str filename:
    :return str:
    """
    with open(filename) as f:
        return f.read().replace('\n', '')


def get_table_from_nirvana_input(filename):
    """
    Возвращает имя таблицы из ресурса MR-table в нирване из файла `filename`
    :param str filename:
    :return str:
    """
    with open(filename) as f:
        return json.load(f)['table']


def save_table_to_nirvana_output(filename, cluster, table):
    """
    Сохраняет YT таблицу в MR-table для нирваны
    :param str filename:
    :param str cluster:
    :param str table:
    :return:
    """
    with open(filename, 'w') as f:
        json.dump({'cluster': cluster, 'table': table}, f, indent=4)


def save_content_to_file(filename, data):
    """
    Сохраняет данные `data` в файл filename
    :param str cluster:
    :param str data:
    :return:
    """
    with open(filename, 'w') as f:
        f.write(data)
