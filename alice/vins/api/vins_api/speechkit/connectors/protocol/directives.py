# coding:utf-8

from __future__ import unicode_literals

import base64
import json
import logging
import six
from functools import wraps

from google.protobuf import json_format

from alice.megamind.protos.common.app_type_pb2 import EAppType
from alice.megamind.protos.scenarios.directives_pb2 import (
    TDirective, TUpdateDialogInfoDirective, TPlayerRewindDirective, TSetSearchFilterDirective, TOpenSettingsDirective,
    TNavigateBrowserDirective, TServerDirective, TUpdateDatasyncDirective, TDrawScledAnimationsDirective
)

from vins_core.dm.response import ClientActionDirective, ServerActionDirective, ActionDirective, UniproxyActionDirective, TypedSemanticFrameDirective
from vins_core.utils.json_util import dict_to_struct

logger = logging.getLogger(__name__)

SETTINGS_TARGET_MAPPING = {
    'accessibility': TOpenSettingsDirective.ESettingsTarget.Accessibility,
    'archiving': TOpenSettingsDirective.ESettingsTarget.Archiving,
    'colors': TOpenSettingsDirective.ESettingsTarget.Colors,
    'datetime': TOpenSettingsDirective.ESettingsTarget.Datetime,
    'defaultapps': TOpenSettingsDirective.ESettingsTarget.DefaultApps,
    'defender': TOpenSettingsDirective.ESettingsTarget.Defender,
    'desktop': TOpenSettingsDirective.ESettingsTarget.Desktop,
    'display': TOpenSettingsDirective.ESettingsTarget.Display,
    'firewall': TOpenSettingsDirective.ESettingsTarget.Firewall,
    'folders': TOpenSettingsDirective.ESettingsTarget.Folders,
    'homegroup': TOpenSettingsDirective.ESettingsTarget.HomeGroup,
    'indexing': TOpenSettingsDirective.ESettingsTarget.Indexing,
    'keyboard': TOpenSettingsDirective.ESettingsTarget.Keyboard,
    'language': TOpenSettingsDirective.ESettingsTarget.Language,
    'lockscreen': TOpenSettingsDirective.ESettingsTarget.LockScreen,
    'microphone': TOpenSettingsDirective.ESettingsTarget.Microphone,
    'mouse': TOpenSettingsDirective.ESettingsTarget.Mouse,
    'network': TOpenSettingsDirective.ESettingsTarget.Network,
    'notifications': TOpenSettingsDirective.ESettingsTarget.Notifications,
    'power': TOpenSettingsDirective.ESettingsTarget.Power,
    'print': TOpenSettingsDirective.ESettingsTarget.Print,
    'privacy': TOpenSettingsDirective.ESettingsTarget.Privacy,
    'sound': TOpenSettingsDirective.ESettingsTarget.Sound,
    'startmenu': TOpenSettingsDirective.ESettingsTarget.StartMenu,
    'system': TOpenSettingsDirective.ESettingsTarget.System,
    'tabletmode': TOpenSettingsDirective.ESettingsTarget.TabletMode,
    'tablo': TOpenSettingsDirective.ESettingsTarget.Tablo,
    'themes': TOpenSettingsDirective.ESettingsTarget.Themes,
    'useraccount': TOpenSettingsDirective.ESettingsTarget.UserAccount,
    'volume': TOpenSettingsDirective.ESettingsTarget.Volume,
    'vpn': TOpenSettingsDirective.ESettingsTarget.Vpn,
    'wifi': TOpenSettingsDirective.ESettingsTarget.WiFi,
    'winupdate': TOpenSettingsDirective.ESettingsTarget.WinUpdate,
    'addremove': TOpenSettingsDirective.ESettingsTarget.AddRemove,
    'taskmanager': TOpenSettingsDirective.ESettingsTarget.TaskManager,
    'devicemanager': TOpenSettingsDirective.ESettingsTarget.DeviceManager,
}

_client_action_serializers = {}

SERVER_DIRECTIVE_NAMES = {
    'memento_change_user_objects_directive',
    'raw_server_directive',
    'send_push',
    'update_datasync',
}


def _register_client_action_serializer(serializer, action_name):
    _client_action_serializers[action_name] = serializer


def _has_client_action_serializer(action_name):
    return action_name in _client_action_serializers


def _get_client_action_serializer(action_name):
    return _client_action_serializers.get(action_name)


def _smart_dialog_id(dialog_id):
    if dialog_id is None or isinstance(dialog_id, (str, unicode)):
        return dialog_id
    if isinstance(dialog_id, (tuple, list)):
        return dialog_id[0]
    raise ValueError('Invalid type of dialog_id: %s' % type(dialog_id))


def _serialize_client_directive(directive_class_name):
    def decorator(f):
        @wraps(f)
        def inner(directive):
            proto = TDirective()
            proto_directive_body = getattr(proto, directive_class_name)
            proto_directive_body.Name = directive.sub_name or ''  # explicitly set empty string to choose case in oneof
            f(directive.payload or {}, proto_directive_body)
            return proto

        return inner

    return decorator


def _rewind_type_to_enum(rewind_type):
    if rewind_type == 'forward':
        return TPlayerRewindDirective.EType.Forward
    if rewind_type == 'backward':
        return TPlayerRewindDirective.EType.Backward
    if rewind_type == 'absolute':
        return TPlayerRewindDirective.EType.Absolute
    raise ValueError('Unexpected rewind: "{}" of type "{}"'.format(rewind_type, type(rewind_type)))


def _search_level_to_enum(search_level):
    if search_level == 'none':
        return TSetSearchFilterDirective.ESearchLevel.None
    if search_level == 'strict':
        return TSetSearchFilterDirective.ESearchLevel.Strict
    if search_level == 'moderate':
        return TSetSearchFilterDirective.ESearchLevel.Moderate
    raise ValueError('Unexpected search level: "{}" of type "{}"'.format(search_level, type(search_level)))


