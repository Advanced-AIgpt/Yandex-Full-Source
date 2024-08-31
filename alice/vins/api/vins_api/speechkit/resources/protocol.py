# coding:utf-8

from __future__ import unicode_literals

import base64
import falcon
import json
import logging
import os
import random
import urllib

from uuid import UUID, uuid4

from google.protobuf import struct_pb2
from google.protobuf.json_format import ParseDict

from ylog.context import LogContext

from alice.protos.data.language.language_pb2 import ELang
from alice.megamind.protos.analytics.scenarios.vins.vins_pb2 import TVinsGcMeta
from alice.megamind.protos.common.data_source_type_pb2 import (
    VINS_WIZARD_RULES, ENTITY_SEARCH, BEGEMOT_EXTERNAL_MARKUP, BLACK_BOX, QUASAR_DEVICES_INFO
)
from alice.megamind.protos.scenarios.request_pb2 import TScenarioApplyRequest, TScenarioRunRequest, TInput
from alice.megamind.protos.scenarios.response_pb2 import (
    TScenarioApplyResponse, TScenarioCommitResponse, TScenarioContinueResponse, TScenarioRunResponse, TLayout
)
from alice.vins.api.vins_api.speechkit.protos.vins_response_pb2 import TVinsApplyResponse, TVinsRunResponse
from alice.vins.api_helper.resources import ValidationError

from library.langs import langs
from library.python.svn_version import svn_last_revision, svn_branch

from personal_assistant.intents import (
    is_player_intent, is_quasar_gallery_navigation, is_quasar_screen_navigation, is_quasar_video_details_opening_action,
    skip_relevant_intents,
)
from personal_assistant.meta import RepeatMeta, GeneralConversationSourceMeta
from personal_assistant.testing_framework import TEST_REQUEST_ID

from vins_api.common.resources import BaseConnectedAppResource
from vins_api.speechkit.connectors.protocol.directives import (
    serialize_directive, serialize_server_directive, SERVER_DIRECTIVE_NAMES
)
from vins_api.speechkit.connectors.protocol.utils import Headers, create_state, unpack_state
from vins_api.speechkit.resources.common import get_device_id, client_time_from_timestamp
from vins_api.speechkit.session import SKSessionStorage
from vins_core.dm.request import AppInfo, Experiments, ReqInfo
from vins_core.dm.request_events import (
    TextInputEvent, SuggestedInputEvent, RequestEvent, VoiceInputEvent, ImageInputEvent, MusicInputEvent,
    ServerActionEvent,
)
from vins_core.dm.response import (
    CardWithButtons, DivCard, Div2Card, SimpleCard, Button, ActionButton, ThemedActionButton, FormInfoFeatures,
    FormRestoredMeta, ErrorMeta, ServerActionDirective
)
from vins_core.utils.config import get_setting
from vins_core.utils.datetime import datetime_to_timestamp
from vins_core.utils.json_util import MessageToDict, dict_to_struct
from vins_core.utils.metrics import sensors
from vins_core.utils.misc import str_to_bool
from vins_core.utils.strings import smart_unicode
from vins_core.nlg.filters import trim_with_ellipsis

logger = logging.getLogger(__name__)

ADD_CONTACT_BOOK_ASR_DIRECTIVE_NAME = 'add_contact_book_asr'

_FORBIDDEN_DIRECTIVE_NAMES = {
    'on_suggest',
    ADD_CONTACT_BOOK_ASR_DIRECTIVE_NAME,
    'on_external_button',
}

MILLISECONDS = 1000
SECONDS_IN_A_WEEK = 7 * 24 * 60 * 60

PLAYER_FRAMES_PREFIX = 'personal_assistant.scenarios.player'
PLAYER_FRAME_WHAT_IS_PLAYING_OUTER = 'personal_assistant.scenarios.player.what_is_playing'
PLAYER_FRAME_WHAT_IS_PLAYING_VINS = 'personal_assistant.scenarios.music_what_is_playing'


def normalize_strings(message):
    if isinstance(message, (str, unicode)):
        return smart_unicode(message)
    if isinstance(message, dict):
        return {normalize_strings(k): normalize_strings(v) for k, v in message.items()}
    if isinstance(message, list):
        return [normalize_strings(e) for e in message]
    return message


def normalize_strings_and_numbers(message):
    if isinstance(message, (str, unicode)):
        return smart_unicode(message)
    if isinstance(message, float) and message.is_integer():
        return int(message)
    if isinstance(message, dict):
        return {normalize_strings(k): normalize_strings_and_numbers(v) for k, v in message.items()}
    if isinstance(message, list):
        return [normalize_strings_and_numbers(e) for e in message]
    return message


def message_as_dict(message, normalize_numbers=False):
    dict_message = MessageToDict(message, including_default_value_fields=True)
    if normalize_numbers:
        return normalize_strings_and_numbers(dict_message)
    return normalize_strings(dict_message)


def get_biometry_scoring(biometry_scoring_message):
    biometry_scoring = message_as_dict(biometry_scoring_message)
    return biometry_scoring or None


def get_biometry_classification(biometry_classification_message):
    return message_as_dict(biometry_classification_message) or None


