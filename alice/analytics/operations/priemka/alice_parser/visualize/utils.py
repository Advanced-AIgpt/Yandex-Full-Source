# coding: utf-8

import os
import json
import gettext

from library.python import resource

from alice.analytics.utils.json_utils import get_path_str, get_path


def get_slots_answer(slots):
    try:
        if slots:
            for slot in slots:
                if slot.get('name') == 'answer':
                    return json.loads(get_path_str(slot, 'typed_value.string'))
    except TypeError:
        return None
    return None


def get_slots(state):
    # ToDo: переписать на objects, подробнее в VA-1531
    analytics_info = state.get('analytics_info')
    if analytics_info and analytics_info.get("analytics_info"):
        info = analytics_info.get("analytics_info")
        scenario_info = None
        if 'alice.vins' in info:
            scenario_info = info['alice.vins']
        elif 'Vins' in info:
            scenario_info = info['Vins']
        elif 'Route' in info:
            scenario_info = info['Route']
        if scenario_info and ("semantic_frame" in scenario_info) and ("slots" in scenario_info["semantic_frame"]):
            return scenario_info["semantic_frame"]["slots"]
    return None


def get_device_state_data(session_state, device_state_name, default=None):
    if device_state_name in session_state:
        return session_state[device_state_name]
    result = get_path(session_state, ['device_state', device_state_name], default)
    """
    get_path(session_state, ['device_state', device_state_name], default) работает плохо, когда {"field": null}
    get_path(session_state, ['device_state', device_state_name], default) нельзя, так как могут быть
    рузультат False, а дефолт True
    """
    if result is None:
        return default
    else:
        return result


def copy_translations_files_to_os():
    LANGUAGES = ('en', 'ar',)

    for lang in LANGUAGES:
        os.makedirs('i18n_translations/{lang}/LC_MESSAGES'.format(lang=lang))

    for resource_path in resource.resfs_files(prefix='alice/analytics/operations/priemka/alice_parser/visualize/i18n'):
        # пути к ресурсам вида alice/analytics/operations/priemka/alice_parser/visualize/i18n/ar/LC_MESSAGES/file.mo
        if '/i18n/' not in resource_path:
            continue
        split_data = resource_path.split('/')
        lang = split_data[-3]
        file_name = split_data[-1]
        file_path = os.path.join('i18n_translations', lang, 'LC_MESSAGES', file_name)
        if not os.path.exists(file_path):
            with open(file_path, 'wb') as file:
                data = resource.resfs_read(resource_path)
                file.write(data)


def load_translations(domain):
    if not os.path.exists('i18n_translations'):
        # при запуске бинаря alice_parser, нужно скопировать переводы (mo файлы)
        # из RESOURCE_FILES на файловую систему в папку i18n_translations
        copy_translations_files_to_os()
    return gettext.translation(domain, localedir='i18n_translations', fallback=True)
