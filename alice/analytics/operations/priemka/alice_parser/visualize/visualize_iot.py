# coding: utf-8

from builtins import next
from builtins import str
from builtins import object
import json
import copy
from alice.analytics.utils.json_utils import get_path_str

from .visualize_alarms_timers import get_time_offset, _get_tz_from_record

import pytz
from dateutil.parser import isoparse
from datetime import timedelta


import sys
reload(sys)  # noqa
sys.setdefaultencoding('utf8')  # noqa

from .utils import load_translations
t = load_translations('visualize_iot')
t.install()


def _(text):
    """
    HACK: Обёртка над gettext для python2 строк
    вместо: `_ = t.gettext`
    :param str text:
    """
    return t.gettext(text.decode('utf-8'))


IOT_CONFIG_SCENATIOS = {
    'iot_do',
    'call'
}

IOT_DEVICES_EXCEPTIONS = {
    'devices.types.hub',  # Пульт
    'devices.types.media_device.dongle.yandex.module'  # Модуль
}

SMART_SPEAKER_DEVICES_PREFIX = 'devices.types.smart_speaker'
TV_DEVICE_TYPE = 'devices.types.media_device.tv'
QUASAR_SKILL_IDS = {'Q', 'QUALITY'}

IOT_VALUE_SET = {
    'value',
    'toggle_capability_value',
    'custom_button_capability_value',
    'color_setting_capability_value',
    'range_capability_value',
    'mode_capability_value',
    'quasar_server_action_capability_value',
    'on_of_capability_value'
}


IOT_DEVICE_TYPES = {
    'devices.types.light': _('Осветительный прибор'),
    'devices.types.socket': _('Розетка'),
    'devices.types.media_device.tv': _('Телевизор'),
    'devices.types.switch': _('Выключатель или переключатель'),
    'devices.types.other': _('Другое IOT-устройство'),
    'devices.types.openable': _('Другое IOT-устройство'),
    'devices.types.thermostat': _('Термостат'),
    'devices.types.media_device': _('Медиа устройство'),
    'devices.types.hub': _('Пульт для управления другими устройствами'),
    'devices.types.thermostat.ac': _('Кондиционер'),
    'devices.types.cooking.kettle': _('Чайник'),
    'devices.types.cooking': _('Кухонная техника'),
    'devices.types.vacuum_cleaner': _('Пылесос'),
    'devices.types.openable.curtain': _('Занавеска'),
    'devices.types.cooking.coffee_maker': _('Кофемашина'),
    'devices.types.purifier': _('Очиститель воздуха'),
    'devices.types.humidifier': _('Увлажнитель воздуха'),
    'devices.types.smart_speaker.yandex.station': _('Яндекс.Станция'),
    'devices.types.smart_speaker.yandex.station.mini': _('Яндекс.Станция Мини'),
    'devices.types.smart_speaker.irbis.a': _('Колонка Irbis'),
    'devices.types.smart_speaker.dexp.smartbox': _('Колонка Dexp'),
    'devices.types.smart_speaker.elari.smartbeat': _('Колонка Elari'),
    'devices.types.media_device.dongle.yandex.module': _('Яндекс.Модуль'),
    'devices.types.smart_speaker.lg.xboom_wk7y': _('Колонка LG'),
    'devices.types.smart_speaker.prestigio.smartmate': _('Колонка Prestigio Smartmate'),
}


def _try_parse_value_string(value):
    if value in ['True', 'true', True]:
        return True
    elif value in ['False', 'false', False]:
        return False
    try:
        return int(value)
    except (ValueError, TypeError):
        return value


class UserConfigDicts(object):
    def __init__(self, devices_dict, scenarios_dict, colors_dict, scenes_dict):
        self.devices_dict = devices_dict
        self.scenarios_dict = scenarios_dict
        self.colors_dict = colors_dict
        self.scenes_dict = scenes_dict


class ActionInfo(object):
    def __init__(self, devices, user_config_dicts, capability_type, capability_instance, value, unit, relativity_type,
                 time_info, client_tz):
        self.devices = devices
        self.user_config_dicts = user_config_dicts
        self.capability_type = capability_type
        self.capability_instance = capability_instance
        self.value = value
        self.unit = unit
        self.relativity_type = relativity_type
        self.time_info = time_info
        self.client_tz = client_tz