IMAGE_CAPTURE_MODE = {
    TInput.TImage.ECaptureMode.OcrVoice: 'voice_text',
    TInput.TImage.ECaptureMode.Ocr: 'text',
    TInput.TImage.ECaptureMode.Photo: 'photo',
    TInput.TImage.ECaptureMode.Market: 'market',
    TInput.TImage.ECaptureMode.Document: 'document',
    TInput.TImage.ECaptureMode.Clothes: 'clothes',
    TInput.TImage.ECaptureMode.Details: 'details',
    TInput.TImage.ECaptureMode.SimilarLike: 'similar_like',
    TInput.TImage.ECaptureMode.SimilarPeople: 'similar_people',
    TInput.TImage.ECaptureMode.SimilarPeopleFrontal: 'similar_people_frontal',
    TInput.TImage.ECaptureMode.Barcode: 'barcode',
    TInput.TImage.ECaptureMode.Translate: 'translate',
    TInput.TImage.ECaptureMode.SimilarArtwork: 'similar_artwork',
}


def get_image_event(image):
    payload = {
        'img_url': smart_unicode(image.Url)
    }
    capture_mode = IMAGE_CAPTURE_MODE.get(image.CaptureMode, None)
    if capture_mode:
        payload['capture_mode'] = capture_mode
    return ImageInputEvent(payload=payload)


def get_event(request):
    event_type = request.Input.WhichOneof('Event')
    if event_type == 'Text':
        if request.Input.Text.FromSuggest:
            return SuggestedInputEvent(text=smart_unicode(request.Input.Text.RawUtterance))
        return TextInputEvent(text=smart_unicode(request.Input.Text.RawUtterance))
    if event_type == 'Voice':
        biometry_scoring = None
        if request.Input.Voice.HasField('BiometryScoring'):
            biometry_scoring = get_biometry_scoring(request.Input.Voice.BiometryScoring)
        biometry_classification = None
        if request.Input.Voice.HasField('BiometryClassification'):
            biometry_classification = get_biometry_classification(request.Input.Voice.BiometryClassification)
        return VoiceInputEvent(
            asr_result=[message_as_dict(r) for r in request.Input.Voice.AsrData],
            biometry_scoring=biometry_scoring,
            biometry_classification=biometry_classification
        )
    if event_type == 'Image':
        return get_image_event(request.Input.Image)
    if event_type == 'Music':
        return MusicInputEvent(music_result=message_as_dict(request.Input.Music.MusicResult))
    if event_type == 'Callback':
        payload = message_as_dict(request.Input.Callback.Payload, normalize_numbers=True)
        logger.debug('Got callback payload: %s', payload)
        return ServerActionEvent(name=smart_unicode(request.Input.Callback.Name), payload=payload)
    raise ValueError('Unknown event type "{}"'.format(request.Input.WhichOneof('Event')))


IMAGE_SEARCH_SEMANTIC_FRAME_NAME = 'personal_assistant.scenarios.search.images'


def has_image_search_intent(request_input):
    return any(frame.Name == IMAGE_SEARCH_SEMANTIC_FRAME_NAME for frame in request_input.SemanticFrames)


def get_trusted_features_mapping(interfaces):
    return {
        'audio_client': interfaces.HasAudioClient,
        'audio_codec_AC3': interfaces.AudioCodecAC3,
        'audio_codec_EAC3': interfaces.AudioCodecEAC3,
        'audio_codec_OPUS': interfaces.AudioCodecOPUS,
        'audio_codec_VORBIS': interfaces.AudioCodecVORBIS,
        'battery_power_state': interfaces.HasAccessToBatteryPowerState,
        'bluetooth_player': interfaces.HasBluetoothPlayer,
        'cec_available': interfaces.HasCEC,
        'change_alarm_sound': interfaces.CanChangeAlarmSound,
        'change_alarm_sound_level': interfaces.CanChangeAlarmSoundLevel,
        'cloud_push_implementation': interfaces.HasCloudPush,
        'cloud_ui': interfaces.SupportsCloudUi,
        'cloud_ui_filling': interfaces.SupportsCloudUiFilling,
        'content_channel_alarm': interfaces.SupportsContentChannelAlarm,
        'dark_theme': interfaces.SupportsDarkTheme,
        'directive_sequencer': interfaces.HasDirectiveSequencer,
        'div2_cards': interfaces.CanRenderDiv2Cards,
        'div_cards': interfaces.CanRenderDivCards,
        'image_recognizer': interfaces.CanRecognizeImage,
        'keyboard': interfaces.CanOpenKeyboard,
        'led_display': interfaces.HasLedDisplay,
        'music_player_allow_shots': interfaces.HasMusicPlayerShots,
        'music_quasar_client': interfaces.HasMusicQuasarClient,
        'music_recognizer': interfaces.CanRecognizeMusic,
        'music_sdk_client': interfaces.HasMusicSdkClient,
        'navigator': interfaces.HasNavigator,
        'notifications': interfaces.HasNotifications,
        'open_dialogs_in_tabs': interfaces.CanOpenDialogsInTabs,
        'open_link': interfaces.CanOpenLink,
        'open_link_intent': interfaces.CanOpenLinkIntent,
        'open_link_search_viewport': interfaces.CanOpenLinkSearchViewport,
        'open_link_turboapp': interfaces.CanOpenLinkTurboApp,
        'open_yandex_auth': interfaces.CanOpenYandexAuth,
        'outgoing_phone_calls': interfaces.OutgoingPhoneCalls,
        'player_continue_directive': interfaces.SupportsPlayerContinueDirective,
        'player_dislike_directive': interfaces.SupportsPlayerDislikeDirective,
        'player_like_directive': interfaces.SupportsPlayerLikeDirective,
        'player_next_track_directive': interfaces.SupportsPlayerNextTrackDirective,
        'player_pause_directive': interfaces.SupportsPlayerPauseDirective,
        'player_previous_track_directive': interfaces.SupportsPlayerPreviousTrackDirective,
        'player_rewind_directive': interfaces.SupportsPlayerRewindDirective,
        'quasar_screen': interfaces.CanOpenQuasarScreen,
        'scled_display': interfaces.HasScledDisplay,
        'set_alarm': interfaces.CanSetAlarm,
        'set_alarm_semantic_frame_v2': interfaces.CanSetAlarmSemanticFrame,
        'show_promo': interfaces.SupportsShowPromo,
        'set_timer': interfaces.CanSetTimer,
        'show_timer': interfaces.CanShowTimer,
        'supports_device_local_reminders': interfaces.SupportsDeviceLocalReminders,
        'synchronized_push_implementation': interfaces.HasSynchronizedPush,
        'tts_play_placeholder': interfaces.TtsPlayPlaceholder,
        'vertical_screen_navigation': interfaces.SupportsVerticalScreenNavigation,
        'video_play_directive': interfaces.SupportsVideoPlayDirective,
        'video_protocol': interfaces.SupportsVideoProtocol,
    }


