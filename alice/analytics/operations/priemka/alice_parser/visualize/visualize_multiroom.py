# coding: utf-8

from alice.analytics.utils.json_utils import get_path


import sys
reload(sys)  # noqa
sys.setdefaultencoding('utf8')  # noqa

from .utils import load_translations
t = load_translations('visualize_multiroom')
t.install()


def _(text):
    """
    HACK: Обёртка над gettext для python2 строк
    вместо: `_ = t.gettext`
    :param str text:
    """
    return t.gettext(text.decode('utf-8'))


def get_room_by_room_id(room_id, iot_config):
    if iot_config:
        for room_info in iot_config.get('rooms', []):
            if room_info.get('id') == room_id:
                return room_info.get('name')
    return None


def get_device_by_device_id(device_id, iot_config):
    if iot_config:
        for device_info in iot_config.get('devices', []):
            device_info_quasar_id = get_path(device_info, ['quasar_info', 'device_id'])
            if device_info_quasar_id and device_info_quasar_id == device_id:
                return device_info.get('name')
    return None


def get_room_by_device_id(device_id, iot_config):
    if iot_config:
        for device_info in iot_config.get('devices', []):
            device_info_quasar_id = get_path(device_info, ['quasar_info', 'device_id'])
            if device_info_quasar_id and device_info_quasar_id == device_id:
                return get_room_by_room_id(device_info.get('room_id'), iot_config)
    return None


def get_multiroom_devices(device_ids, room_id, iot_config):
    if device_ids:
        if room_id == '__all__':
            devices_human_readable = [x for x in [(get_device_by_device_id(device_id, iot_config),
                                                   get_room_by_device_id(device_id, iot_config))
                                                  for device_id in device_ids] if x[0] is not None and x[1] is not None]
            if devices_human_readable:
                res = ''
                for device, room in devices_human_readable:
                    res += device + _(' в комнате ') + room + ', '
                return res[:-2]
            return None
        else:
            room_human_readable = get_room_by_room_id(room_id, iot_config)
            devices_human_readable = [_f for _f in [get_device_by_device_id(device_id, iot_config) for device_id in device_ids] if _f]
            if devices_human_readable:
                res = ', '.join(devices_human_readable)
                if room_human_readable:
                    res += _(' в комнате ') + room_human_readable
                return res
    return None