def _settings_target_to_enum(settings_target):
    enum_target = SETTINGS_TARGET_MAPPING.get(settings_target)
    if enum_target is not None:
        return enum_target
    raise ValueError('Unexpected settings target: "{}" of type "{}"'.format(settings_target, type(settings_target)))


def _browser_command_to_enum(command):
    if command == 'clear_history':
        return TNavigateBrowserDirective.ECommand.ClearHistory
    if command == 'close_browser':
        return TNavigateBrowserDirective.ECommand.CloseBrowser
    if command == 'go_home':
        return TNavigateBrowserDirective.ECommand.GoHome
    if command == 'new_tab':
        return TNavigateBrowserDirective.ECommand.NewTab
    if command == 'open_bookmarks_manager':
        return TNavigateBrowserDirective.ECommand.OpenBookmarksManager
    if command == 'open_history':
        return TNavigateBrowserDirective.ECommand.OpenHistory
    if command == 'open_incognito_mode':
        return TNavigateBrowserDirective.ECommand.OpenIncognitoMode
    if command == 'restore_tab':
        return TNavigateBrowserDirective.ECommand.RestoreTab
    raise ValueError('Unexpected command: "{}"'.format(command))


def _compression_type_to_enum(compression_type):
    if compression_type == 'none':
        return TDrawScledAnimationsDirective.TAnimation.ECompressionType.None
    elif compression_type == 'gzip':
        return TDrawScledAnimationsDirective.TAnimation.ECompressionType.Gzip
    return TDrawScledAnimationsDirective.TAnimation.ECompressionType.Unknown


def _animation_stop_policy_to_enum(animation_stop_policy):
    if animation_stop_policy == 'play_once':
        return TDrawScledAnimationsDirective.EAnimationStopPolicy.PlayOnce
    elif animation_stop_policy == 'play_once_till_end_of_tts':
        return TDrawScledAnimationsDirective.EAnimationStopPolicy.PlayOnceTillEndOfTTS
    elif animation_stop_policy == 'repeat_last_till_end_of_tts':
        return TDrawScledAnimationsDirective.EAnimationStopPolicy.RepeatLastTillEndOfTTS
    elif animation_stop_policy == 'repeat_last_till_next_directive':
        return TDrawScledAnimationsDirective.EAnimationStopPolicy.RepeatLastTillNextDirective
    return TDrawScledAnimationsDirective.EAnimationStopPolicy.Unknown


def _serialize_callback(server_action_directive):
    directive = TDirective()
    callback = directive.CallbackDirective

    callback.Name = server_action_directive.name
    callback.IgnoreAnswer = server_action_directive.ignore_answer
    json_format.ParseDict(server_action_directive.payload, callback.Payload)

    return directive


def _serialize_typed_semantic_frame_directive(tsf):
    directive = TDirective()
    callback = directive.CallbackDirective
    callback.Name = tsf.name
    tsf_dict = tsf.to_dict()
    payload = {
        'typed_semantic_frame': tsf_dict['payload'],
        'analytics': tsf_dict['analytics'],
    }
    json_format.ParseDict(payload, callback.Payload)

    return directive


@_serialize_client_directive('OpenDialogDirective')
def _serialize_open_dialog(payload, open_dialog):
    open_dialog.DialogId = _smart_dialog_id(payload.get('dialog_id'))
    for inner_directive in payload.get('directives') or []:
        open_dialog.Directives.add().CopyFrom(serialize_directive(inner_directive))


def _serialize_style(style):
    proto_style = TUpdateDialogInfoDirective.TStyle()

    proto_style.SuggestBorderColor = style.get('suggest_border_color') or ''
    proto_style.SuggestBorderColor = style.get('suggest_border_color') or ''
    proto_style.UserBubbleFillColor = style.get('user_bubble_fill_color') or ''
    proto_style.SuggestTextColor = style.get('suggest_text_color') or ''
    proto_style.SuggestFillColor = style.get('suggest_fill_color') or ''
    proto_style.UserBubbleTextColor = style.get('user_bubble_text_color') or ''
    proto_style.SkillActionsTextColor = style.get('skill_actions_text_color') or ''
    proto_style.SkillBubbleFillColor = style.get('skill_bubble_fill_color') or ''
    proto_style.SkillBubbleTextColor = style.get('skill_bubble_text_color') or ''
    proto_style.OknyxLogo = style.get('oknyx_logo') or ''

    for color in style.get('oknyx_error_colors') or []:
        proto_style.OknyxErrorColors.append(color)

    for color in style.get('oknyx_normal_colors') or []:
        proto_style.OknyxNormalColors.append(color)

    return proto_style


@_serialize_client_directive('UpdateDialogInfoDirective')
def _serialize_update_dialog_info(payload, update_dialog_info):
    update_dialog_info.Style.CopyFrom(_serialize_style(payload.get('style') or {}))
    update_dialog_info.DarkStyle.CopyFrom(_serialize_style(payload.get('dark_style') or {}))
    update_dialog_info.Title = payload.get('title') or ''
    update_dialog_info.Url = payload.get('url') or ''
    update_dialog_info.ImageUrl = payload.get('url') or ''

    for menu_item in payload.get('menu_items') or []:
        item = update_dialog_info.MenuItems.add()
        item.Url = menu_item.get('url') or ''
        item.Title = menu_item.get('title') or ''


@_serialize_client_directive('OpenUriDirective')
def _serialize_open_uri(payload, open_uri):
    open_uri.Uri = payload.get('uri') or ''


@_serialize_client_directive('ShowPromoDirective')
def _serialize_show_promo(payload, show_promo):
    pass


