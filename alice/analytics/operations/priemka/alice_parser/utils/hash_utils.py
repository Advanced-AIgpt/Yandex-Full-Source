import json
import copy
import hashlib


def get_hash_for_json(item):
    item_json = json.dumps(item, sort_keys=True, indent=2)
    return hashlib.md5(item_json.encode("utf-8")).hexdigest()


def get_hashable_data(record):
    """
    Возвращает словарь с значащей/существенной/важной информацией, по которому считается hashsum для сессии `record`
    Из объекта запроса выбираются ключевые поля для каждого запроса:
        * state0, state1, ..., state10      - визуализированное состояние устройства. Данные state'а берутся из корзинки
        * action0, action1, ..., action10   - визуализация ответа Алисы. Содержит query, scenario, answer, action, url
        * hashsum0, ..., hashsum10 - заранее посчитанные хэшсамы, которые используется вместо action.url для ПП general
        * hashable0, ... hashable10 - объект, который используется вместо actionX/hashsumX для вычисления хэшсама
    :param dict|Record record:
    :return dict:
    """
    hash_dict = {}
    for key, value in list(record.items()):
        if key.startswith('state'):
            state_copy = copy.deepcopy(value)
            if state_copy and state_copy.get('time'):
                del state_copy['time']
            hash_dict[key] = state_copy
        elif key.startswith('action'):
            action_copy = copy.deepcopy(value)
            if action_copy and action_copy.get('url'):
                del action_copy['url']
            if action_copy:
                hashsum_with_ind = 'hashsum' + key[6:]
                if record.get(hashsum_with_ind):
                    action_copy[hashsum_with_ind] = record.get(hashsum_with_ind)

                hashable_field_with_ind = 'hashable' + key[6:]
                if record.get(hashable_field_with_ind):
                    action_copy = record.get(hashable_field_with_ind)
            hash_dict[key] = action_copy
    return hash_dict


def get_hash(record):
    """Вычисляет хэш для запроса `record`"""
    hash_dict = get_hashable_data(record)
    return get_hash_for_json(hash_dict)