def get_supported_features(interfaces):
    sf = []
    usf = []

    # trusted features
    for feature, enabled in get_trusted_features_mapping(interfaces).items():
        if enabled:
            sf.append(feature)
        else:
            usf.append(feature)

    # legacy: SK-4350
    if not interfaces.HasReliableSpeakers:
        sf.append('no_reliable_speakers')
    if not interfaces.HasBluetooth:
        sf.append('no_bluetooth')
    if not interfaces.HasMicrophone:
        sf.append('no_microphone')

    return sf, usf


def get_bass_data_sources(data_sources):
    result = {}

    black_box_data = data_sources.get(BLACK_BOX)
    if black_box_data:
        result[str(BLACK_BOX)] = message_as_dict(black_box_data)

    qdi = data_sources.get(QUASAR_DEVICES_INFO)
    if qdi:
        result[str(QUASAR_DEVICES_INFO)] = message_as_dict(qdi)

    # Vins video is disabled since protocol one is working so we don't save external markup.
    return result


def get_features(data_sources):
    features = {}

    wizard = {}
    vins_wizard_rules = data_sources.get(VINS_WIZARD_RULES)
    if vins_wizard_rules:
        vins_wizard_rules = vins_wizard_rules.VinsWizardRules
        wizard['rules'] = json.loads(vins_wizard_rules.RawJson)
    begemot_external_markup = data_sources.get(BEGEMOT_EXTERNAL_MARKUP)
    if begemot_external_markup:
        begemot_external_markup = begemot_external_markup.BegemotExternalMarkup
        wizard['markup'] = message_as_dict(begemot_external_markup)
    if wizard:
        features['wizard'] = wizard

    entity_search = data_sources.get(ENTITY_SEARCH)
    if entity_search:
        entity_search = entity_search.EntitySearch
        features['entity_search'] = json.loads(entity_search.RawJson)

    return features if features else None


def get_form_from_semantic_frame(semantic_frame, is_utterance_empty=False):
    def get_dict_or_unicode(value):
        try:
            return json.loads(value)
        except ValueError:
            return value

    def get_type(value):
        if value:
            if value.startswith('sys.'):
                value = value[4:]
            elif value.startswith('custom.'):
                value = value[7:]
        return value

    slots = [{
        'name': smart_unicode(slot.Name),
        'type': get_type(smart_unicode(slot.Type)),
        'value': get_dict_or_unicode(smart_unicode(slot.Value)),
    } for slot in semantic_frame.Slots]

    # MEGAMIND-2621 - we should support compatibility for player command pushes
    form_name = smart_unicode(semantic_frame.Name)
    if is_utterance_empty:
        if form_name == PLAYER_FRAME_WHAT_IS_PLAYING_OUTER:
            form_name = PLAYER_FRAME_WHAT_IS_PLAYING_VINS
        elif form_name.startswith(PLAYER_FRAMES_PREFIX + '.'):
            form_name = form_name.replace(PLAYER_FRAMES_PREFIX + '.', PLAYER_FRAMES_PREFIX + '_')

    return {
        'slots': slots,
        'name': form_name,
    }


