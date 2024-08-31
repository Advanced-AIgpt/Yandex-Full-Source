# coding: utf-8

from .utils import get_device_state_data

from .visualize_alarms_timers import (
    timers_processing,
    alarms_processing,
    _get_tz_from_record,
    _get_cur_date,
)
from .visualize_video import (
    _get_visualized_state_screen_extra_states,
    _get_visualized_state_for_currently_playing_item,
)

import six

import sys
reload(sys)  # noqa
sys.setdefaultencoding('utf8')  # noqa

from .utils import load_translations
t = load_translations('visualize_state')
t.install()


def _(text):
    """
    HACK: Обёртка над gettext для python2 строк
    вместо: `_ = t.gettext`
    :param str text:
    """
    return t.gettext(text.decode('utf-8'))


def _get_visualized_extra_states(record):
    """
    Возвращает состояния устройства
    :param dict record:
    :return List[dict]:
    """
    extra_states = []

    messenger_call = get_device_state_data(record, 'messenger_call')
    if messenger_call:
        def _mapper(data):
            if data.get('incoming'):
                return _('входящий вызов')
            if data.get('current'):
                return _('пользователь ведет разговор')
            return None

        extra_states.append({
            'type': _('Звонки'),
            'content': _mapper(messenger_call)
        })

    sound_muted = get_device_state_data(record, 'sound_muted')
    if sound_muted:
        extra_states.append({
            'type': _('Беззвучный режим'),
            'content': _('включен')
        })
    timers = get_device_state_data(record, 'timers', {'active_timers': []})
    if timers.get('active_timers'):
        extra_states.append(timers_processing(timers['active_timers']))

    if record.get('filtration_level'):
        mapper = {
            'children': _('Семейный'),
            'without': _('Без ограничений'),
            'medium': _('Умеренный'),
            'safe': _('Безопасный'),
        }
        extra_states.append({
            'type': _('Фильтрация контента'),
            'content': mapper.get(record['filtration_level'], _('Умеренный'))
        })

    if record.get('location'):
        extra_states.append({
            'type': _('Местоположение пользователя'),
            'content': record['location']
        })

    alarm_state = get_device_state_data(record, 'alarm_state', {})
    if alarm_state.get('icalendar'):
        alarms = alarms_processing(alarm_state['icalendar'],
                                   _get_tz_from_record(record),
                                   _get_cur_date(record),
                                   alarm_state.get('currently_playing', False))
        if alarms:
            ret = {
                'type': _('Будильники'),
                'content': alarms
            }
            if alarm_state.get('currently_playing'):
                ret['playback'] = _('В настоящее время звонит будильник!')
            extra_states.append(ret)
        elif alarm_state.get('currently_playing', False):
            ret = {
                'type': _('Будильники'),
                'playback': _('В настоящее время звонит будильник!')
            }
            extra_states.append(ret)

    # here's screen state processing
    video = get_device_state_data(record, 'video', {})
    current_screen = video.get('current_screen', 'main')
    extra_states += _get_visualized_state_screen_extra_states(record)

    navigator = get_device_state_data(record, 'navigator', {})
    if navigator:
        if navigator.get('home') and navigator.get('work'):
            extra_states.append({
                'type': _('Настройки пользователя'),
                'content': _('Указан адрес дома и работы')
            })
        elif navigator.get('home'):
            extra_states.append({
                'type': _('Настройки пользователя'),
                'content': _('Указан адрес дома')
            })
        elif navigator.get('work'):
            extra_states.append({
                'type': _('Настройки пользователя'),
                'content': _('Указан адрес работы')
            })

    # here is processing music, radio, audio and video states
    music = get_device_state_data(record, 'music', {})
    radio = get_device_state_data(record, 'radio', {})
    audio_player = get_device_state_data(record, 'audio_player', {})
    music_last_play_timestamp = music.get('last_play_timestamp')
    audio_player_last_play_timestamp = audio_player.get('last_play_timestamp')

    current_player = None

    if music_last_play_timestamp is not None and audio_player_last_play_timestamp is not None:
        if music_last_play_timestamp > audio_player_last_play_timestamp:
            current_player = 'music'
        else:
            current_player = 'audio_player'
    elif audio_player:
        current_player = 'audio_player'
    elif music:
        current_player = 'music'

    if current_player == 'music':
        ret = {
            'type': _('Последняя прослушанная музыка')
        }
        if music.get('currently_playing'):
            now = music['currently_playing']

            if now.get('track_info') and now['track_info'].get('artists') and now['track_info'].get('title'):
                if isinstance(now['track_info'].get('artists'), six.string_types):
                    artists = _(now['track_info'].get('artists'))
                else:
                    artists = [y for y in [x.get('name') for x in now['track_info']['artists']] if y]
                if artists:
                    ret['content'] = ', '.join(artists) + ' - ' + now['track_info']['title']
            else:
                ret['content'] = _('аудиозапись')

            if music['player']['pause'] \
                or (current_screen == 'video_player' and video and
                    video.get('currently_playing') and not video['currently_playing'].get('paused')) \
                or (current_screen == 'radio_player' and radio and
                    'pause' in radio.get('player', {}) and not radio['player']['pause']):
                ret['playback'] = _('Сейчас воспроизведение поставлено на паузу')
            else:
                ret['playback'] = _('Сейчас этот трек воспроизводится')
            extra_states.append(ret)

    elif current_player == 'audio_player':
        if audio_player.get('current_stream'):
            now = audio_player.get('current_stream')
            ret = {
                'type': _('Последнее прослушанное аудио'),
                # device state for thin player don't have metadata yet
                'content': _('аудиозапись')
            }
            title = now.get('title')
            if title:
                subtitle = now.get('subtitle')
                ret['content'] = (subtitle if subtitle else title) + ' - ' + title
            else:
                # device state for thin player may not have title
                ret['content'] = _('аудиозапись')

            if audio_player.get('player_state') == 'Playing':
                ret['playback'] = _('Сейчас это аудио воспроизводится')
            if audio_player.get('player_state') in ['Stopped', 'Paused']:
                ret['playback'] = _('Сейчас воспроизведение поставлено на паузу')
            extra_states.append(ret)

    if radio:
        if radio.get('currently_playing', {}).get('radioTitle'):
            ret = {
                'type': _('Последняя включенная радиостанция'),
                'content': radio['currently_playing']['radioTitle']
            }
            if current_screen == 'radio_player' and radio['player']['pause']:
                ret['playback'] = _('Сейчас радио выключено')
                extra_states.append(ret)
            elif not radio['player']['pause']:
                ret['playback'] = _('Сейчас радио включено')
                extra_states.append(ret)

    extra_states += _get_visualized_state_for_currently_playing_item(record)

    return extra_states


def _get_visualized_volume_state(record):
    """
    Возвращает уровень громкости Алисы
    :param dict record:
    :return str|int:
    """
    sound_level = get_device_state_data(record, 'sound_level', 1)
    return '0' if sound_level == 0 else sound_level