def _get_iot_action_description(action_info):
    devices = action_info.devices
    capability_type = action_info.capability_type
    capability_instance = action_info.capability_instance
    value = action_info.value
    unit = action_info.unit
    relativity_type = action_info.relativity_type
    time_info = action_info.time_info
    client_tz = action_info.client_tz

    devices_dict = action_info.user_config_dicts.devices_dict
    colors_dict = action_info.user_config_dicts.colors_dict
    scenes_dict = action_info.user_config_dicts.scenes_dict
    many_devices = len(devices) > 1
    devices_string = ', '.join([devices_dict[device]['device_name'] for device in devices])

    action_string = ''

    if capability_type == 'devices.capabilities.on_off':
        assert capability_instance == 'on', 'Unknown case in IOT action processing, please update code\n' \
                                            + json.dumps(action_info, default=lambda o: o.__dict__)
        if relativity_type == 'invert':
            if many_devices:
                action_string = _('Включаются/выключаются ') + devices_string
            else:
                action_string = _('Включается/выключается ') + devices_string
        if value is not None:
            if value:
                if many_devices:
                    action_string = _('Включаются ') + devices_string
                else:
                    action_string = _('Включается ') + devices_string
            else:
                if many_devices:
                    action_string = _('Выключаются ') + devices_string
                else:
                    action_string = _('Выключается ') + devices_string
    elif capability_type == 'devices.capabilities.color_setting' and capability_instance == 'color':
        color = value
        action_string = devices_string
        if many_devices:
            action_string += _(' меняют цвет на ')
        else:
            action_string += _(' меняет цвет на ')
        action_string += colors_dict[color]
    elif capability_type == 'devices.capabilities.color_setting' and capability_instance == 'color_scene':
        scene = value
        action_string = devices_string
        if many_devices:
            action_string += _(' меняют цветовую сцену на ')
        else:
            action_string += _(' меняет цветовую сцену на ')
        action_string += scenes_dict[scene]
    elif capability_type in ('devices.capabilities.range', 'devices.capabilities.color_setting'):
        action_string = ''
        if capability_instance == 'channel':
            if many_devices:
                action_string += _('Канал на устройствах: ')
            else:
                action_string += _('Канал на устройстве ')
        elif capability_instance == 'temperature':
            if many_devices:
                action_string += _('Настройка температуры устройств: ')
            else:
                action_string += _('Настройка температуры устройства ')
        elif capability_instance == 'temperature_k':
            if many_devices:
                action_string += _('Численое значение цветовой температуры устройств: ')
            else:
                action_string += _('Численое значение цветовой температуры устройства ')
        elif capability_instance == 'brightness':
            if many_devices:
                action_string += _('Яркость устройств: ')
            else:
                action_string += _('Яркость устройства ')
        elif capability_instance == 'volume':
            if many_devices:
                action_string += _('Громкость звука на устройствах: ')
            else:
                action_string += _('Громкость звука на устройстве ')
        elif capability_instance == 'open':
            if many_devices:
                action_string += _('Открытие устройств: ')
            else:
                action_string += _('Открытие устройства ')
        else:
            raise ValueError('Unknown case in IOT action processing, please update code\n'
                             + json.dumps(action_info, default=lambda o: o.__dict__))
        action_string += devices_string
        if many_devices:
            action_string += ' — '
        else:
            action_string += ' '
        if relativity_type is not None:
            if relativity_type == 'increase':
                action_string += _('увеличивается')
                if capability_instance == 'temperature_k':
                    action_string += _(' (свет становится более \'холодным\')')
            if relativity_type == 'decrease':
                action_string += _('уменьшается')
                if capability_instance == 'temperature_k':
                    action_string += _(' (свет становится более \'теплым\')')
            if value is not None:
                if unit is None:
                    if isinstance(value, int):
                        action_string += _(' на {}').format(value)
                else:
                    assert unit in ('unit.percent', 'unit.temperature.celsius'), \
                        'Unknown case in IOT action processing, please update code\n' \
                        + json.dumps(action_info, default=lambda o: o.__dict__)
                    if unit == 'unit.percent':
                        action_string += ' на %d%%' % int(value)
                    if unit == 'unit.temperature.celsius':
                        action_string += ' на %d°C' % int(value)

        else:
            action_string += _('устанавливается на ')
            if value == 'max':
                action_string += _('максимум')
                if capability_instance == 'temperature_k':
                    action_string += _(' (свет становится \'холодным\')')
            elif value == 'min':
                action_string += _('минимум')
                if capability_instance == 'temperature_k':
                    action_string += _(' (свет становится \'теплым\')')
            else:
                if value is None:
                    return _('Алиса сообщает состояние устройства')
                try:
                    int(value)
                except Exception:
                    raise TypeError('Unknown case in IOT action processing, please update code\n'
                                    + json.dumps(action_info, default=lambda o: o.__dict__))

                action_string += str(value)
                if unit is None:
                    pass
                elif unit == 'unit.temperature.celsius':
                    action_string += '°C'
                elif unit == 'unit.percent':
                    action_string += '%'
                else:
                    raise ValueError('Unknown measure unit, please update code\n'
                                     + json.dumps(action_info, default=lambda o: o.__dict__))

    elif capability_type == 'devices.capabilities.mode':
        if capability_instance == 'fan_speed':
            action_string = _('Скорость вентилятора на устройств{}{} ').format(
                'ах: ' if many_devices else 'е ', devices_string)
            if relativity_type == 'increase':
                action_string += _('увеличивается')
            else:
                action_string += _('устанавливается на ')
                if value == 'high':
                    action_string += _('высокий')
                elif value == 'low':
                    action_string += _('низкий')
                elif value == 'medium':
                    action_string += _('средний')
                elif value == 'auto':
                    action_string += _('выбираемый автоматически')
                else:
                    raise ValueError('Unknown fan speed value unit, please update code\n'
                                     + json.dumps(action_info, default=lambda o: o.__dict__))
                action_string += _(' уровень')

        elif capability_instance == 'thermostat':
            action_string = _('Режим работы устройств{}{} ').format(
                ': ' if many_devices else 'а ', devices_string)
            action_string += devices_string + _(' устанавливается на ')
            if value == 'cool':
                action_string += _('охлаждение')
            elif value == 'heat':
                action_string += _('нагрев')
            elif value == 'auto':
                action_string += _('\'автоматический\'')
            elif value == 'fan_only':
                action_string += _('\'только вентилятор\'')
            elif value == 'dry':
                action_string += _('сушка')
            else:
                raise ValueError('Unknown thermostat value unit, please update code\n'
                                 + json.dumps(action_info, default=lambda o: o.__dict__))

        elif capability_instance == 'input_source':
            action_string = _('Изменяется источник сигнала')

        elif capability_instance == 'work_speed':
            action_string = _('Скорость работы устройств{}{} ').format(
                ': ' if many_devices else 'а ', devices_string)
            if relativity_type == 'increase':
                action_string += _('увеличивается')
            else:
                action_string += _('устанавливается на ')
                if value == 'high':
                    action_string += _('высокий')
                elif value == 'low':
                    action_string += _('низкий')
                elif value == 'medium':
                    action_string += _('средний')
                elif value == 'quiet':
                    action_string += _('тихий')
                elif value == 'normal':
                    action_string += _('нормальный')
                elif value == 'fast':
                    action_string += _('быстрый')
                elif value == 'auto':
                    action_string += _('выбираемый автоматически')
                else:
                    raise ValueError('Unknown work speed value unit, please update code\n'
                                     + json.dumps(action_info, default=lambda o: o.__dict__))
                action_string += _(' уровень')

        else:
            raise ValueError('Unknown case in IOT action processing, please update code\n'
                             + json.dumps(action_info, default=lambda o: o.__dict__))
    elif capability_type == 'devices.capabilities.toggle':
        if value:
            action_string = _('Включается ')
        else:
            action_string = _('Выключается ')

        if capability_instance == 'mute':
            action_string += _('беззвучный режим')
        elif capability_instance == 'ionization':
            action_string += _('ионизация')
        elif capability_instance == 'backlight':
            action_string += _('задняя подсветка')
        elif capability_instance == 'controls_locked':
            action_string += _('возможность управления')
        elif capability_instance == 'oscillation':
            action_string += _('колебания')
        elif capability_instance == 'keep_warm':
            action_string += _('поддержание температуры')
        elif capability_instance == 'pause':
            action_string += _('пауза')
        else:
            raise ValueError('Unknown case in IOT action processing, please update code\n'
                             + json.dumps(action_info, default=lambda o: o.__dict__))

        action_string += _(' на устройств')
        if many_devices:
            action_string += 'ах: '
        else:
            action_string += 'е '
        action_string += devices_string
    elif capability_type == 'devices.capabilities.custom.button':
        capability_dict = [c for c in devices_dict[devices[0]]['device_capabilities']
                           if c['custom_button_capability_parameters']['instance'] == capability_instance][0]
        button_name = capability_dict['custom_button_capability_parameters']['instance_names'][0]

        if value is not None:
            action_string = _('{verb} пользовательская кнопка "{button_name}" устройств{ending} {devices_string}') \
                .format(verb=_('Включается') if value else _('Выключается'),
                        ending='' if many_devices else 'a',
                        button_name=button_name,
                        devices_string=devices_string)
        else:
            raise ValueError('Unknown case in IOT action processing, please update code\n'
                             + json.dumps(action_info, default=lambda o: o.__dict__))
    else:
        raise ValueError('Unknown action type in IOT action processing, please update code\n'
                         + 'cap_type:<{}>, cap_instance:<{}>\n'.format(capability_type, capability_instance)
                         + json.dumps(action_info, default=lambda o: o.__dict__))

    """при наличии ненулевого поля `time_info` указываем время отложенного исполнения."""
    if time_info:
        delayed_time = time_info.get('value') or time_info.get('time_point') or time_info.get('time_interval') or dict()
        delayed_time = delayed_time.get('time')
        if delayed_time:
            delayed_ts = isoparse(delayed_time).astimezone(pytz.utc).replace(tzinfo=None) \
                + timedelta(seconds=get_time_offset(client_tz))
            return _('Команда "{}" , будет исполнена {} в {}').format(
                action_string, delayed_ts.strftime('%Y-%m-%d'), delayed_ts.strftime('%H:%M:%S'))

    return action_string