def create_req_info(request, event, ensure_purity=False, utterance=None, headers=None, srcrwr=None, scenario_id=None):
    headers = headers or Headers()
    state = unpack_state(request.BaseRequest.State)
    base_request = request.BaseRequest
    options = base_request.Options
    srcrwr = srcrwr or {}

    supported_features, unsupported_features = get_supported_features(base_request.Interfaces)

    additional_options = {
        'bass_options': {
            'filtration_level': options.FiltrationLevel,
            'video_gallery_limit': options.VideoGalleryLimit,
        },
        'supported_features': supported_features,
        'unsupported_features': unsupported_features,
        'server_time_ms': base_request.ServerTimeMs,
    }

    if options.UserDefinedRegionId:
        additional_options['bass_options']['region_id'] = options.UserDefinedRegionId
    if headers.oauth_token:
        additional_options['oauth_token'] = headers.oauth_token
    if headers.user_ticket:
        additional_options['user_ticket'] = headers.user_ticket

    if options.RadioStations:
        additional_options['radiostations'] = [smart_unicode(station) for station in options.RadioStations]

    if options.ClientIP:
        additional_options['bass_options']['client_ip'] = options.ClientIP

    # MEGAMIND-467
    additional_options['bass_options']['event'] = event.to_dict()
    if options.UserAgent:
        additional_options['bass_options']['user_agent'] = options.UserAgent
    additional_options['permissions'] = [
        {
            'status': permission.Granted,
            'name': permission.Name
        } for permission in options.Permissions
    ]

    if options.FavouriteLocations:
        favourites = []
        for favourite in options.FavouriteLocations:
            value = {
                'lat': favourite.Lat,
                'lon': favourite.Lon,
                'title': smart_unicode(favourite.Title),
            }
            if favourite.SubTitle:
                value['subtitle'] = smart_unicode(favourite.SubTitle)
            favourites.append(value)
        additional_options['favourites'] = favourites

    personal_data = None
    if options.RawPersonalData:
        personal_data = json.loads(smart_unicode(options.RawPersonalData))

    semantic_frames = None
    if request.Input.SemanticFrames:
        is_utterance_empty = not event.utterance or not event.utterance.text
        semantic_frames = [get_form_from_semantic_frame(semantic_frame, is_utterance_empty) for semantic_frame in
                           request.Input.SemanticFrames if semantic_frame.Name != IMAGE_SEARCH_SEMANTIC_FRAME_NAME]

    if skip_relevant_intents(scenario_id, semantic_frames, base_request.Experiments):
        additional_options['skip_relevant_intents'] = True
    # todo(g-kostin,alkapov,ran1s): each field should be filled properly.
    req_info = ReqInfo(
        additional_options=additional_options,
        app_info=AppInfo(
            app_id=smart_unicode(base_request.ClientInfo.AppId),
            app_version=smart_unicode(base_request.ClientInfo.AppVersion),
            device_manufacturer=smart_unicode(base_request.ClientInfo.DeviceManufacturer),
            device_model=smart_unicode(base_request.ClientInfo.DeviceModel),
            os_version=smart_unicode(base_request.ClientInfo.OsVersion),
            platform=smart_unicode(base_request.ClientInfo.Platform),
        ),
        client_time=client_time_from_timestamp(float(base_request.ClientInfo.Epoch), base_request.ClientInfo.Timezone),
        device_id=get_device_id(smart_unicode(base_request.ClientInfo.DeviceId)),
        device_state=message_as_dict(base_request.DeviceState) or dict(),
        dialog_id=smart_unicode(base_request.DialogId) or None,
        ensure_purity=ensure_purity,
        event=event,
        experiments=Experiments(message_as_dict(base_request.Experiments)).merge({
            'uniproxy_vins_sessions': '1',
        }),
        memento=base64.b64encode(base_request.Memento.SerializeToString()),
        lang=base_request.ClientInfo.Lang,
        user_lang=langs.IsoNameByLanguage(base_request.UserLanguage) if base_request.UserLanguage != ELang.L_UNK else '',
        location={
            'lon': base_request.Location.Lon,
            'lat': base_request.Location.Lat,
            'accuracy': base_request.Location.Accuracy,
        },
        prev_req_id=smart_unicode(state.PrevReqId) or None,
        request_id=base_request.RequestId,
        rng_seed_salt=get_setting('RANDOM_SEED_SALT', default=''),
        session=smart_unicode(state.Session) or None,
        utterance=utterance or event.utterance,
        uuid=UUID(smart_unicode(base_request.ClientInfo.Uuid)),
        # test_ids=req_data['request'].get('test_ids', []),
        voice_session=base_request.Interfaces.VoiceSession or isinstance(event, VoiceInputEvent),
        # laas_region=req_data['request'].get('laas_region'),
        # proxy_header=proxy_header, todo: figure out
        srcrwr=srcrwr,
        personal_data=personal_data,
        data_sources=get_bass_data_sources(request.DataSources),
        features=get_features(request.DataSources),
        has_image_search_granet=has_image_search_intent(request.Input),
        reset_session=base_request.IsNewSession or base_request.IsSessionReset,
        semantic_frames=semantic_frames,
        scenario_id=scenario_id,
    )
    return req_info


VERSION = '{branch}@{revision}'.format(branch=svn_branch(), revision=svn_last_revision())


def fill_version(response):
    response.Version = VERSION


def serialize_action(vins_directives, action):
    for vins_directive in vins_directives:
        if vins_directive.name not in _FORBIDDEN_DIRECTIVE_NAMES:
            serialized = serialize_directive(vins_directive)
            if serialized is not None:
                action.Directives.List.add().CopyFrom(serialized)


def serialize_vins_base_button(vins_button, button, actions):
    button.Title = vins_button.title
    if isinstance(vins_button, ActionButton):
        action_id, action = generate_action(actions)
        serialize_action(vins_button.directives, action)
        button.ActionId = action_id


def ensure_vins_button(vins_button):
    if isinstance(vins_button, dict):
        vins_button = Button.from_dict(vins_button)
    assert isinstance(vins_button, Button)
    return vins_button


def serialize_button(vins_button, actions):
    vins_button = ensure_vins_button(vins_button)
    button = TLayout.TButton()
    serialize_vins_base_button(vins_button, button, actions)
    return button


def try_get_search_query_from_slots(slots):
    for slot in slots:
        if slot.get('name') == 'query':
            return slot.get('value')
    return None


