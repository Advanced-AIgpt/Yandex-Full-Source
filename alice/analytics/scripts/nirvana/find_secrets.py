#!/usr/bin/env python
# encoding: utf-8
"""
Находит все используемые в графе секреты.
Глобальные показывает все, в блоках - только те, что не привязаны к глобальным.
К сожалению, не умеет забираться внутрь композитных операций, т.к. api Нирваны пока таких ручек не предоставляет.
Поэтому, если встречаются композитные операции, ссылки на их графы выводятся с пометкой 'COMPOSITE'
"""
from pprint import pprint
from collections import defaultdict, deque

from utils.nirvana.api import post_request


def find_secrets(workflow_id, instance_id):
    """
    Найти все используемые в графе секреты
    :param str workflow_id:
    :param str instance_id:
    :return:
    """
    found = defaultdict(set)
    workflow_key = {'workflowId': workflow_id, 'workflowInstanceId': instance_id}

    # Глобальные параметры
    meta = post_request('getGlobalParametersMetaData', workflow_key)['result']
    global_secrets = set(m['parameter'] for m in meta if m['type'] == 'secret')
    if global_secrets:
        url = 'https://nirvana.yandex-team.ru/flow/{workflowId}/{workflowInstanceId}/options'.format(**workflow_key)
        found[url] = {p['parameter']: p['value']
                      for p in post_request('getGlobalParameters', workflow_key)['result']
                      if p['parameter'] in global_secrets}

    # Параметры блоков
    blocks_meta = post_request('getBlockMetaData', workflow_key)['result']
    block_secrets = {b['blockGuid']: set(m['parameter'] for m in b['parameters'] if m['type'] == 'secret')
                     for b in blocks_meta}
    #print block_secrets

    blocks_params = post_request('getBlockParameters', workflow_key)['result']

    for b in blocks_params:
        guid = b['blockGuid']
        secret_names = block_secrets[guid]
        if not secret_names:
            continue

        params = {p['parameter']: p['value']
                  for p in b['parameters']
                  if p['parameter'] in secret_names and not p.get('inheritedFrom')}

        if params:
            url = 'https://nirvana.yandex-team.ru/flow/{workflowId}/{workflowInstanceId}/graph/FlowchartBlockOperation/{blockGuid}'.format(
                blockGuid=guid,
                **workflow_key
            )
            found[url] = params

    # Параметры вложенных операций
    ops_seen = set(b['operationId'] for b in blocks_meta)  # Здесь должна была быть рекурсивная обработка, но апи не умеет в композитные операции
    ops_to_check = deque(ops_seen)
    for op_id in ops_to_check:
        op_meta = post_request('getOperation', {'operationId': op_id})['result']
        if op_meta['composite']:
            url = 'https://nirvana.yandex-team.ru/operation/{}/subprocess'.format(op_id)
            found[url] = 'COMPOSITE'

    return dict(found)


if __name__ == '__main__':
    # Опции ком.строки наверное когда-нибудь воспоследуют.
    # Пока можно просто вписывать параметры сюда:
    pprint(find_secrets('f6225a2f-25e6-4f01-91a1-d9f61c5ddff3', '51546890-fd36-468a-97dd-ae121bf25b33'))