def get_household_by_household_id(household_id, iot_config):
    if iot_config:
        for household_info in iot_config.get('households', []):
            if household_info.get('id') == household_id:
                return household_info.get('name')
    return None


def _get_iot_query_reaction_description():
    return _('Алиса сообщает состояние устройства')


def _get_iot_scenario_reaction_description(reaction, user_config_dicts):
    # сознательно не обрабатываем исключение: если в реакции с типом "scenario" нет поля "scenarios", что-то не так
    scenarios = reaction['scenario_parameters']['scenarios']

    if len(scenarios) > 1:
        scenario_names = [("'%s'" % user_config_dicts.scenarios_dict[scenario]['scenario_name']) for scenario in scenarios]
        return _('Запускаются сценарии умного дома: {}').format(', '.join(scenario_names))
    elif len(scenarios) == 1:
        return _('Запускается сценарий умного дома "{}"').format(user_config_dicts.scenarios_dict[scenarios[0]]['scenario_name'])
    else:
        raise ValueError('Empty scenarios array in scenario reaction\n' + json.dumps(reaction))


def _get_iot_reaction_description(
    reaction,
    user_config_dicts,
    client_tz,
):
    try:
        reaction_type = reaction['type']
        if reaction_type == 'action':
            # в новом формате value – всегда строка. Приводим к int или bool, если это возможно
            value = _try_parse_value_string(reaction['action_parameters'].get('capability_value'))

            action_info = ActionInfo(
                devices=reaction['action_parameters']['devices'],
                user_config_dicts=user_config_dicts,
                capability_type=reaction['action_parameters']['capability_type'],
                capability_instance=reaction['action_parameters']['capability_instance'],
                value=value,
                unit=reaction['action_parameters'].get('capability_unit'),
                relativity_type=reaction['action_parameters'].get('relativity_type'),
                time_info=reaction['action_parameters'].get('time_info'),
                client_tz=client_tz,
            )
            return _get_iot_action_description(action_info)
        elif reaction_type == 'query':
            return _get_iot_query_reaction_description()
        elif reaction_type == 'scenario':
            return _get_iot_scenario_reaction_description(reaction, user_config_dicts)
        else:
            raise ValueError('Unknown reaction type in IOT reaction processing, please update code\n' + json.dumps(reaction))
    except Exception as e:
        return 'ERROR: ' + repr(e)


