#!/usr/bin/env python
# encoding: utf-8
import json

import requests

from utils.auth import make_oauth_header, CERT_PATH


# Обёртки к https://wiki.yandex-team.ru/JandeksPoisk/Nirvana/Components/API/

API_ROOT = 'https://nirvana.yandex-team.ru/api/public/v1/'


def _make_headers(token):
    return make_oauth_header(token, 'NIRVANA_TOKEN', '~/.nirvana/token')


def post_request(api_method, params, api_root=API_ROOT, token=None):
    """
    Общая функция для вызова методов api Нирваны
    :param str api_method:
    :param dict[str, Any] params:
    :param str api_root:
    :param str|None token:
    """
    data = {"jsonrpc": "2.0",
            "method": api_method,
            "id": "required_id",
            "params": params}
    url = '{}{}'.format(api_root, api_method)
    req = requests.post(url, headers=_make_headers(token), json=data, verify=CERT_PATH)
    if req.status_code != 200:
        raise UserWarning('Response status: %s\n%s' % (req.status_code, req.content))

    response = req.json()
    if 'error' in response:
        raise UserWarning('Request error: %s' % response)
    else:
        return response

# Блоки


def iter_block_results(instance_id, block_local_id, output_names):
    """
    Получение результатов выполненного блока
    :param str instance_id: Идентификатор завершившегося графа
    :param str block_local_id: Локальный идентификатор блока внутри графа
    :param list[str]|None output_names: Названия выходов, которые нужно получить
        Если установлен в None, будут получены все
    :rtype: Iterable(tuple(str, str))
    :return: Итератор из названий выходов и содержимого в нераспаршенном виде
    """
    blocks = post_request('getBlockResults',
                          {"workflowInstanceId": instance_id,
                           "blocks": [{"code": block_local_id}],
                           "outputs": output_names,
                           })
    try:
        block = blocks['result'][0]
    except IndexError:
        msg = 'Block "%s" not found in "%s"'  # TODO: отдавать ссылку на граф
        raise UserWarning(msg % (block_local_id, instance_id))
    pending_outputs = set(output_names or ())

    for result in block['results']:
        name = result['endpoint']
        if output_names is None or name in pending_outputs:
            response = requests.get(result['storagePath'], verify=CERT_PATH)
            yield name, response.content
            pending_outputs.discard(name)

    if pending_outputs:
        msg = 'Outputs %s not found in block %s, instance %s'  # TODO: отдавать ссылку на граф
        raise UserWarning(msg % (pending_outputs, block_local_id, instance_id))


def get_block_result(instance_id, block_local_id, output_name, parse=json.loads):
    """
    Получение результата из выхода выполненного блока
    :param str instance_id: Идентификатор завершившегося графа
    :param str block_local_id: Локальный идентификатор блока внутри графа
    :param str output_name: Название выхода
    :param Callable(str)|None parse: Функция, которая может десериализовать данные, хранящиеся в выходе
       Может быть задана в None - тогда результат будет в виде строки, как есть
       По-умолчанию, результат парсится как json
    :return:
    """
    for _name, value in iter_block_results(instance_id,
                                           block_local_id,
                                           [output_name]):
        if parse is None:
            return value
        else:
            return parse(value)
    #  Если мы здесь, значит итератор оказался пустым
    msg = 'Block "%s" not found in instance "%s"'  # TODO: отдавать ссылку на граф
    raise UserWarning(msg % (block_local_id, instance_id))


# Инстансы графов

def get_global_params(instance_id):
    """
    Получение глобальных параметров инстанса
    :param str instance_id:
    :rtype: dict[str, Any]
    """
    result = post_request('getGlobalParameters',
                          {'workflowInstanceId': instance_id})['result']
    return {p['parameter']: p.get('value') for p in result}


def get_instance_meta(instance_id):
    """
    Получение мета-информации инстанса
    :param str instance_id:
    :rtype: dict[str, Any]
    """
    return post_request('getWorkflowMetaData',
                          {'workflowInstanceId': instance_id})['result']


def last_instances(workflow_id, max_count=10, finished_only=True):
    """
    Получить последние (по дате создания) инстансы в указанном воркфлоу
    :param str workflow_id: Идентификатор воркфлоу, в котором находятся инстансы
    :param int max_count: Количество инстансов, которое нужно вернуть.
    :param bool finished_only: Отдавать только успешно завершённые инстансы
    :return:
    """
    # TODO: В Nirvana api есть ограничение - не больше 300 инстансов.
    #   Если понадобится больше, можно сделать обёртку вокруг пагинации.
    params = {
        'pattern': workflow_id,
        'sortingRules': [{'isDescending': True, 'sortingField': 'created'}],
        "paginationData": {"pageSize": max_count, "pageNumber": 1},
    }
    if finished_only:
        params['additionalFilters'] = {'status': ['completed'], 'result': ['success']}
    response = post_request('findWorkflows', params)
    return response['result']