def try_get_search_query_from_button(vins_button):
    for directive in vins_button.directives:
        if isinstance(directive, ServerActionDirective) and directive.payload:
            suggest_block = directive.payload.get('suggest_block', {})
            if suggest_block.get('suggest_type') == 'search_internet_fallback':
                form_update = suggest_block.get('form_update')
                if form_update:
                    query = try_get_search_query_from_slots(form_update.get('slots') or [])
                    if query:
                        return query
    return None


def serialize_suggest(vins_button, actions):
    vins_button = ensure_vins_button(vins_button)
    suggest = TLayout.TSuggest()

    search_query = try_get_search_query_from_button(vins_button)

    if search_query:
        button = suggest.SearchButton
        button.Title = trim_with_ellipsis(search_query)
        button.Query = search_query
    else:
        button = suggest.ActionButton
        serialize_vins_base_button(vins_button, button, actions)
        if isinstance(vins_button, ThemedActionButton):
            button.Theme.ImageUrl = vins_button.theme.get('image_url') or ''

    return suggest


def generate_action(actions):
    action_id = uuid4().hex
    return action_id, actions[action_id]


def serialize_card(vins_card, card, actions):
    # TODO(alkapov): implement
    if isinstance(vins_card, SimpleCard):
        card.Text = vins_card.text
    elif isinstance(vins_card, CardWithButtons):
        text_with_buttons = card.TextWithButtons
        text_with_buttons.Text = vins_card.text
        for vins_button in vins_card.buttons:
            text_with_buttons.Buttons.add().CopyFrom(serialize_button(vins_button, actions))
    elif isinstance(vins_card, DivCard):
        card.DivCard.CopyFrom(dict_to_struct(vins_card.body))  # TODO(alkapov): deeplinks???
    elif isinstance(vins_card, Div2Card):
        card.Div2CardExtended.Body.CopyFrom(dict_to_struct(vins_card.body))  # TODO(alkapov): deeplinks???
        card.Div2CardExtended.HideBorders = vins_card.hide_borders
        card.Div2CardExtended.Text = vins_card.text or ''
    else:
        logger.error('Unable to serialize card with type: %(type)s', {'type': card.type})


def serialize_dict_to_struct_value(data):
    value = struct_pb2.Value()
    value.string_value = json.dumps(data, ensure_ascii=False)
    return value


def safe_non_serializable_dict_to_struct(dict_):
    default = lambda o: "<<non-serializable {}>>".format(type(o).__name__)
    json_str = json.dumps(dict_, default=default)
    dict_ = json.loads(json_str)
    return dict_to_struct(dict_)


def select_req_info_required_fields(req_info):
    # Original req_info can contain sensitive data (tokens), so we select only required fields. Feel free to add some fields if needed.
    return {
        'experiments': req_info.experiments.to_dict() if req_info.experiments else {},
        'utterance': req_info.utterance.to_dict() if req_info.utterance else None,
        'client_time': req_info.client_time.isoformat() if req_info.client_time else None,
        'request_id': str(req_info.request_id) if req_info.request_id else None,
        'event': req_info.event.to_dict() if req_info.event else None,
    }


def parse_slot(slot_dict, slot):
    slot.Name = slot_dict.get('name') or slot_dict.get('slot') or ''
    if 'value_type' in slot_dict:
        slot.TypedValue.Type = slot_dict['value_type'] or 'string'
        if slot_dict['value_type'] == 'string':
            slot.TypedValue.String = smart_unicode(slot_dict.get('value') or '')
        else:
            slot.TypedValue.String = json.dumps(slot_dict.get('value'), ensure_ascii=False)


def parse_semantic_frame(form, semantic_frame):
    semantic_frame.Name = form.get('name') or form.get('form') or ''
    for slot_dict in form.get('slots') or []:
        parse_slot(slot_dict, semantic_frame.Slots.add())


def parse_gc_meta(meta):
    intent = None
    source = None
    not_pure_gc = True
    for item in meta:
        if isinstance(item, FormRestoredMeta) and not_pure_gc:
            intent = item.overriden_form
        elif isinstance(item, RepeatMeta):
            intent = 'personal_assistant.scenarios.repeat'
        elif hasattr(item, 'pure_gc'):
            intent = 'personal_assistant.scenarios.external_skill_gc'
            not_pure_gc = False
        if isinstance(item, GeneralConversationSourceMeta):
            source = item.source

    if intent is None and source is None:
        return None

    form_info = TVinsGcMeta()
    form_info.Intent = intent or ''
    form_info.IsPureGc = not not_pure_gc
    form_info.Source = source or ''
    return form_info


def serialize_nlg_render_data(nlg_render_bass_blocks, nlg_render_data):
    for nlg_render_bass_block in nlg_render_bass_blocks:
        bass_block_proto = nlg_render_data.BassBlocks.add()
        bass_block_proto.CopyFrom(dict_to_struct(nlg_render_bass_block.bass_block_dict))


def serialize_bass_response(bass_response, bass_response_proto):
    if not bass_response or not bass_response.form_info:
        return

    bass_response_proto.Form.Name = bass_response.form_info.name
    for slot in bass_response.form_info.slots:
        slot_proto = bass_response_proto.Form.Slots.add()
        slot_proto.Type = slot.type
        slot_proto.Name = slot.name
        slot_proto.Data.CopyFrom(dict_to_struct(slot.to_dict()))

    for block in bass_response.blocks:
        block_proto = bass_response_proto.Blocks.add()
        block_proto.Type = block.type
        block_proto.Data.CopyFrom(dict_to_struct(block.to_dict()))