def _get_iot_actions_from_new_analytics_info(record, user_config_dicts):
    objects = get_path_str(record, 'analytics_info.analytics_info.IoT.scenario_analytics_info.objects')
    reactions = next(obj['iot_reactions']['reactions'] for obj in objects if obj['id'] == 'iot_reactions')
    client_tz = _get_tz_from_record(record)

    # получаем текстовое описание для каждой реакции и совмещаем их в одну строку через '\n'
    action_descriptions = list(set([_get_iot_reaction_description(reaction, user_config_dicts, client_tz) for reaction in reactions]))
    return '\n'.join(action_descriptions)


def is_smart_home_user(analytics_info):
    # есть ли у пользователя девайсы Умного Дома
    if analytics_info and analytics_info.get('iot_user_info') and analytics_info['iot_user_info'].get('devices'):
        for device in analytics_info['iot_user_info']['devices']:
            if device.get('analytics_type') and not device['analytics_type'].startswith(
                SMART_SPEAKER_DEVICES_PREFIX) and \
                device['analytics_type'] not in IOT_DEVICES_EXCEPTIONS and not (
                    device['analytics_type'] == TV_DEVICE_TYPE and device.get('skill_id') in QUASAR_SKILL_IDS):
                return True
    elif analytics_info and analytics_info.get('users_info'):
        scenario = list(analytics_info['users_info'].keys())[0]
        if analytics_info['users_info'][scenario].get('scenario_user_info') and \
                analytics_info['users_info'][scenario]['scenario_user_info'].get('properties'):
            for property in analytics_info['users_info'][scenario]['scenario_user_info']['properties']:
                if property.get('iot_profile') and property['iot_profile'].get('devices'):
                    for device in property['iot_profile']['devices']:
                        if not device['type'].startswith(SMART_SPEAKER_DEVICES_PREFIX) and \
                                device['type'] not in IOT_DEVICES_EXCEPTIONS and not (
                    device['type'] == TV_DEVICE_TYPE and device.get('skill_id') in QUASAR_SKILL_IDS):
                            return True
    return False