@_serialize_client_directive('EndDialogSessionDirective')
def _serialize_end_dialog_session(payload, end_dialog_session):
    end_dialog_session.DialogId = _smart_dialog_id(payload.get('dialog_id'))


@_serialize_client_directive('TypeTextDirective')
def _serialize_type_text(payload, type_text):
    type_text.Text = payload.get('text')


@_serialize_client_directive('TypeTextSilentDirective')
def _serialize_type_text_silent(payload, type_text_silent):
    type_text_silent.Text = payload.get('text')


@_serialize_client_directive('CloseDialogDirective')
def _serialize_close_dialog(payload, close_dialog_directive):
    close_dialog_directive.DialogId = _smart_dialog_id(payload.get('dialog_id'))


@_serialize_client_directive('ClearQueueDirective')
def _serialize_clear_queue(payload, clear_queue_directive):
    pass


@_serialize_client_directive('StartImageRecognizerDirective')
def _serialize_start_image_recognizer(payload, image_recognizer):
    mapping = (
        ('camera_type', 'CameraType'),
        ('image_search_mode', 'ImageSearchMode'),
        ('image_search_mode_name', 'ImageSearchModeName'),
    )
    for key, field in mapping:
        value = payload.get(key)
        if value is not None:
            setattr(image_recognizer, field, value)


@_serialize_client_directive('SoundSetLevelDirective')
def _serialize_sound_set_level(payload, sound_set_level):
    sound_set_level.NewLevel = payload.get('new_level')


@_serialize_client_directive('CarDirective')
def _serialize_car(payload, car):
    car.Application = payload.get('application')
    car.Intent = payload.get('intent')
    json_format.ParseDict(payload.get('params') or {}, car.Params)


@_serialize_client_directive('YandexNaviDirective')
def _serialize_yandexnavi(payload, yandexnavi):
    yandexnavi.Application = payload.get('application')
    yandexnavi.Intent = payload.get('intent')
    json_format.ParseDict(payload.get('params') or {}, yandexnavi.Params)


@_serialize_client_directive('PlayerNextTrackDirective')
def _serialize_player_next_track(payload, player_next_track):
    player_next_track.Player = payload.get('player') or ''
    player_next_track.Uid = payload.get('uid') or ''
    player_next_track.MultiroomSessionId = payload.get('multiroom_session_id') or ''


@_serialize_client_directive('PlayerPreviousTrackDirective')
def _serialize_player_previous_track(payload, player_previous_track):
    player_previous_track.Player = payload.get('player') or ''
    player_previous_track.MultiroomSessionId = payload.get('multiroom_session_id') or ''


@_serialize_client_directive('PlayerPauseDirective')
def _serialize_player_pause(payload, player_pause):
    player_pause.Smooth = payload.get('smooth') or False


@_serialize_client_directive('PlayerContinueDirective')
def _serialize_player_continue(payload, player_continue):
    player_continue.Player = payload.get('player') or ''


@_serialize_client_directive('PlayerLikeDirective')
def _serialize_player_like(payload, player_like):
    player_like.Uid = payload.get('uid') or ''


@_serialize_client_directive('PlayerDislikeDirective')
def _serialize_player_dislike(payload, player_dislike):
    player_dislike.Uid = payload.get('uid') or ''


@_serialize_client_directive('PlayerShuffleDirective')
def _serialize_player_shuffle(payload, player_shuffle):
    pass


@_serialize_client_directive('PlayerOrderDirective')
def _serialize_player_order(payload, player_order):
    pass


@_serialize_client_directive('PlayerReplayDirective')
def _serialize_player_replay(payload, player_replay):
    pass


@_serialize_client_directive('PlayerRepeatDirective')
def _serialize_player_repeat(payload, player_repeat):
    pass


@_serialize_client_directive('PlayerRewindDirective')
def _serialize_player_rewind(payload, player_rewind):
    player_rewind.Amount = payload.get('amount') or 0
    player_rewind.Type = _rewind_type_to_enum(payload.get('type'))


@_serialize_client_directive('SoundQuiterDirective')
def _serialize_sound_quiter(payload, sound_quiter):
    pass


@_serialize_client_directive('SoundLouderDirective')
def _serialize_sound_louder(payload, sound_louder):
    pass


@_serialize_client_directive('SoundMuteDirective')
def _serialize_sound_mute(payload, sound_mute):
    pass


@_serialize_client_directive('SoundUnmuteDirective')
def _serialize_sound_unmute(payload, sound_unmute):
    pass


@_serialize_client_directive('StartMusicRecognizerDirective')
def _serialize_start_music_recognizer(payload, start_music_recognizer):
    pass


@_serialize_client_directive('AlarmSetSoundDirective')
def _serialize_alarm_set_sound(payload, alarm_set_sound):
    callback = payload.get('server_action')
    if callback:
        alarm_set_sound.Callback.CopyFrom(_serialize_callback(ActionDirective.from_dict(callback)).CallbackDirective)
    json_format.ParseDict(payload.get('sound_alarm_setting') or {}, alarm_set_sound.Settings, ignore_unknown_fields=True)


@_serialize_client_directive('AlarmResetSoundDirective')
def _serialize_alarm_reset_sound(payload, alarm_reset_sound):
    pass


@_serialize_client_directive('AlarmNewDirective')
def _serialize_alarm_new(payload, alarm_new):
    alarm_new.State = payload.get('state') or ''
    json_format.ParseDict(payload.get('on_success') or {}, alarm_new.OnSuccessCallbackPayload)
    json_format.ParseDict(payload.get('on_fail') or {}, alarm_new.OnFailureCallbackPayload)


@_serialize_client_directive('ShowAlarmsDirective')
def _serialize_show_alarms(payload, show_alarms):
    pass


@_serialize_client_directive('ShowTimersDirective')
def _serialize_show_timers(payload, show_timers):
    pass