def serialize_response_body(vins_response, response_body, state, is_voice_session, gc_meta):
    response_body.State.Pack(state)
    layout = response_body.Layout
    actions = response_body.FrameActions
    for card in vins_response.cards:
        serialize_card(card, layout.Cards.add(), actions)

    if vins_response.templates:
        layout.Div2Templates.CopyFrom(dict_to_struct(vins_response.templates))

    add_contact_book_asr = any(
        vins_directive.name == ADD_CONTACT_BOOK_ASR_DIRECTIVE_NAME for vins_directive in vins_response.directives)
    for vins_directive in vins_response.directives:
        if vins_directive.name in _FORBIDDEN_DIRECTIVE_NAMES:
            continue
        if vins_directive.name in SERVER_DIRECTIVE_NAMES:
            directive = serialize_server_directive(vins_directive)
            response_body.ServerDirectives.add().CopyFrom(directive)
            continue
        directive = serialize_directive(vins_directive)
        if directive is None:
            continue
        if directive.WhichOneof('Directive') == 'FindContactsDirective':
            directive.FindContactsDirective.AddAsrContactBook = add_contact_book_asr
        layout.Directives.add().CopyFrom(directive)

    for suggest in vins_response.suggests:
        layout.SuggestButtons.add().CopyFrom(serialize_suggest(suggest, actions))

    layout.ShouldListen = is_voice_session and (vins_response.should_listen is None or vins_response.should_listen)

    if vins_response.voice_text and (is_voice_session or vins_response.force_voice_answer):
        layout.OutputSpeech = vins_response.voice_text

    analytics_info_meta = vins_response.analytics_info_meta
    if analytics_info_meta:
        scenario_analytics_info = analytics_info_meta.get_scenario_analytics_info()
        response_body.AnalyticsInfo.CopyFrom(scenario_analytics_info)
        parse_semantic_frame(analytics_info_meta.form or {}, response_body.SemanticFrame)

    for nlg_render_history_record in vins_response.nlg_render_history_records:
        proto = response_body.AnalyticsInfo.NlgRenderHistoryRecords.add()
        if nlg_render_history_record.phrase_id:
            proto.PhraseName = nlg_render_history_record.phrase_id
        elif nlg_render_history_record.card_id:
            proto.CardName = nlg_render_history_record.card_id
        if nlg_render_history_record.context:
            proto.Context.CopyFrom(safe_non_serializable_dict_to_struct(nlg_render_history_record.context))
        if nlg_render_history_record.req_info:
            proto.ReqInfo.CopyFrom(safe_non_serializable_dict_to_struct(select_req_info_required_fields(nlg_render_history_record.req_info)))
        if nlg_render_history_record.form:
            proto.Form.CopyFrom(safe_non_serializable_dict_to_struct(nlg_render_history_record.form.to_dict(truncate=True)))
        proto.Language = ELang.L_RUS

    response_body.ContextualData.ResponseLanguage = ELang.L_RUS

    if gc_meta is not None:
        response_body.AnalyticsInfo.Objects.add().VinsGcMeta.CopyFrom(gc_meta)

    for item in vins_response.meta:
        if isinstance(item, ErrorMeta):
            error_meta = response_body.AnalyticsInfo.Objects.add().VinsErrorMeta
            error_meta.Type = item.error_type
            if item.form_name:
                error_meta.Intent = item.form_name

    for action_id, frame_action in vins_response.frame_actions.items():
        ParseDict(frame_action, actions[action_id])

    if vins_response.scenario_data is not None:
        ParseDict(vins_response.scenario_data, response_body.ScenarioData)

    if vins_response.stack_engine is not None:
        ParseDict(vins_response.stack_engine, response_body.StackEngine)


def serialize_apply_arguments(vins_apply_arguments, apply_arguments):
    apply_arguments.Pack(serialize_dict_to_struct_value(vins_apply_arguments.to_dict()))


def serialize_commit_arguments(vins_commit_arguments, commit_arguments):
    commit_arguments.Pack(serialize_dict_to_struct_value(vins_commit_arguments))


def deserialize_struct_value_to_dict(data):
    value = struct_pb2.Value()
    data.Unpack(value)
    if value.string_value:
        return json.loads(value.string_value)
    return {}


def determine_player_type(vins_response):
    for d in vins_response.directives:
        if d.name == 'video_play':
            return 'video'
        elif d.name == 'music_play':
            return 'music'
        elif d.name == 'radio_play':
            return 'radio'
        elif d.payload is not None and 'player' in d.payload:
            return d.payload.get('player')

    return None


def serialize_player_features(response, device_state, current_timestamp, player_type=None, player_features=None):
    def _apply(last_play_timestamp):
        logger.info('serialize_player_features: last_play_timestamp = %s (for player_type=%s)', last_play_timestamp, player_type)

        seconds_since_last_pause = current_timestamp - last_play_timestamp // MILLISECONDS
        if seconds_since_last_pause < 0:
            seconds_since_last_pause = 0
            logger.warning('serialize_player_features: Got negative seconds since last pause: %s', seconds_since_last_pause)

        if seconds_since_last_pause < SECONDS_IN_A_WEEK:
            logger.info('serialize_player_features: setting the RestorePlayer flag')
            response.Features.PlayerFeatures.RestorePlayer = True
            response.Features.PlayerFeatures.SecondsSincePause = int(seconds_since_last_pause)

    response.Features.PlayerFeatures.RestorePlayer = False

    if player_features is not None and player_features.restore_player:
        _apply(player_features.last_play_timestamp)
        return

    if (
            is_quasar_gallery_navigation(response.Features.Intent) or
            is_quasar_screen_navigation(response.Features.Intent) or
            is_quasar_video_details_opening_action(response.Features.Intent)
    ):
        logger.info('serialize_player_features: setting the RestorePlayer flag, for player-like navigation intent')
        response.Features.PlayerFeatures.RestorePlayer = True
        response.Features.PlayerFeatures.SecondsSincePause = 0
        return

    if not is_player_intent(response.Features.Intent):
        logger.info('serialize_player_features: intent %s is not a player/music_recognizer intent',
                    response.Features.Intent)
        return

    last_play_timestamps = {
        'bluetooth': device_state.Bluetooth.LastPlayTimestamp,
        'music': device_state.Music.LastPlayTimestamp,
        'video': device_state.Video.LastPlayTimestamp,
        'radio': device_state.Radio.fields['last_play_timestamp'].number_value,
    }

    last_play_timestamp = last_play_timestamps.get(player_type, max(last_play_timestamps.values()))

    _apply(last_play_timestamp)