def _get_iot_config(record, only_smart_speakers=False):
    # берем конфиг и гипотезы умного дома из (в порядке приоритета):
    # 1. analytics_info
    # 2. device_state (нигде не должен использоваться, кроме одной корзины из регулярного процесса)

    iot_config = record.get('iot_config')

    properties = get_path_str(record, 'analytics_info.users_info.IoT.scenario_user_info.properties')
    if properties:
        for property in properties:
            if property.get('iot_profile'):
                iot_config = copy.deepcopy(property['iot_profile'])

    if record['analytics_info'].get('iot_user_info'):
        iot_config = copy.deepcopy(record['analytics_info']['iot_user_info'])

    if not iot_config:
        return None

    # приведение формата конфига из логов к текущему
    rooms_mapping = {}
    if iot_config.get('rooms'):
        for room in iot_config['rooms']:
            rooms_mapping[room['id']] = room['name']

    for i, device in enumerate(iot_config.get('devices', [])):
        iot_config['devices'][i]['device_name'] = device.get('device_name') or device.get('name')
        iot_config['devices'][i]['device_id'] = device.get('device_id') or device.get('id')
        iot_config['devices'][i]['device_type'] = device.get('device_type') or device.get('type')
        iot_config['devices'][i]['device_analytics_type'] = device.get('analytics_type')
        iot_config['devices'][i]['device_analytics_name'] = device.get('analytics_name')
        iot_config['devices'][i]['device_capabilities'] = device.get('device_capabilities') or device.get(
            'capabilities')
        if iot_config['devices'][i]['device_capabilities'] is None:
            iot_config['devices'][i]['device_capabilities'] = []
        iot_config['devices'][i]['room_name'] = device.get('room_name') or rooms_mapping.get(device.get('room_id'))

    if iot_config.get('groups'):
        for i, group in enumerate(iot_config['groups']):
            iot_config['groups'][i]['group_name'] = group.get('group_name') or group.get('name')
            iot_config['groups'][i]['group_id'] = group.get('group_id') or group.get('id')

    if iot_config.get('scenarios'):
        for i, scenario in enumerate(iot_config['scenarios']):

            scenario['scenario_name'] = (scenario.get('scenario_name') or scenario.get('name')).strip()
            scenario['scenario_id'] = scenario.get('scenario_id') or scenario.get('id')

            trigger_phrases = []  # collect voice trigger phrases for given scenario
            for t in scenario.get('triggers', dict()):
                if t.get('type') == 'VoiceScenarioTriggerType' and t.get('voice'):
                    trigger_phrases.append(t['voice'].strip())
            scenario['trigger_phrases'] = \
                [tp for tp in trigger_phrases if tp.lower() != scenario['scenario_name'].lower()]

            iot_config['scenarios'][i] = scenario  # update config

    # добавление толокерского id устройств
    toloka_id_mapper = {}
    for i, device in enumerate(iot_config.get('devices', [])):
        iot_config['devices'][i]['device_name'] = '%s[%d]' % (device['device_name'], i + 1)
        toloka_id_mapper[device['device_id']] = i + 1
    if iot_config.get('groups'):
        for i, group in enumerate(iot_config['groups']):
            if group.get('devices'):
                for j, device in enumerate(group['devices']):
                    iot_config['groups'][i]['devices'][j]['device_name'] = '%s[%d]' % (
                        device['device_name'], toloka_id_mapper[device['device_id']])

    return iot_config