@_serialize_client_directive('FindContactsDirective')
def _serialize_find_contacts(payload, find_contacts):
    mime_type = payload.get('mimetypes_whitelist') or {}
    mime = find_contacts.MimeTypesWhitelist
    for item in mime_type.get('column') or []:
        mime.Column.append(item)
    for item in mime_type.get('name') or []:
        mime.Name.append(item)

    json_format.ParseDict(payload.get('on_permission_denied_payload') or {},
                          find_contacts.OnPermissionDeniedCallbackPayload)

    for part in payload.get('request') or []:
        request_part = find_contacts.Request.add()
        request_part.Tag = part.get('tag')
        for item in payload.get('values'):
            request_part.Values.append(item)

    for value in payload.get('values') or []:
        find_contacts.Values.append(value)

    find_contacts.CallbackName = payload.get('form') or ''


@_serialize_client_directive('SetSearchFilterDirective')
def _serialize_set_search_filter(payload, set_search_filter):
    set_search_filter.Level = _search_level_to_enum(payload.get('new_level'))


@_serialize_client_directive('AlarmsUpdateDirective')
def _serialize_alarms_update(payload, alarms_update):
    alarms_update.State = payload.get('state') or ''
    alarms_update.ListeningIsPossible = payload.get('listening_is_possible') or False


@_serialize_client_directive('ResumeTimerDirective')
def _serialize_resume_timer(payload, resume_timer):
    resume_timer.TimerId = payload.get('timer_id')


@_serialize_client_directive('CancelTimerDirective')
def _serialize_cancel_timer(payload, cancel_timer):
    cancel_timer.TimerId = payload.get('timer_id')


@_serialize_client_directive('SetTimerDirective')
def _serialize_set_timer(payload, set_timer):
    set_timer.Duration = payload.get('duration') or 0
    set_timer.ListeningIsPossible = payload.get('listening_is_possible') or False
    set_timer.Timestamp = payload.get('timestamp') or 0
    on_success = payload.get('on_success')
    if on_success is not None:
        json_format.ParseDict(on_success, set_timer.OnSuccessCallbackPayload)
    on_fail = payload.get('on_fail')
    if on_fail is not None:
        json_format.ParseDict(on_fail, set_timer.OnFailureCallbackPayload)
    for inner_directive in payload.get('directives') or []:
        set_timer.Directives.add().CopyFrom(serialize_directive(inner_directive))


def _common_reminders_serialize(payload, directive):
    on_success_cb = payload.get('on_success_callback')
    if on_success_cb is not None:
        directive.OnSuccessCallback.CopyFrom(serialize_directive(on_success_cb).CallbackDirective)

    on_fail_cb = payload.get('on_fail_callback')
    if on_fail_cb is not None:
        directive.OnFailCallback.CopyFrom(serialize_directive(on_fail_cb).CallbackDirective)


@_serialize_client_directive('RemindersSetDirective')
def _serialize_reminders_set_directive(payload, directive):
    _common_reminders_serialize(payload, directive)

    directive.Id = payload.get('id') or ''
    directive.Text = payload.get('text') or ''
    directive.Epoch = payload.get('epoch') or ''
    directive.TimeZone = payload.get('timezone') or ''

    on_shoot_frame = payload.get('on_shoot_frame')
    if on_shoot_frame is not None:
        json_format.ParseDict(on_shoot_frame, directive.OnShootFrame)


@_serialize_client_directive('RequestPermissionsDirective')
def _serialize_request_permission_directive(payload, directive):
    directive.Name = payload.get('name') or ''
    directive.Permissions.extend(payload.get('permissions') or [])

    on_success_cb = payload.get('on_success')
    if on_success_cb:
        directive.OnSuccess.CallbackDirective.ParseFromString(base64.b64decode(on_success_cb))

    on_fail_cb = payload.get('on_fail')
    if on_fail_cb:
        directive.OnFail.CallbackDirective.ParseFromString(base64.b64decode(on_fail_cb))


@_serialize_client_directive('RemindersCancelDirective')
def _serialize_reminders_cancel_directive(payload, directive):
    _common_reminders_serialize(payload, directive)
    directive.Action = payload.get('action') or ''
    for id in payload.get('id') or []:
        directive.Ids.append(id)


@_serialize_client_directive('StopBluetoothDirective')
def _serialize_stop_bluetooth(payload, stop_bluetooth):
    pass


@_serialize_client_directive('StartBluetoothDirective')
def _serialize_start_bluetooth(payload, start_bluetooth):
    pass


@_serialize_client_directive('ReadPageDirective')
def _serialize_read_page(payload, read_page):
    pass


@_serialize_client_directive('ReadPagePauseDirective')
def _serialize_read_page_pause(payload, read_page_pause):
    pass


@_serialize_client_directive('ReadPageContinueDirective')
def _serialize_read_page_continue(payload, read_page_continue):
    pass


@_serialize_client_directive('MusicPlayDirective')
def _serialize_music_play(payload, music_play):
    music_play.Uid = payload.get('uid') or ''
    music_play.SessionId = payload.get('session_id') or ''
    music_play.Offset = payload.get('offset') or 0
    music_play.AlarmId = payload.get('alarm_id') or ''
    music_play.FirstTrackId = payload.get('first_track_id') or ''


@_serialize_client_directive('SearchLocalDirective')
def _serialize_search_local(payload, search_local):
    search_local.Text = payload.get('text') or ''


@_serialize_client_directive('OpenFolderDirective')
def _serialize_open_folder(payload, open_folder):
    open_folder.Folder = payload.get('folder') or ''


@_serialize_client_directive('OpenFileDirective')
def _serialize_open_file(payload, open_file):
    open_file.File = payload.get('file') or ''


