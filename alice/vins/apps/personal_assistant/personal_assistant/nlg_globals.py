# coding: utf-8
from __future__ import unicode_literals

from math import sin, cos, sqrt, atan2, radians

from jinja2 import contextfunction

from personal_assistant import clients
from personal_assistant import intents
from datetime import timedelta

from vins_core.utils.lemmer import Inflector
from vins_core.utils.strings import smart_unicode


_inflector = Inflector('ru')


def _get_req_info(context):
    return context['req_info']


def _get_app_info(context):
    return context['req_info'].app_info


def _get_device_state(context):
    return context['req_info'].device_state or {}


def _get_session(context):
    return context['session']


@contextfunction
def is_stroka(context):
    return clients.is_stroka(_get_app_info(context))


@contextfunction
def is_telegram(context):
    return clients.is_telegram(_get_app_info(context))


@contextfunction
def is_smart_speaker(context):
    return clients.is_smart_speaker(_get_app_info(context))


@contextfunction
def is_quasar(context):
    return clients.is_quasar(_get_app_info(context))


@contextfunction
def is_legatus(context):
    return clients.is_legatus(_get_app_info(context))


@contextfunction
def is_mini_speaker(context):
    return clients.is_mini_speaker(_get_app_info(context))


@contextfunction
def is_mini_speaker_dexp(context):
    return clients.is_mini_speaker_dexp(_get_app_info(context))


@contextfunction
def is_mini_speaker_lg(context):
    return clients.is_mini_speaker_lg(_get_app_info(context))


@contextfunction
def is_mini_speaker_elari(context):
    return clients.is_mini_speaker_elari(_get_app_info(context))


@contextfunction
def has_uncontrollable_updates(context):
    return clients.has_uncontrollable_updates(_get_app_info(context))


@contextfunction
def is_desktop(context):
    return clients.is_desktop(_get_app_info(context))


@contextfunction
def is_android(context):
    return clients.is_android(_get_app_info(context))


@contextfunction
def is_ios(context):
    return clients.is_ios(_get_app_info(context))


@contextfunction
def is_searchapp_android(context):
    return clients.is_searchapp_android(_get_app_info(context))


@contextfunction
def is_searchapp_ios(context):
    return clients.is_searchapp_ios(_get_app_info(context))


@contextfunction
def is_searchapp(context):
    return clients.is_searchapp(_get_app_info(context))


def _get_attention(context, type_):
    return next((a for a in context['context']['attention'] if a.attention_type == type_), None)


@contextfunction
def is_active_attention(context, type_):
    return _get_attention(context, type_) is not None


@contextfunction
def get_attention(context, type_):
    return _get_attention(context, type_)


def _get_commands(context, type_):
    return (c for c in context['context']['command'] if c.command_type == type_)


@contextfunction
def get_commands(context, type_):
    return _get_commands(context, type_)


@contextfunction
def is_navigator(context):
    return clients.is_navigator(_get_app_info(context))


@contextfunction
def is_navigator_projected_mode(context):
    return (
        'req_info' in context and
        intents.is_navigator_projected_mode(context['req_info'])
    )


@contextfunction
def is_client_with_navigator(context):
    return (
        'req_info' in context and
        clients.is_client_with_navigator(context['req_info'])
    )


@contextfunction
def is_elari_watch(context):
    return clients.is_elari_watch(_get_app_info(context))


@contextfunction
def is_webtouch(context):
    return clients.is_webtouch(_get_app_info(context))


@contextfunction
def is_sdg(context):
    return clients.is_sdg(_get_app_info(context))


@contextfunction
def is_gc_skill(context):
    session = _get_session(context)
    if session is not None:
        return session.app_id == 'gc_skill'
    return False


@contextfunction
def is_yandex_drive(context):
    return clients.is_yandex_drive(_get_req_info(context))


@contextfunction
def is_auto(context):
    return clients.is_auto(_get_app_info(context))


@contextfunction
def is_auto_kaptur(context):
    return clients.is_auto_kaptur(_get_req_info(context))


@contextfunction
def is_ya_music_app(context):
    return clients.is_ya_music_app(_get_app_info(context))


@contextfunction
def is_yabro_windows(context):
    return clients.is_yabro_windows(_get_app_info(context))


@contextfunction
def is_yabro_desktop(context):
    return clients.is_yabro_desktop(_get_app_info(context))


@contextfunction
def is_yabro_mobile_android(context):
    return clients.is_yabro_mobile_android(_get_app_info(context))


@contextfunction
def is_yabro_mobile_ios(context):
    return clients.is_yabro_mobile_ios(_get_app_info(context))


@contextfunction
def is_tv_device(context):
    return clients.is_tv_device(_get_app_info(context))


@contextfunction
def has_alicesdk_player(context):
    return clients.has_alicesdk_player(_get_req_info(context))


@contextfunction
def is_tv_plugged_in(context):
    return _get_device_state(context).get('is_tv_plugged_in', False)


@contextfunction
def is_yandex_spotter(context):
    if 'req_info' not in context:
        return False

    req_info = context['req_info']
    return (req_info.device_state and 'device_config' in req_info.device_state and
            'spotter' in req_info.device_state['device_config'] and
            req_info.device_state['device_config']['spotter'] == 'yandex')


@contextfunction
def is_child_request(context):
    if 'req_info' not in context:
        return False

    req_info = context['req_info']
    event = req_info.event
    if event.event_type == 'voice_input':
        biometry_classification = event.biometry_classification()
        if not biometry_classification:
            return False
        for classification in biometry_classification['simple']:
            if classification['tag'] == 'children' and classification['classname'] == 'child':
                return True
    return False