def _is_new_iot_format(record):
    """checks for new format of analytics_info - see IOT-1255"""
    objects = get_path_str(record, 'analytics_info.analytics_info.IoT.scenario_analytics_info.objects')
    return objects and any([o['id'] == 'iot_reactions' for o in objects])


def _get_iot_response(record):
    iot_response_from_analytics_info = {}
    objects = get_path_str(record, 'analytics_info.analytics_info.IoT.scenario_analytics_info.objects')
    if objects:
        for obj in objects:
            if obj.get('id') == 'hypotheses' and 'hypotheses' not in iot_response_from_analytics_info:
                iot_response_from_analytics_info['hypotheses'] = obj['hypotheses']['hypotheses']
            if obj.get('id') == 'selected_hypotheses' and 'result' not in iot_response_from_analytics_info:
                iot_response_from_analytics_info['result'] = {
                    'selected_hypotheses': obj['selected_hypotheses']['selected_hypotheses']
                }
    if iot_response_from_analytics_info:
        return iot_response_from_analytics_info

    if record.get('meta'):
        for meta in record['meta']:
            if meta.get('type') == 'smart_home_meta':
                if meta.get('payload'):
                    return meta['payload']

    return None


def get_scenario_repr(scenario):
    """produces representation string from scenario (provided as dict from config)"""
    repr_ = scenario['scenario_name']
    if scenario.get('trigger_phrases', []):
        trigger_phrases_joined = ', '.join(['{}'.format(tp) for tp in scenario.get('trigger_phrases', [])])
        repr_ += ' <' + trigger_phrases_joined + '>'
    return repr_


def iot_hypothesis_processing(
    hypothesis,
    devices_dict,
    scenarios_dict,
    colors_dict,
    scenes_dict,
    client_tz=None
):
    hypothesis_type = hypothesis.get('type')
    if hypothesis_type == 'query':  # catch queries by type. Consider combining with similar code below.
        return _('Алиса сообщает состояние указанного устройства')

    scenario = hypothesis.get('scenario')
    if scenario is not None:
        return _('Запускается сценарий умного дома{}{}').format(
            _(' <с фразами активации>: ') if 'trigger_phrases' in scenario else ': ',
            get_scenario_repr(scenarios_dict[scenario])
        )

    devices = hypothesis.get('devices', [])
    if not devices:
        return ''
    action = hypothesis['action']
    value = None
    for el in IOT_VALUE_SET:
        if el in action:
            value = action[el]
            break
    action_type = action.get('type')
    instance = action.get('instance')
    if action_type == 'devices.properties.float' or action.get('target') == 'state':
        return _('Алиса сообщает состояние устройства')

    user_config_dicts = UserConfigDicts(
        devices_dict=devices_dict,
        scenarios_dict=scenarios_dict,
        colors_dict=colors_dict,
        scenes_dict=scenes_dict,
    )
    return _get_iot_action_description(
        ActionInfo(
            devices=devices,
            user_config_dicts=user_config_dicts,
            capability_type=action_type,
            capability_instance=instance,
            value=value,
            unit=action.get('unit'),
            relativity_type=action.get('relative'),
            time_info=hypothesis.get('time_info'),
            client_tz=client_tz
        )
    )


def _get_iot_action(record, only_smart_speakers=False):
    """
    Возвращает действие, произошедшее с устройствами умного дома
    :param dict record:
    :return Optional[str]:
    """
    iot_config = _get_iot_config(record)
    if not iot_config:
        return None

    devices_dict = dict((device['device_id'], device)
                        for device in iot_config.get('devices', []))
    colors_dict = dict((color['id'], color['name'].lower())
                       for color in iot_config.get('colors', []))
    scenes_dict = {}
    for device in iot_config.get('devices', []):
        for capability in device.get('capabilities', []):
            if capability.get('analytics_type') == 'devices.capabilities.color_setting' and capability.get(
                    'color_setting_capability_parameters'):
                scenes = get_path_str(capability, 'color_setting_capability_parameters.color_scene.scenes', [])
                if scenes:
                    scenes_dict = dict((scene['id'], scene.get('name', '').lower()) for scene in scenes)
    scenarios = iot_config.get('scenarios', [])
    scenarios_dict = dict((scenario['scenario_id'], scenario) for scenario in scenarios)

    user_config_dicts = UserConfigDicts(
        devices_dict=devices_dict,
        scenarios_dict=scenarios_dict,
        colors_dict=colors_dict,
        scenes_dict=scenes_dict,
    )

    # новый формат аналитики: https://st.yandex-team.ru/IOT-1255
    if _is_new_iot_format(record):
        return _get_iot_actions_from_new_analytics_info(record, user_config_dicts)  # TO DO: пробросить tz + time_info

    iot_response = _get_iot_response(record)
    if not iot_response:
        return None

    if iot_response['result'].get('status', 'success') != 'success':
        return None

    selected_hypotheses = iot_response['result']['selected_hypotheses']
    for i, selected_hypothesis in enumerate(selected_hypotheses):
        if 'scenario' not in selected_hypothesis:
            for hypothesis in iot_response['hypotheses']:
                if selected_hypothesis['id'] == hypothesis['id']:
                    selected_hypotheses[i]['action'] = hypothesis['action']
                    if hypothesis.get('scenario'):
                        selected_hypotheses[i]['scenario'] = hypothesis['scenario']

    cur_actions = []
    for selected_hypothesis in selected_hypotheses:
        try:
            action = iot_hypothesis_processing(
                selected_hypothesis, devices_dict, scenarios_dict, colors_dict, scenes_dict,
                client_tz=_get_tz_from_record(record)
            )
        except Exception as e:
            action = 'ERROR in iot_hypothesis_processing: ' + repr(e)
        cur_actions.append(action)
    cur_actions = list(set(cur_actions))  # уникализация на случай, если образовалось несколько одинаковых действий
    return '\n'.join(cur_actions)