@_serialize_client_directive('OpenSoftDirective')
def _serialize_open_soft(payload, open_soft):
    open_soft.Soft = payload.get('soft') or ''


@_serialize_client_directive('PowerOffDirective')
def _serialize_power_off(payload, power_off):
    pass


@_serialize_client_directive('HibernateDirective')
def _serialize_hibernate(payload, hibernate):
    pass


@_serialize_client_directive('RestartPcDirective')
def _serialize_restart_pc(payload, restart_pc):
    pass


@_serialize_client_directive('MuteDirective')
def _serialize_mute(payload, mute):
    pass


@_serialize_client_directive('UnmuteDirective')
def _serialize_unmute(payload, unmute):
    pass


@_serialize_client_directive('OpenDefaultBrowserDirective')
def _serialize_open_default_browser(payload, open_default_browser):
    pass


@_serialize_client_directive('OpenYaBrowserDirective')
def _serialize_open_ya_browser(payload, open_ya_browser):
    pass


@_serialize_client_directive('OpenFlashCardDirective')
def _serialize_open_flash_card(payload, open_flash_card):
    pass


@_serialize_client_directive('OpenStartDirective')
def _serialize_open_start(payload, open_start):
    pass


@_serialize_client_directive('OpenDiskDirective')
def _serialize_open_disk(payload, open_disk):
    open_disk.Disk = payload.get('disk') or ''


@_serialize_client_directive('OpenSettingsDirective')
def _serialize_open_settings(payload, open_settings):
    open_settings.Target = _settings_target_to_enum(payload.get('target'))


@_serialize_client_directive('GoForwardDirective')
def _serialize_go_forward(payload, go_forward):
    pass


@_serialize_client_directive('GoBackwardDirective')
def _serialize_go_backward(payload, go_backward):
    pass


@_serialize_client_directive('GoToTheBeginningDirective')
def _serialize_go_to_the_beginning(payload, go_to_the_beginning):
    pass


@_serialize_client_directive('GoToTheEndDirective')
def _serialize_go_to_the_end(payload, go_to_the_end):
    pass


@_serialize_client_directive('GoHomeDirective')
def _serialize_go_home(payload, go_home):
    pass


@_serialize_client_directive('PauseTimerDirective')
def _serialize_pause_timer(payload, pause_timer):
    pause_timer.TimerId = payload.get('timer_id') or ''


@_serialize_client_directive('TimerStopPlayingDirective')
def _serialize_timer_stop_playing(payload, timer_stop_playing):
    timer_stop_playing.TimerId = payload.get('timer_id') or ''


@_serialize_client_directive('NavigateBrowserDirective')
def _serialize_navigate_browser(payload, navigate_browser):
    navigate_browser.Command = _browser_command_to_enum(payload.get('command_name'))


@_serialize_client_directive('RadioPlayDirective')
def _serialize_radio_play(payload, radio_play):
    radio_play.IsActive = payload.get('active') or False
    radio_play.IsAvailable = payload.get('available') or False
    radio_play.Color = payload.get('color') or ''
    radio_play.Frequency = payload.get('frequency') or ''
    radio_play.ImageUrl = payload.get('imageUrl') or ''
    radio_play.OfficialSiteUrl = payload.get('officialSiteUrl') or ''
    radio_play.RadioId = payload.get('radioId') or ''
    radio_play.Score = payload.get('score') or 0.0
    radio_play.ShowRecognition = payload.get('showRecognition') or False
    radio_play.StreamUrl = payload.get('streamUrl') or ''
    radio_play.Title = payload.get('title') or ''
    alarm_id = payload.get('alarm_id')
    if alarm_id:
        radio_play.AlarmId = alarm_id


@_serialize_client_directive('GoDownDirective')
def _serialize_go_down(payload, go_down):
    pass


@_serialize_client_directive('GoTopDirective')
def _serialize_go_top(payload, go_top):
    pass


@_serialize_client_directive('GoUpDirective')
def _serialize_go_up(payload, go_up):
    pass


@_serialize_client_directive('ScreenOffDirective')
def _serialize_screen_off(payload, screen_off):
    pass


@_serialize_client_directive('ShowTvGalleryDirective')
def _serialize_show_tv_gallery(payload, show_tv_gallery):
    for item in payload.get('items', []):
        json_format.ParseDict(item, show_tv_gallery.Items.add(), ignore_unknown_fields=True)


@_serialize_client_directive('ShowGalleryDirective')
def _serialize_show_gallery(payload, show_gallery):
    json_format.ParseDict(payload, show_gallery, ignore_unknown_fields=True)


@_serialize_client_directive('AlarmStopDirective')
def _serialize_alarm_stop(payload, alarm_stop):
    pass


@_serialize_client_directive('AlarmSetMaxLevelDirective')
def _serialize_alarm_set_max_level(payload, alarm_set_max_level):
    alarm_set_max_level.NewLevel = payload.get('new_level') or 0


@_serialize_client_directive('MusicRecognitionDirective')
def _serialize_music_recognition(payload, music_recognition):
    music_recognition.Uri = payload.get('uri') or ''
    music_recognition.Type = payload.get('type') or ''
    music_recognition.Title = payload.get('title') or ''
    music_recognition.Subtype = payload.get('subtype') or ''
    music_recognition.Id = payload.get('id') or ''
    music_recognition.CoverUri = payload.get('coverUri') or ''

    album = payload.get('album') or {}
    if album:
        music_recognition.Album.Genre = album.get('genre') or ''
        music_recognition.Album.Id = album.get('id') or ''
        music_recognition.Album.Title = album.get('title') or ''

    for item in payload.get('artists') or []:
        artist = music_recognition.Artists.add()
        artist.Composer = item.get('composer') or False
        artist.Id = item.get('id') or ''
        artist.IsVarious = item.get('is_various') or False
        artist.Name = item.get('name') or ''


