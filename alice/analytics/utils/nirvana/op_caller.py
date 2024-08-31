#!/usr/bin/env python
# encoding: utf-8

import os
import sys
import json

from nirvana.job_context import context, JOB_CONTEXT_JSON

from utils.json_utils import json_dumps


def call_as_operation(func, input_spec=None):
    if os.path.exists(JOB_CONTEXT_JSON):
        call_as_nirvana_operation(func, input_spec)
    else:
        call_as_local_code(func)


def call_as_local_code(func):
    try:
        import fire
    except:
        # cannot run script outside nirvana or without https://github.com/google/python-fire
        pass
    else:
        fire.Fire(func)


def call_as_nirvana_operation(func, input_spec=None):
    """
    Читает json из параметра кубика `kwargs` и отдаёт его на вход функции, отдельными аргументами
    Выход функции сериализует в json и пишет в выход операции
    :param func: Функция, с произвольными аргументами, возвращающая любую структуру сериализуемую в json
        Отсутствие возврата, то есть, None, тоже вполне подойдёт
    :param dict[basestring, dict[basestring, any]] input_spec: Спецификация данных, которые можно подать на вход кубика
        {"название_аргумента_в_функции": {"опция_для_входа": значение_опции}}
        Все данные подаются на единственный вход "input", который принимает произвольное количество связей.
        Доступные опции:
            "link_name": Если на вход могут быть поданы данные с разным смыслом,
                 то на соединениях лучше поставить link_name
                 и каждый link_name будет попадать в свой аргумент
            "required": Обязательно ли должна быть хотя бы одна связь для этого аргумента.
                 Если не обязательна, в функции должен быть проставлен дефолт.
                 Если аргумент списковый, то в любом случае в него будет передан список,
                     а опция `required` будет определять, обязан ли список быть непустым
                 По-умолчанию = True.
            "as_list": Допустимы ли множественные входы с указанным link_name.
                 Если True, в аргумент будет передан список из значений.
                 По-умолчанию = False.
            "parser": Способ парсинга значения.
                Если не задан, равен None, или 'path', в функцию будет передан путь к файлу, содержащему данные.
                Если равен "json", файл будет прочитан как json и в функцию будет передан получившийся объект.
                Если равен "text", файл будет загружен в память и передан в функцию как строка
                Так же, можно указать свою функцию парсинга, которая принимает на вход открытый файл.
        Примеры спецификаций:
            `{"data": {}}`
                Минимальная спецификация для передачи единственного (обязательного) входа.
                Вход будет передан аргументом `data` как путь к файлу.
            `{"data": {"as_list": True},
              "settings": {"required": False, "parser": "json", "link_name": "settings"}}`
                данные передаются набором файлов и опционально могут быть переданы настройки в формате json.
    :return: Ничего не возвращает. Только пишет результат в файл для выхода "retvalue".
    """

    ctx = context()
    kwargs = json.loads(ctx.get_parameters()['kwargs'])
    if input_spec:
        from op_input import parse_inputs
        kwargs.update(parse_inputs(ctx.get_inputs().get_item_list('input'), input_spec))

    ret = func(**kwargs)

    write_mode = 'w'
    if sys.version_info.major == 3 and ret is not None and isinstance(ret, (bytes, bytearray)):
        write_mode += 'b'

    outputs = ctx.get_outputs()
    if outputs.has('retvalue'):
        with open(outputs.get('retvalue'), write_mode) as out:
            out.write(json_dumps(ret))
    else:
        # Предполагается, что в этом случае в ret лежит dict и каждый ключ надо разложить в отдельный output
        for name, val in ret.iteritems():
            with open(outputs.get(name), write_mode) as out:
                out.write(json_dumps(val))


def get_current_workflow_url():
    """
    Получить ссылку на граф, в котором запущен скрипт
    Если скрипт запущен вне нирваны, вернёт None
    :rtype: str|None
    """
    try:
        ctx = context()
    except IOError:  # [Errno 2] No such file or directory: './job_context.json'
        return None
    else:
        return ctx.get_meta().get_workflow_url()


def get_current_workflow_title():
    """
    Получить название графа, в котором запущен скрипт
    Если скрипт запущен вне нирваны, вернёт None
    :rtype: str|None
    """
    try:
        ctx = context()
    except IOError:  # [Errno 2] No such file or directory: './job_context.json'
        return None
    else:
        return ctx.get_meta().get_description()