def get_instance_for_capability(capability):
    if capability.get('instance'):
        return capability.get('instance')
    for key, value in list(capability.items()):
        if key.endswith('parameters'):
            if value.get('instance'):
                return value.get('instance')
    return None


def iot_capability_processing(capability):
    if capability.get('analytics_name'):
        # костыль, чтобы достать название кнопки не размеченное в Бульбазавре
        if capability['analytics_name'] != _('обученная пользователем кнопка'):
            return capability['analytics_name']

    capability_string = ''
    capability_type = capability['type']
    instance = get_instance_for_capability(capability)
    if capability_type in ('devices.capabilities.on_off', 'OnOffCapabilityType'):
        capability_string = _('включение/выключение')
    elif capability_type in ('devices.capabilities.color_setting', 'ColorSettingCapabilityType'):
        params = capability.get('parameters') or capability.get('color_setting_capability_parameters')
        capability_string = _('изменение ')
        color_capabilities = []
        if not params:
            color_capabilities.append(_('цвета'))
        else:
            count = 0
            if 'color_model' in params and (
                params['color_model'] in ['hsv', 'rgb'] or params['color_model'].get('type') in ['HsvColorModel',
                                                                                                 'RgbColorModel']):
                color_capabilities.append(_('цвета'))
                count += 1
            if 'temperature_k' in params:
                color_capabilities.append(_('цветовой температуры'))
                count += 1
            if 'color_scene' in params:
                color_capabilities.append(_('цветовой сцены'))
                count += 1
            if count != len(params):
                raise ValueError('Unknown color setting param\n' + json.dumps(capability))
        capability_string += ', '.join(color_capabilities)
    elif capability_type in ('devices.capabilities.range', 'RangeCapabilityType'):
        if instance == 'channel':
            capability_string += _('переключение канала')
        elif instance == 'temperature':
            capability_string += _('настройка температуры')
        elif instance == 'brightness':
            capability_string += _('изменение яркости')
        elif instance == 'volume':
            capability_string += _('изменение громкости')
        elif instance == 'open':
            capability_string += _('открытие')
        else:
            raise ValueError('Unknown case in IOT capability processing, please update code\n' + json.dumps(capability))
    elif capability_type in ('devices.capabilities.mode', 'ModeCapabilityType'):
        if instance == 'fan_speed':
            capability_string += _('изменение скорости вентилятора')
        elif instance == 'thermostat':
            capability_string += _('изменение режима термостата')
        elif instance == '':
            capability_string += _('изменение настроек')
        elif instance == 'input_source':
            capability_string += _('изменение источника сигнала')
        elif instance == 'work_speed':
            capability_string += _('изменение скорости работы')
        else:
            raise ValueError('Unknown case in IOT capability processing, please update code\n' + json.dumps(capability))
    elif capability_type in ('devices.capabilities.toggle', 'ToggleCapabilityType'):
        capability_string = _('включение/выключение ')
        if instance == 'mute':
            capability_string += _('беззвучного режима')
        elif instance == 'ionization':
            capability_string += _('ионизации')
        elif instance == 'backlight':
            capability_string += _('задней подстветки')
        elif instance == 'controls_locked':
            capability_string += _('возможности усправления')
        elif instance == 'oscillation':
            capability_string += _('колебания')
        elif instance == 'keep_warm':
            capability_string += _('поддержания температуры')
        elif instance == 'pause':
            capability_string += _('паузы')
        else:
            raise ValueError('Unknown case in IOT capability processing, please update code\n' + json.dumps(capability))
    elif capability_type in ('devices.capabilities.custom.button', 'CustomButtonCapabilityType'):
        params = capability.get('parameters') or capability.get('custom_button_capability_parameters')
        if params and params.get('instance_names'):
            capability_string = _('кнопка:[{}]').format(', '.join(params["instance_names"]))
        else:
            capability_string = _('пользовательская кнопка')
    else:
        raise ValueError('Unknown case in IOT capability processing, please update code\n' + json.dumps(capability))
    return capability_string