@_serialize_client_directive('VideoPlayDirective')
def _serialize_video_play(payload, music_recognition):
    json_format.ParseDict(payload, music_recognition, ignore_unknown_fields=True)


@_serialize_client_directive('ShowPayPushScreenDirective')
def _serialize_show_pay_push_screen(payload, show_pay_push_screen):
    json_format.ParseDict(payload, show_pay_push_screen, ignore_unknown_fields=True)


@_serialize_client_directive('ShowVideoDescriptionDirective')
def _serialize_show_description(payload, show_description):
    json_format.ParseDict(payload, show_description, ignore_unknown_fields=True)


@_serialize_client_directive('ShowSeasonGalleryDirective')
def _serialize_show_season_gallery(payload, show_season_gallery):
    json_format.ParseDict(payload, show_season_gallery, ignore_unknown_fields=True)


def _serialize_save_voiceprint(uniproxy_action_directive):
    proto = TDirective()
    directive = proto.SaveVoiceprintDirective
    payload = uniproxy_action_directive.payload or {}

    directive.UserId = payload.get('user_id') or ''
    for request_id in payload.get('requests') or []:
        directive.RequestIds.append(request_id)

    return proto


def _serialize_remove_voiceprint(uniproxy_action_directive):
    proto = TDirective()
    directive = proto.RemoveVoiceprintDirective
    payload = uniproxy_action_directive.payload or {}

    directive.UserId = payload.get('user_id') or ''

    return proto


@_serialize_client_directive('SendBugReportDirective')
def _serialize_send_bug_report(payload, send_bug_report):
    send_bug_report.RequestId = payload.get('id') or ''


@_serialize_client_directive('MordoviaShowDirective')
def _serialize_mordovia_show(payload, mordovia_show):
    mordovia_show.Url = payload.get('url') or ''
    mordovia_show.IsFullScreen = payload.get('is_full_screen') or False
    mordovia_show.ViewKey = payload.get('view_key', payload.get('scenario')) or ''
    mordovia_show.SplashDiv = payload.get('splash_div') or ''
    callback = payload.get('callback_prototype')
    if callback:
        mordovia_show.CallbackPrototype.CopyFrom(_serialize_callback(
            ActionDirective.from_dict(callback)).CallbackDirective)


@_serialize_client_directive('MordoviaCommandDirective')
def _serialize_mordovia_command(payload, mordovia_command):
    mordovia_command.Command = payload.get('command') or ''
    mordovia_command.ViewKey = payload.get('view_key') or ''
    meta = payload.get('meta')
    if meta:
        json_format.ParseDict(meta, mordovia_command.Meta)


@_serialize_client_directive('TtsPlayPlaceholderDirective')
def _serialize_tts_play_placeholder(payload, tts_play_placeholder):
    pass


@_serialize_client_directive('DrawLedScreenDirective')
def _serialize_draw_led_screen(payload, draw_led_screen):
    for item in payload.get('animation_sequence') or []:
        drawItem = draw_led_screen.DrawItem.add()
        drawItem.FrontalLedImage = item.get('frontal_led_image') or ''
        drawItem.Endless = item.get('endless') or False
    draw_led_screen.TillEndOfSpeech = payload.get('till_end_of_speech') or False


@_serialize_client_directive('DrawScledAnimationsDirective')
def _serialize_draw_scled_animations(payload, draw_scled_animations):
    for item in payload.get('animations') or []:
        animationItem = draw_scled_animations.Animations.add()
        animationItem.Name = item.get('name') or ''
        animationItem.Compression = _compression_type_to_enum(item.get('compression_type'))
        animationItem.Base64EncodedValue = item.get('base64_encoded_value') or ''

    draw_scled_animations.AnimationStopPolicy = _animation_stop_policy_to_enum(payload.get('animation_stop_policy'))


@_serialize_client_directive('ForceDisplayCardsDirective')
def _serialize_force_display_cards(payload, force_display_cards):
    pass


@_serialize_client_directive('ShowButtonsDirective')
def _serialize_show_buttons(payload, show_buttons):
    json_format.ParseDict(payload, show_buttons, ignore_unknown_fields=True)


@_serialize_client_directive('FillCloudUiDirective')
def _serialize_fill_cloud_ui(payload, fill_cloud_ui):
    json_format.ParseDict(payload, fill_cloud_ui, ignore_unknown_fields=True)


@_serialize_client_directive('WebOSLaunchAppDirective')
def _serialize_web_os_launch_app(payload, web_os_launch_app):
    web_os_launch_app.AppId = payload.get('appId')
    web_os_launch_app.ParamsJson = json.dumps(payload.get('params')).encode('utf-8')


def _serialize_uniproxy_directive(uniproxy_action_directive):
    if uniproxy_action_directive.name == 'update_datasync':
        logging.warning('trying to serialize datasync directive: %s', json.dumps(uniproxy_action_directive.to_dict()))
        return None

    if uniproxy_action_directive.name == 'save_voiceprint':
        return _serialize_save_voiceprint(uniproxy_action_directive)

    if uniproxy_action_directive.name == 'remove_voiceprint':
        return _serialize_remove_voiceprint(uniproxy_action_directive)

    raise TypeError('Unable to serialize uniproxy directive {}'.format(json.dumps(uniproxy_action_directive.to_dict())))


def _serialize_update_datasync_directive(uniproxy_action_directive):
    proto = TServerDirective()
    directive = proto.UpdateDatasyncDirective
    payload = uniproxy_action_directive.payload or {}

    directive.Key = payload.get('key') or ''
    value = payload.get('value')
    if isinstance(value, six.string_types):
        directive.StringValue = value
    elif isinstance(value, dict) or value is None:
        directive.StructValue.CopyFrom(dict_to_struct(value))
    else:
        raise TypeError('Unsupported value type %s for UpdateDatasyncDirective' % type(value))

    method = payload.get('method') or 'PUT'
    if method != 'PUT':
        raise TypeError('Unsupported method %s for UpdateDatasyncDirective: ' % method)
    directive.Method = TUpdateDatasyncDirective.EDataSyncMethod.Put

    return proto