def serialize_features(vins_response, response, gc_meta):
    form_info_features = vins_response.features.get(FormInfoFeatures.FEATURE_TYPE)
    response.Features.IgnoresExpectedRequest = True
    if form_info_features:
        response.Features.VinsFeatures.IsContinuing = form_info_features.is_continuing
        response.Features.IgnoresExpectedRequest = not form_info_features.answers_expected_request
        response.Features.Intent = form_info_features.intent or ''
        response.Features.IsIrrelevant = form_info_features.is_irrelevant or False
        response.ResponseBody.ExpectsRequest = form_info_features.expects_request or False
    if gc_meta is not None:
        response.Features.VinsFeatures.IsPureGC = gc_meta.IsPureGc


def handle_vins_request(app, req_info, **kwargs):
    if req_info.request_id != TEST_REQUEST_ID:
        random.seed(req_info.rng_seed)
    return app.handle_request(req_info, **kwargs)


def on_run_request(app, request, headers=None, srcrwr=None, cgi_params=None, scenario_id=None):
    vins_run_response_proto = TVinsRunResponse()
    response = TScenarioRunResponse()
    fill_version(response)
    try:
        req_info = create_req_info(request, get_event(request), ensure_purity=True, headers=headers, srcrwr=srcrwr,
                                   scenario_id=scenario_id)
        with sensors.labels_context({'app_id': req_info.app_info.app_id}):
            vins_response = handle_vins_request(app, req_info)
            gc_meta = parse_gc_meta(vins_response.meta)
            serialize_features(vins_response, response, gc_meta)
            serialize_player_features(response, request.BaseRequest.DeviceState,
                                      datetime_to_timestamp(req_info.client_time),
                                      player_type=determine_player_type(vins_response),
                                      player_features=vins_response.player_features)
            if vins_response.apply_arguments is not None:
                # Pass datasources to apply request.
                vins_response.apply_arguments.data_sources = req_info.data_sources
                serialize_apply_arguments(vins_response.apply_arguments, response.ApplyArguments)
            elif vins_response.continue_arguments is not None:
                serialize_apply_arguments(vins_response.continue_arguments, response.ContinueArguments)
            elif vins_response.commit_arguments is not None:
                serialize_response_body(
                    vins_response,
                    response.CommitCandidate.ResponseBody,
                    create_state(req_info, vins_response),
                    is_voice_session=req_info.voice_session,
                    gc_meta=gc_meta,
                )
                if vins_response.commit_arguments is not None:
                    serialize_commit_arguments(vins_response.commit_arguments, response.CommitCandidate.Arguments)
            else:
                serialize_response_body(
                    vins_response,
                    response.ResponseBody,
                    create_state(req_info, vins_response),
                    is_voice_session=req_info.voice_session,
                    gc_meta=gc_meta,
                )
            serialize_nlg_render_data(vins_response.nlg_render_bass_blocks, vins_run_response_proto.NlgRenderData)
            serialize_bass_response(vins_response.bass_response, vins_run_response_proto.BassResponse)
    except Exception as e:
        logger.exception('Unable to handle Run request %(request)s', {'request': request})
        response.Error.Message = unicode(e)

    if str_to_bool(cgi_params and cgi_params.get('use_vins_response_proto')):
        logger.debug('Using TVinsRunResponse instead of TScenarioRunResponse')
        vins_run_response_proto.ScenarioRunResponse.CopyFrom(response)
        return vins_run_response_proto

    return response


def on_continue_request(app, request, headers=None, srcrwr=None, scenario_id=None):
    response = TScenarioContinueResponse()
    fill_version(response)
    try:
        arguments = deserialize_struct_value_to_dict(request.Arguments)
        logger.debug('Got continue arguments: %s', arguments)
        event_data = {
            'name': 'continue_request',
            'payload': arguments,
            'end_of_utterance': True,
            'type': 'server_action',
        }
        response.ResponseBody.ExpectsRequest = arguments['callback'].get('expects_request', False)
        req_info = create_req_info(request, RequestEvent.from_dict(event_data), headers=headers, srcrwr=srcrwr,
                                   scenario_id=scenario_id)
        vins_response = handle_vins_request(app, req_info)
        gc_meta = parse_gc_meta(vins_response.meta)
        serialize_response_body(
            vins_response,
            response.ResponseBody,
            create_state(req_info, vins_response),
            is_voice_session=req_info.voice_session,
            gc_meta=gc_meta,
        )
    except Exception as e:
        logger.exception('Unable to handle Continue request %(request)s', {'request': request})
        response.Error.Message = unicode(e)
    return response


