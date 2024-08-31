#!/usr/bin/env python
# encoding: utf-8

import sys
import json


def json_dumps(obj):
    """
    Сериализует в json читабельный для человека.
    В том числе избавляется от экранирования кириллицы в пользу кодировки utf-8.
    :param any obj:
    :rtype: str
    """
    # json.dumps возвращает нативный строковый литерал
    # во втором питоне - это байтовая строка, а в третьем - текстовая строка
    if sys.version_info.major == 3:
        return json.dumps(obj, indent=2, ensure_ascii=False, sort_keys=True, separators=(',', ': '))
    else:
        return json.dumps(obj, indent=2, sort_keys=True, separators=(',', ': ')) \
            .decode('raw_unicode_escape').encode('utf-8')


def _get_element_by_path(*path_elements, **kwargs):
    """ Safe
    :param path_elements: strings and ints that are keys and indices of the corresponding level.
    :param default:
    :param data: python dict
    :return: the node at the given path, default otherwise.
    """

    default = kwargs.get('default')
    data = kwargs.get('data', {})

    node = data
    for pe in path_elements:
        if node is None:
            break
        elif isinstance(node, dict):
            if pe in node:
                node = node[pe]
            else:
                node = default
                break
        elif isinstance(node, list):
            if isinstance(pe, str) and pe.isdigit():
                pe = int(pe)
            if isinstance(pe, int):
                if pe + 1 <= len(node):
                    node = node[pe]
                else:
                    break
            else:
                break
        else:
            break
    else:
        return node
    return default


def get_path_str(data, path, default=None):
    """
    Shorthand for self._get_element_by_path
    :param data: python dict
    :param path: string path to object in json tree, segments separated with "." (dot)
    :param default:
    :return: the node at the given path, default otherwise.
    """
    path_elements = [int(x) if x.lstrip("-").isdigit() else x
                     for x in path.split(".")]
    return _get_element_by_path(*path_elements, default=default, data=data)


def get_path(data, path, default=None):
    """
    Shorthand for self._get_element_by_path
    :param data: python dict
    :param path: path (array of keys) to object in json tree
    :param default:
    :return: the node at the given path, default otherwise.
    """
    return _get_element_by_path(*path, default=default, data=data)