def _serialize_memento_change_user_objects_directive(uniproxy_action_directive):
    proto = TServerDirective()
    proto.MementoChangeUserObjectsDirective.ParseFromString(base64.b64decode(uniproxy_action_directive.payload["protobuf"]))
    return proto


def _serialize_raw_server_directive(uniproxy_action_directive):
    proto = TServerDirective()
    proto.ParseFromString(base64.b64decode(uniproxy_action_directive.payload["protobuf"]))
    return proto


def _serialize_send_push_directive(send_push_directive):
    proto = TServerDirective()
    send_push = proto.SendPushDirective
    payload = send_push_directive.payload or {}

    def _fill_common_params(common, payload, prefix):
        common.Title = payload.get(prefix + 'title') or ''
        common.Text = payload.get(prefix + 'text') or ''
        common.Link = payload.get(prefix + 'url') or ''
        common.TtlSeconds = payload.get(prefix + 'ttl') or 0

    # common settings
    _fill_common_params(send_push.Settings, payload, '')
    # push message
    send_push.PushMessage.ThrottlePolicy = payload.get('throttle') or ''
    send_push.PushMessage.AppTypes.append(EAppType.AT_SEARCH_APP)
    _fill_common_params(send_push.PushMessage.Settings, payload, 'push_')
    # personal card
    send_push.PersonalCard.ImageUrl = payload.get('personal_card_image_url') or ''
    min_price = payload.get('min_price') or 0
    if min_price:
        send_push.PersonalCard.YandexStationFilmData.MinPrice = min_price
    _fill_common_params(send_push.PersonalCard.Settings, payload, 'personal_card_')
    # misc
    send_push.PushId = payload.get('id') or ''
    send_push.PushTag = payload.get('tag') or ''
    send_push.RemoveExistingCards = payload.get('remove_existing_cards') or False

    return proto


def _serialize_uniproxy_directive_as_server_directive(uniproxy_action_directive):
    if uniproxy_action_directive.name == 'update_datasync':
        return _serialize_update_datasync_directive(uniproxy_action_directive)
    if uniproxy_action_directive.name == 'send_push':
        return _serialize_send_push_directive(uniproxy_action_directive)
    if uniproxy_action_directive.name == 'memento_change_user_objects_directive':
        return _serialize_memento_change_user_objects_directive(uniproxy_action_directive)
    if uniproxy_action_directive.name == 'raw_server_directive':
        return _serialize_raw_server_directive(uniproxy_action_directive)
    return None


def serialize_directive(directive):
    if isinstance(directive, dict):
        directive = ActionDirective.from_dict(directive)
    if not isinstance(directive, ActionDirective):
        raise TypeError('Invalid type of directive: got {} but expected ActionDirective'.format(type(directive)))
    if isinstance(directive, ClientActionDirective) and _has_client_action_serializer(directive.name):
        return _get_client_action_serializer(directive.name)(directive)
    elif isinstance(directive, ServerActionDirective):
        return _serialize_callback(directive)
    elif isinstance(directive, UniproxyActionDirective):
        return _serialize_uniproxy_directive(directive)
    elif isinstance(directive, TypedSemanticFrameDirective):
        return _serialize_typed_semantic_frame_directive(directive)
    else:
        raise TypeError('Unable to serialize directive {} of type {}'.format(directive.name, directive.type))


def serialize_server_directive(directive):
    if isinstance(directive, UniproxyActionDirective):
        return _serialize_uniproxy_directive_as_server_directive(directive)
    if isinstance(directive, ClientActionDirective) and directive.name == 'send_push':
        return _serialize_send_push_directive(directive)
    return None