def on_apply_request(app, request, headers=None, srcrwr=None, cgi_params=None, scenario_id=None):
    vins_apply_response_proto = TVinsApplyResponse()
    response = TScenarioApplyResponse()
    fill_version(response)
    try:
        arguments = deserialize_struct_value_to_dict(request.Arguments)
        logger.debug('Got apply arguments: %s', arguments)
        event_data = {
            'name': 'apply_request',
            'payload': arguments,
            'end_of_utterance': True,
            'type': 'server_action',
        }
        response.ResponseBody.ExpectsRequest = arguments['callback'].get('expects_request', False)
        req_info = create_req_info(request, RequestEvent.from_dict(event_data), headers=headers, srcrwr=srcrwr,
                                   scenario_id=scenario_id)

        # patch datasources from arguments
        data_sources = arguments.get('data_sources', {})
        req_info.data_sources.update(data_sources)

        vins_response = handle_vins_request(app, req_info)
        gc_meta = parse_gc_meta(vins_response.meta)
        serialize_response_body(
            vins_response,
            response.ResponseBody,
            create_state(req_info, vins_response),
            is_voice_session=req_info.voice_session,
            gc_meta=gc_meta,
        )
        serialize_nlg_render_data(vins_response.nlg_render_bass_blocks, vins_apply_response_proto.NlgRenderData)
        serialize_bass_response(vins_response.bass_response, vins_apply_response_proto.BassResponse)
    except Exception as e:
        logger.exception('Unable to handle Apply request %(request)s', {'request': request})
        response.Error.Message = unicode(e)

    if str_to_bool(cgi_params and cgi_params.get('use_vins_response_proto')):
        logger.debug('Using TVinsApplyResponse instead of TScenarioApplyResponse')
        vins_apply_response_proto.ScenarioApplyResponse.CopyFrom(response)
        return vins_apply_response_proto

    return response


def on_commit_request(app, request, headers=None, srcrwr=None, scenario_id=None):
    request_dict = message_as_dict(request)
    response = TScenarioCommitResponse()
    try:
        arguments = deserialize_struct_value_to_dict(request.Arguments)
        logger.debug('Got commit arguments: %s', arguments)
        event_data = {
            'name': 'commit_request',
            'payload': {'request': request_dict},
            'end_of_utterance': True,
            'type': 'server_action',
        }
        req_info = create_req_info(request, RequestEvent.from_dict(event_data), headers=headers, srcrwr=srcrwr,
                                   scenario_id=scenario_id)
        vins_response = handle_vins_request(app, req_info, request=request_dict)

        ParseDict(vins_response.commit_response, response)
    except Exception as e:
        logger.exception('Unable to handle commit request %(request)s', {'request': request_dict})
        response.Error.Message = unicode(e)

    fill_version(response)  # Overwrite the version got from BASS.
    return response


def handle_request(handle_name, app, request, headers, srcrwr, cgi_params=None, scenario_id=None):
    with LogContext(request_id=str(request.BaseRequest.RequestId),
                    uuid=str(request.BaseRequest.ClientInfo.Uuid),
                    device_id=str(request.BaseRequest.ClientInfo.DeviceId),
                    worker_pid=os.getpid()):
        with sensors.timer('view_handle_request_time'):
            if isinstance(request, TScenarioRunRequest):
                return on_run_request(app, request, headers, srcrwr, cgi_params, scenario_id)
            elif isinstance(request, TScenarioApplyRequest):
                if handle_name == 'apply':
                    return on_apply_request(app, request, headers, srcrwr, cgi_params, scenario_id)
                elif handle_name == 'continue':
                    return on_continue_request(app, request, headers, srcrwr, scenario_id)
                elif handle_name == 'commit':
                    return on_commit_request(app, request, headers, srcrwr, scenario_id)
    raise ValueError('Invalid request type')


def parse_srcrwr(params):
    srcrwr = {}
    for k, v in params.items():
        if k.lower() in {'x-srcrwr', 'srcrwr'}:
            for rewrite in (v if isinstance(v, list) else [v]):
                rewrite = urllib.unquote(rewrite)
                rw = rewrite.split(':', 1)
                if len(rw) == 2:
                    srcrwr[rw[0]] = rw[1]
    return srcrwr


class ProtocolResource(BaseConnectedAppResource):
    storage_cls = SKSessionStorage

    def on_post(self, req, resp, app_id, method='run', scenario_id=None):
        if req.content_type != 'application/protobuf':
            raise ValidationError('Unsupported media type "{}" in request.'.format(req.content_type))

        if method == 'run':
            raw_request = req.stream.read()
            request = TScenarioRunRequest.FromString(raw_request)
        elif method in ('apply', 'commit', 'continue'):
            raw_request = req.stream.read()
            request = TScenarioApplyRequest.FromString(raw_request)
        else:
            resp.status = falcon.HTTP_404
            logger.error('Method %s unsupported', method)
            return

        headers = Headers.from_dict(req.headers)

        srcrwr = parse_srcrwr(req.params)
        logger.info('Rewriting sources: %s', srcrwr)

        response = handle_request(method, self.get_or_create_connected_app(app_id), request, headers=headers,
                                  srcrwr=srcrwr, cgi_params=req.params, scenario_id=scenario_id)

        resp.set_header('Content-Type', 'application/protobuf')
        resp.body = response.SerializeToString()
