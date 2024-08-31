'''
These mappings are in separate file because they are used in yql script (devices_denominator_yql_query.yql)
 No external dependencies can be in this file
'''
import re

DEVICE2DEVICE = {
    'yandexstation': 'Yandex Station',
    'Station': 'Yandex Station',
    'yandexstation_2': 'Yandex Station_2',
    'yandexmodule': 'Yandex YandexModule-00002',
    'yandexmini': 'Yandex yandexmini',
    'yandexmini_2': 'Yandex yandexmini_2',
    'yandexmicro': 'Yandex yandexmicro',
    'elari_a98': 'Elari elari_a98',
    'wk7y': 'LG wk7y',
    'lightcomm': 'Dexp lightcomm',
    'linkplay_a98': 'Irbis linkplay_a98',
    'jbl_link_portable': 'JBL jbl_link_portable',
    'jbl_link_music': 'JBL jbl_link_music',
    'prestigio_smart_mate': 'Prestigio prestigio_smart_mate'
}

DEVICE2APP = {
    'elariwatch': 'elariwatch',
    'yandexstation': 'quasar',
    'Station': 'quasar',
    'yandexstation_2': 'quasar',
    'yandexmodule': 'quasar',
    'yandexmini': 'small_smart_speakers',
    'yandexmini_2': 'small_smart_speakers',
    'yandexmicro': 'small_smart_speakers',
    'elari_a98': 'small_smart_speakers',
    'wk7y': 'small_smart_speakers',
    'lightcomm': 'small_smart_speakers',
    'linkplay_a98': 'small_smart_speakers',
    'jbl_link_portable': 'small_smart_speakers',
    'jbl_link_music': 'small_smart_speakers',
    'prestigio_smart_mate': 'small_smart_speakers',
    'yandex_midi': 'small_smart_speakers'
}

VALID_DEVICES = set(DEVICE2DEVICE.values({'cvte', 'cv', 'hikeen', 'SmartTV', 'gntch'}))
APPS_WITH_DEVICE_FILTER = set(DEVICE2APP.values())


def map_device_to_app(device):
    if device is None:
        return None
    elif device in DEVICE2APP:
        return DEVICE2APP.get(device)
    device = re.sub('_subscription', '', device)
    if device in DEVICE2APP:
        return DEVICE2APP.get(device)
    if re.findall("yandex_tv", device):
        return "tv"


def map_device_appmetic_to_device_expboxes(device_appmetric):
    if device_appmetric is None:
        return None
    elif device_appmetric in DEVICE2DEVICE:
        return DEVICE2DEVICE.get(device_appmetric)
    device_appmetric = re.sub('_subscription', '', device_appmetric)
    if device_appmetric in DEVICE2DEVICE:
        return DEVICE2DEVICE.get(device_appmetric)
    if re.findall("yandex_tv", device_appmetric):
        return device_appmetric.split("_")[-1]


def filter_device_models_in_apps(device, app):
    if app in APPS_WITH_DEVICE_FILTER and device not in VALID_DEVICES:
        return False
    return True