_register_client_action_serializer(_serialize_alarm_set_max_level, 'alarm_set_max_level')
_register_client_action_serializer(_serialize_alarm_new, 'alarm_new')
_register_client_action_serializer(_serialize_alarm_reset_sound, 'alarm_reset_sound')
_register_client_action_serializer(_serialize_alarm_set_sound, 'alarm_set_sound')
_register_client_action_serializer(_serialize_alarm_stop, 'alarm_stop')
_register_client_action_serializer(_serialize_alarms_update, 'alarms_update')
_register_client_action_serializer(_serialize_cancel_timer, 'cancel_timer')
_register_client_action_serializer(_serialize_car, 'car')
_register_client_action_serializer(_serialize_clear_queue, 'clear_queue')
_register_client_action_serializer(_serialize_close_dialog, 'close_dialog')
_register_client_action_serializer(_serialize_end_dialog_session, 'end_dialog_session')
_register_client_action_serializer(_serialize_find_contacts, 'find_contacts')
_register_client_action_serializer(_serialize_go_backward, 'go_backward')
_register_client_action_serializer(_serialize_go_down, 'go_down')
_register_client_action_serializer(_serialize_go_forward, 'go_forward')
_register_client_action_serializer(_serialize_go_home, 'go_home')
_register_client_action_serializer(_serialize_go_to_the_beginning, 'go_to_the_beginning')
_register_client_action_serializer(_serialize_go_to_the_end, 'go_to_the_end')
_register_client_action_serializer(_serialize_go_top, 'go_top')
_register_client_action_serializer(_serialize_go_up, 'go_up')
_register_client_action_serializer(_serialize_hibernate, 'hibernate')
_register_client_action_serializer(_serialize_mordovia_command, 'mordovia_command')
_register_client_action_serializer(_serialize_mordovia_show, 'mordovia_show')
_register_client_action_serializer(_serialize_music_play, 'music_play')
_register_client_action_serializer(_serialize_music_recognition, 'music_recognition')
_register_client_action_serializer(_serialize_mute, 'mute')
_register_client_action_serializer(_serialize_navigate_browser, 'navigate_browser')
_register_client_action_serializer(_serialize_open_default_browser, 'open_default_browser')
_register_client_action_serializer(_serialize_open_dialog, 'open_dialog')
_register_client_action_serializer(_serialize_open_disk, 'open_disk')
_register_client_action_serializer(_serialize_open_file, 'open_file')
_register_client_action_serializer(_serialize_open_flash_card, 'open_flash_card')
_register_client_action_serializer(_serialize_open_folder, 'open_folder')
_register_client_action_serializer(_serialize_open_settings, 'open_settings')
_register_client_action_serializer(_serialize_open_soft, 'open_soft')
_register_client_action_serializer(_serialize_open_start, 'open_start')
_register_client_action_serializer(_serialize_open_uri, 'open_uri')
_register_client_action_serializer(_serialize_open_ya_browser, 'open_ya_browser')
_register_client_action_serializer(_serialize_pause_timer, 'pause_timer')
_register_client_action_serializer(_serialize_player_continue, 'player_continue')
_register_client_action_serializer(_serialize_player_dislike, 'player_dislike')
_register_client_action_serializer(_serialize_player_like, 'player_like')
_register_client_action_serializer(_serialize_player_next_track, 'player_next_track')
_register_client_action_serializer(_serialize_player_order, 'player_order')
_register_client_action_serializer(_serialize_player_pause, 'player_pause')
_register_client_action_serializer(_serialize_player_previous_track, 'player_previous_track')
_register_client_action_serializer(_serialize_player_repeat, 'player_repeat')
_register_client_action_serializer(_serialize_player_replay, 'player_replay')
_register_client_action_serializer(_serialize_player_rewind, 'player_rewind')
_register_client_action_serializer(_serialize_player_shuffle, 'player_shuffle')
_register_client_action_serializer(_serialize_power_off, 'power_off')
_register_client_action_serializer(_serialize_radio_play, 'radio_play')
_register_client_action_serializer(_serialize_read_page, 'read_page')
_register_client_action_serializer(_serialize_read_page_continue, 'read_page_continue')
_register_client_action_serializer(_serialize_read_page_pause, 'read_page_pause')
_register_client_action_serializer(_serialize_restart_pc, 'restart_pc')
_register_client_action_serializer(_serialize_resume_timer, 'resume_timer')
_register_client_action_serializer(_serialize_screen_off, 'screen_off')
_register_client_action_serializer(_serialize_search_local, 'search_local')
_register_client_action_serializer(_serialize_send_bug_report, 'send_bug_report')
_register_client_action_serializer(_serialize_set_search_filter, 'set_search_filter')
_register_client_action_serializer(_serialize_set_timer, 'set_timer')
_register_client_action_serializer(_serialize_reminders_set_directive, 'reminders_set_directive')
_register_client_action_serializer(_serialize_reminders_cancel_directive, 'reminders_cancel_directive')
_register_client_action_serializer(_serialize_request_permission_directive, 'request_permissions')
_register_client_action_serializer(_serialize_show_alarms, 'show_alarms')
_register_client_action_serializer(_serialize_show_description, 'show_description')
_register_client_action_serializer(_serialize_show_pay_push_screen, 'show_pay_push_screen')
_register_client_action_serializer(_serialize_show_promo, 'show_promo')
_register_client_action_serializer(_serialize_show_season_gallery, 'show_season_gallery')
_register_client_action_serializer(_serialize_show_timers, 'show_timers')
_register_client_action_serializer(_serialize_show_tv_gallery, 'show_tv_gallery')
_register_client_action_serializer(_serialize_show_gallery, 'show_gallery')
_register_client_action_serializer(_serialize_sound_louder, 'sound_louder')
_register_client_action_serializer(_serialize_sound_mute, 'sound_mute')
_register_client_action_serializer(_serialize_sound_quiter, 'sound_quiter')
_register_client_action_serializer(_serialize_sound_set_level, 'sound_set_level')
_register_client_action_serializer(_serialize_sound_unmute, 'sound_unmute')
_register_client_action_serializer(_serialize_start_bluetooth, 'start_bluetooth')
_register_client_action_serializer(_serialize_start_image_recognizer, 'start_image_recognizer')
_register_client_action_serializer(_serialize_start_music_recognizer, 'start_music_recognizer')
_register_client_action_serializer(_serialize_stop_bluetooth, 'stop_bluetooth')
_register_client_action_serializer(_serialize_timer_stop_playing, 'timer_stop_playing')
_register_client_action_serializer(_serialize_type_text, 'type')
_register_client_action_serializer(_serialize_type_text_silent, 'type_silent')
_register_client_action_serializer(_serialize_unmute, 'unmute')
_register_client_action_serializer(_serialize_update_dialog_info, 'update_dialog_info')
_register_client_action_serializer(_serialize_video_play, 'video_play')
_register_client_action_serializer(_serialize_yandexnavi, 'yandexnavi')
_register_client_action_serializer(_serialize_tts_play_placeholder, 'tts_play_placeholder')
_register_client_action_serializer(_serialize_draw_led_screen, 'draw_led_screen')
_register_client_action_serializer(_serialize_draw_scled_animations, 'draw_scled_animations')
_register_client_action_serializer(_serialize_force_display_cards, 'force_display_cards')
_register_client_action_serializer(_serialize_show_buttons, 'show_buttons')
_register_client_action_serializer(_serialize_fill_cloud_ui, 'fill_cloud_ui')
_register_client_action_serializer(_serialize_web_os_launch_app, 'web_os_launch_app')