@contextfunction
def is_child_content_settings(context):
    return content_restriction(context) == 'safe'


@contextfunction
def is_child_microintent(context):
    return is_elari_watch(context) or is_child_content_settings(context) or is_child_request(context)


@contextfunction
def assistant_name(context):
    if is_yandex_spotter(context):
        return u'Яндекс'
    else:
        return u'Алиса'


@contextfunction
def get_device_id(context, preamble=False):
    if 'req_info' not in context:
        return u''
    device_state = context['req_info'].device_state or {}
    device_id = device_state.get('device_id')
    if not device_id:
        return u''
    device_id = smart_unicode(device_id)
    if preamble:
        return u'Выглядит вот так: {}.'.format(device_id)
    else:
        return device_id


@contextfunction
def username_suffix(context):
    # Note: do not use username_suffix inside text-only or voice-only block!
    if context['context'] is not None and 'userinfo' in context['context']:
        userinfo = context['context']['userinfo']
        username = userinfo.username()
        is_silent = userinfo.is_silent()
        if username and len(username) > 0:
            if is_silent:
                return "<vins_only_text>, %s</vins_only_text>" % username
            return ", %s" % username

    return ""


@contextfunction
def username_infix(context):
    # Note: do not use username_infix inside text-only or voice-only block!
    if context['context'] is not None and 'userinfo' in context['context']:
        userinfo = context['context']['userinfo']
        username = userinfo.username()
        is_silent = userinfo.is_silent()
        if username and len(username) > 0:
            if is_silent:
                return "<vins_only_text>, %s,</vins_only_text>" % username
            return ", %s," % username
    return ""


@contextfunction
def username_prefix(context, begin_text=""):
    # Note: do not use username_prefix inside text-only or voice-only block!
    if context['context'] is not None and 'userinfo' in context['context']:
        userinfo = context['context']['userinfo']
        username = userinfo.username()
        is_silent = userinfo.is_silent()
        if username and len(username) > 0:
            space = ''
            if begin_text:
                begin_text = begin_text.strip()
                if len(begin_text) > 0:
                    begin_text = begin_text[0].lower() + begin_text[1:]
                    space = ' '
            if is_silent:
                return "<vins_only_text>%s,%s</vins_only_text>%s" % (username, space, begin_text)
            return "%s,%s%s" % (username, space, begin_text)

    return begin_text


@contextfunction
def username_prefix_if_needed(context, begin_text=""):
    # Note: do not use username_prefix_if_needed inside text-only or voice-only block!
    if context['context'] is not None and 'userinfo' in context['context']:
        if context['context']['userinfo'].is_used():
            return begin_text
    return username_prefix(context, begin_text)


@contextfunction
def content_restriction(context):
    if 'req_info' not in context:
        return ''
    device_state = context['req_info'].device_state or {}
    device_config = device_state.get('device_config')
    if not device_config or 'content_settings' not in device_config:
        return ''
    return device_config['content_settings']


def add_hours(dt, hours=0):
    return dt + timedelta(hours=hours)


def render_traffic_forecast(current_level, traffic_forecast, max_range=1):
    """
    Transform numeric representation of the forecast to short text.
    :param current_level: int or None, the value of current traffic level
    :param traffic_forecast: ordered list of dicts, like [{'hour': int, 'score': int}, ...]
    :param max_range: the maximal difference in traffic levels which is considered as "roughly the same"
    :return: string representation of the forecast dynamics
    """
    pluralize = _inflector.pluralize

    if len(traffic_forecast) == 0:
        return ''
    if current_level is None:
        current_level = int(traffic_forecast[0]['score'])
    current_level = int(current_level)
    min_value = current_level
    max_value = current_level
    for i, part in enumerate(traffic_forecast):
        new_score = int(part['score'])
        new_hour = int(part['hour'])
        max_value = max(max_value, new_score)
        min_value = min(min_value, new_score)
        if max_value - min_value > max_range:
            time = '{} {}'.format(new_hour, pluralize('час', new_hour, 'dat'))
            score = '{} {}'.format(new_score, pluralize('балл', new_score, 'gen'))
            if part['score'] > current_level:
                return 'К {} пробки вырастут до {}'.format(time, score)
            elif part['score'] < current_level:
                return 'К {} пробки упадут до {}'.format(time, score)
            else:
                # with predicted score equal to current level, this condition should never activate
                return 'К {} пробки будут в районе {}, однако'.format(time, score)
    time_span = len(traffic_forecast)
    if time_span == 1:
        time = 'ближайший час'
    else:
        time = 'ближайшие {} {}'.format(time_span, pluralize('час', time_span, 'nomn'))
    score = '{} {}'.format(max_value, pluralize('балл', max_value, 'nomn'))
    if min_value < max_value:
        score = '{}-{}'.format(min_value, score)
    return 'В {} пробки останутся на уровне {}'.format(time, score)


def geodesic_distance(point_a, point_b):
    """ Calculate approximate distance between two points, based on spherical Earth assumption """
    # approximate radius of earth in meters
    R = 6373000.0
    if not point_a or not point_b:
        return None
    loc1, loc2 = point_a.get('location'), point_b.get('location')
    if not loc1 or not loc2:
        return None
    lat1 = radians(loc1['lat'])
    lon1 = radians(loc1['lon'])
    lat2 = radians(loc2['lat'])
    lon2 = radians(loc2['lon'])
    dlon = lon2 - lon1
    dlat = lat2 - lat1
    a = sin(dlat / 2) ** 2 + cos(lat1) * cos(lat2) * sin(dlon / 2) ** 2
    c = 2 * atan2(sqrt(a), sqrt(1 - a))
    distance = R * c
    return distance