def _get_iot_extra_states(record, only_smart_speakers=False):
    """
    Возвращает массив состояний устройств умного дома
    :param dict record:
    :return List[dict]:
    """
    iot_config = _get_iot_config(record)
    if not iot_config:
        return []

    extra_states = []

    # дома пользователя
    households_count = 0
    if iot_config.get('households'):
        objects = set()
        for hh in iot_config['households']:
            objects.add(hh['name'].strip())
        if objects:
            households_count = len(objects)
            extra_states.append({
                'type': _('Дома пользователя'),
                'content': sorted(list(objects))
            })

    # комнаты пользователя
    if iot_config.get('rooms'):
        objects = set()
        for room in iot_config['rooms']:
            room_name = room['name'].strip()
            if households_count > 1:
                hh_name = get_household_by_household_id(room['household_id'], iot_config)
                room_name = '{}[{}]'.format(room_name, hh_name)
            objects.add(room_name)
        if objects:
            extra_states.append({
                'type': _('Комнаты пользователя'),
                'content': sorted(list(objects))
            })

    # сценарии пользователя
    if iot_config.get('scenarios'):
        unique_scenarios = set()
        for scenario in iot_config['scenarios']:
            unique_scenarios.add(get_scenario_repr(scenario))
        if unique_scenarios:
            extra_states.append({
                'type': _('Сценарии умного дома, созданные пользователем'),
                'content': sorted(list(unique_scenarios))
            })

    # таблица устройств пользователей
    if only_smart_speakers:
        smart_speakers = []
        for device_info in iot_config.get('devices', []):
            if device_info.get('analytics_type', '').startswith("devices.types.smart_speaker"):
                smart_speakers.append(device_info)
        iot_config['devices'] = smart_speakers
    content = []
    for device in iot_config.get('devices', []):
        capabilities = []
        custom_buttons = []
        for cap in device['device_capabilities']:
            try:
                processed_cap = iot_capability_processing(cap)
            except Exception as e:
                processed_cap = 'ERROR: ' + repr(e)

            if processed_cap.lower().startswith(_('обученная пользователем кнопка')):
                custom_buttons.append(processed_cap)
            else:
                capabilities.append(processed_cap)

        if len(custom_buttons) <= 1:
            capabilities.extend(custom_buttons)
        else:
            custom_buttons = [b.replace(_('обученная пользователем кнопка: '), '') for b in custom_buttons]
            capabilities.append(_('Обученные пользователем кнопки: ') +
                                ', '.join(['"' + b + '"' for b in custom_buttons]))

        aliases = ['<' + a + '>' for a in device.get('aliases', [])]

        content.append([
            {
                'col_name': _('Название устройства'),
                'col_content': device['device_name'] + ''.join(aliases)
            },
            {
                'col_name': _('Тип устройства'),
                'col_content': device['device_analytics_name'] or device[
                    'device_analytics_type'] or IOT_DEVICE_TYPES.get(
                    device['device_type'], 'ERROR: ' + str(device['device_type'])
                )
            },
            {
                'col_name': _('Комната'),
                'col_content': device['room_name']
            },
            {
                'col_name': _('Возможности устройства'),
                'col_content': capabilities
            }
        ])

    if content:
        extra_states.append({
            'type': _('Устройства умного дома'),
            'content': content
        })

    # группы пользователя
    if iot_config.get('groups'):
        content = []
        for group in iot_config['groups']:
            if group.get('devices'):
                content.append([
                    {
                        'col_name': _('Название группы'),
                        'col_content': group['group_name']
                    },
                    {
                        'col_name': _('Устройства группы'),
                        'col_content': [device['device_name'] for device in group['devices']]
                    }
                ])
        if content:
            extra_states.append({
                'type': _('Группы устройств'),
                'content': content
            })

    return extra_states
