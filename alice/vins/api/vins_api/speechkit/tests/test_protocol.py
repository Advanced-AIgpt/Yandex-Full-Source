# coding: utf-8

from __future__ import unicode_literals

import attr
import json
import pytest
import six

from google.protobuf import struct_pb2, json_format
from uuid import uuid4

from alice.megamind.protos.analytics.scenarios.vins.vins_pb2 import TVinsGcMeta, TVinsErrorMeta
from alice.megamind.protos.common.app_type_pb2 import EAppType
from alice.megamind.protos.common.data_source_type_pb2 import (
    VINS_WIZARD_RULES, ENTITY_SEARCH, BEGEMOT_EXTERNAL_MARKUP, BLACK_BOX
)
from alice.memento.proto.api_pb2 import CK_REMINDERS, TConfigKeyAnyPair
from alice.megamind.protos.blackbox.blackbox_pb2 import TBlackBoxUserInfo
from alice.megamind.protos.common.frame_pb2 import TSemanticFrame
from alice.megamind.protos.scenarios.analytics_info_pb2 import TAnalyticsInfo
from alice.megamind.protos.scenarios.external_markup_pb2 import TBegemotExternalMarkup
from alice.megamind.protos.scenarios.directives_pb2 import (
    TDirective, TNavigateBrowserDirective, TPlayerRewindDirective, TServerDirective, TUpdateDatasyncDirective, TDrawScledAnimationsDirective
)
from alice.megamind.protos.scenarios.request_pb2 import (
    TScenarioApplyRequest, TScenarioRunRequest, TInput, TInterfaces, TDataSource, TVinsWizardRules, TEntitySearch
)
from alice.megamind.protos.scenarios.response_pb2 import (
    TScenarioApplyResponse, TScenarioCommitResponse, TScenarioContinueResponse, TScenarioRunResponse, TLayout, TScenarioResponseBody
)
from alice.vins.api.vins_api.speechkit.connectors.protocol.protos.state_pb2 import TState

from alice.protos.data.scenario.reminders.state_pb2 import TState as TRemindersState

from library.python.svn_version import svn_branch, svn_last_revision

from personal_assistant.meta import ExternalSkillMeta, GeneralConversationMeta, GeneralConversationSourceMeta

from vins_api.speechkit.connectors.protocol.directives import (
    serialize_directive, SETTINGS_TARGET_MAPPING, serialize_server_directive
)

from vins_api.speechkit.connectors.protocol.utils import Headers, unpack_state
from vins_api.speechkit.resources.protocol import (
    serialize_card, get_event, parse_gc_meta, get_supported_features, get_features, create_req_info, parse_srcrwr,
    get_form_from_semantic_frame, serialize_dict_to_struct_value, serialize_response_body
)
from vins_core.dm.request_events import ServerActionEvent, RequestEvent
from vins_core.dm.response import ActionDirective, DivCard, VinsResponse
from vins_core.utils.json_util import MessageToDict, dict_to_struct
from vins_core.utils.strings import smart_unicode


def _fill_base_request(request, request_id=None, state=None, uuid=None, device_id=None):
    request.BaseRequest.RequestId = str(request_id or uuid4())
    request.BaseRequest.RandomSeed = 42
    if state:
        request.BaseRequest.State.Pack(state)

    request.BaseRequest.ClientInfo.AppId = 'com.yandex.search'
    request.BaseRequest.ClientInfo.AppVersion = '1.2.3'
    request.BaseRequest.ClientInfo.ClientTime = '20161213T151558'
    request.BaseRequest.ClientInfo.Timezone = 'Europe/Moscow'
    request.BaseRequest.ClientInfo.Epoch = '1481631358'
    request.BaseRequest.ClientInfo.Lang = 'ru-RU'
    request.BaseRequest.ClientInfo.Platform = 'android'
    request.BaseRequest.ClientInfo.OsVersion = '5.0'
    request.BaseRequest.ClientInfo.DeviceId = str(device_id or uuid4())
    request.BaseRequest.ClientInfo.Uuid = str(uuid or uuid4())


def _create_run_request(request_input, request_id=None, state=None, uuid=None, device_id=None):
    request = TScenarioRunRequest()
    _fill_base_request(request, request_id, state, uuid, device_id)
    request.Input.MergeFrom(request_input)
    return request


def _run_request(sk_client, request):
    resp = sk_client.simulate_post(
        '/proto/app/test/run',
        body=request.SerializeToString(),
        headers={'content-type': b'application/protobuf'}
    )
    return resp


def _create_apply_request(apply_arguments=None, request_id=None, state=None, uuid=None, device_id=None):
    request = TScenarioApplyRequest()
    _fill_base_request(request, request_id, state, uuid, device_id)
    if apply_arguments:
        request.Arguments.CopyFrom(apply_arguments)
    return request


def _post_run_request(handle_name, sk_client, request):
    resp = sk_client.simulate_post(
        '/proto/app/test/' + handle_name,
        body=request.SerializeToString(),
        headers={'content-type': b'application/protobuf'}
    )
    return resp


def _apply_request(sk_client, request):
    return _post_run_request('apply', sk_client, request)


def _continue_request(sk_client, request):
    return _post_run_request('continue', sk_client, request)


def _commit_request(sk_client, request):
    return _post_run_request('commit', sk_client, request)


def _run_scenario_request(scenario_name, sk_client, request):
    return _post_run_request('scenario/' + scenario_name + '/run', sk_client, request)


def _assert_version(version):
    assert version == '{branch}@{revision}'.format(branch=svn_branch(),
                                                   revision=svn_last_revision())


def test_simple_run_request(sk_client):
    req_id = '2048'
    request_input = TInput()
    request_input.Text.RawUtterance = 'привет'
    request_input.Text.Utterance = 'привет'
    request = _create_run_request(request_input, req_id)
    resp = _run_request(sk_client, request)
    assert resp.status_code == 200
    response = TScenarioRunResponse.FromString(resp.content)
    assert response.ResponseBody.Layout.Cards[0].Text == 'hello, username!'
    state = unpack_state(response.ResponseBody.State)
    assert state.PrevReqId == req_id
    _assert_version(response.Version)


def test_simple_apply_request(sk_client):
    request = _create_apply_request()
    request.Arguments.Pack(serialize_dict_to_struct_value({'callback': {'name': 'empty_response',
                                                                        'arguments': {}}}))
    resp = _apply_request(sk_client, request)
    assert resp.status_code == 200
    response = TScenarioApplyResponse.FromString(resp.content)
    assert response.Error.Message == ''
    _assert_version(response.Version)


def test_simple_continue_request(sk_client):
    request = _create_apply_request()
    request.Arguments.Pack(serialize_dict_to_struct_value({'callback': {'name': 'empty_response',
                                                                        'arguments': {}}}))
    resp = _continue_request(sk_client, request)
    assert resp.status_code == 200
    response = TScenarioContinueResponse.FromString(resp.content)
    assert response.Error.Message == ''
    _assert_version(response.Version)


def test_simple_commit_request(sk_client):
    request = _create_apply_request()
    request.Arguments.Pack(serialize_dict_to_struct_value({'callback': {'name': 'empty_response',
                                                                        'arguments': {}}}))
    resp = _commit_request(sk_client, request)
    assert resp.status_code == 200
    response = TScenarioCommitResponse.FromString(resp.content)
    assert response.Error.Message == ''
    _assert_version(response.Version)


def test_run_request_with_apply_response(sk_client):
    request_input = TInput()
    request_input.Text.RawUtterance = 'запусти навык города'
    request_input.Text.Utterance = 'запусти навык города'
    request = _create_run_request(request_input)
    resp = _run_request(sk_client, request)
    assert resp.status_code == 200
    response = TScenarioRunResponse.FromString(resp.content)
    assert response.WhichOneof('Response') == 'ApplyArguments'


def test_run_request_with_continue_response(sk_client):
    request_input = TInput()
    request_input.Text.RawUtterance = 'включи поиск с continue'
    request_input.Text.Utterance = 'включи поиск с continue'
    request = _create_run_request(request_input)
    request.BaseRequest.Experiments['vins_use_continue'] = '1'
    request.BaseRequest.Experiments['vins_continue_on_search'] = '1'
    resp = _run_request(sk_client, request)
    assert resp.status_code == 200
    response = TScenarioRunResponse.FromString(resp.content)
    assert response.WhichOneof('Response') == 'ContinueArguments'


def test_run_request_with_commit_response(sk_client):
    request_input = TInput()
    request_input.Text.RawUtterance = 'включи поиск с commit'
    request_input.Text.Utterance = 'включи поиск с commit'
    request = _create_run_request(request_input)
    request.BaseRequest.Experiments['direct_confirm_hit'] = '1'
    resp = _run_request(sk_client, request)
    assert resp.status_code == 200
    response = TScenarioRunResponse.FromString(resp.content)
    assert response.WhichOneof('Response') == 'CommitCandidate'
    assert smart_unicode(response.CommitCandidate.ResponseBody.Layout.Cards[0].Text) == 'Обязательно.'


def test_run_request_with_state(sk_client):
    request_input = TInput()
    request_input.Text.RawUtterance = 'погода'
    request_input.Text.Utterance = 'погода'
    request = _create_run_request(request_input, '1')
    resp = _run_request(sk_client, request)
    assert resp.status_code == 200
    response = TScenarioRunResponse.FromString(resp.content)
    state = unpack_state(response.ResponseBody.State)
    assert state.PrevReqId == '1'
    assert smart_unicode(response.ResponseBody.Layout.Cards[0].Text) == 'Нормальная'

    request_input.Text.RawUtterance = 'а завтра'
    request_input.Text.Utterance = 'а завтра'
    request = _create_run_request(request_input, '2', state)
    resp = _run_request(sk_client, request)

    response = TScenarioRunResponse.FromString(resp.content)
    state = unpack_state(response.ResponseBody.State)
    assert state.PrevReqId == '2'
    assert smart_unicode(response.ResponseBody.Layout.Cards[0].Text) == 'И завтра нормальная'


def test_analytics_info(sk_client):
    request_input = TInput()
    utt = 'аналитикс инфо'
    request_input.Text.RawUtterance = utt
    request_input.Text.Utterance = utt
    request = _create_run_request(request_input, '1')
    resp = _run_request(sk_client, request)
    assert resp.status_code == 200
    response = TScenarioRunResponse.FromString(resp.content)
    expected_analytics_info = TAnalyticsInfo()
    expected_analytics_info.Intent = 'personal_assistant.scenarios.search'
    expected_analytics_info.ProductScenarioName = 'search'
    expected_semantic_frame = TSemanticFrame()
    slot = expected_semantic_frame.Slots.add()
    slot.TypedValue.Type = 'string'
    slot.TypedValue.String = 'какое-то значение'
    slot.Name = 'query'
    slot = expected_semantic_frame.Slots.add()
    slot.TypedValue.Type = 'string'
    slot.TypedValue.String = ''
    slot.Name = 'query_none'
    slot = expected_semantic_frame.Slots.add()
    slot.TypedValue.Type = 'named_location'
    slot.TypedValue.String = '"home"'
    slot.Name = 'where_from'
    slot = expected_semantic_frame.Slots.add()
    slot.TypedValue.Type = 'search_results'
    slot.TypedValue.String = 'null'
    slot.Name = 'search_results'
    slot = expected_semantic_frame.Slots.add()
    slot.TypedValue.Type = 'unicode_data'
    slot.TypedValue.String = '{"data": {"ключ": "значение"}}'
    slot.Name = 'unicode_data'
    expected_semantic_frame.Name = 'personal_assistant.scenarios.search'

    assert expected_analytics_info == response.ResponseBody.AnalyticsInfo
    assert expected_semantic_frame == response.ResponseBody.SemanticFrame


def test_find_contacts_with_add_asr_contacts_book(sk_client):
    request = TInput()
    request.Text.Utterance = 'find contact'
    request.Text.RawUtterance = request.Text.Utterance

    response = _run_request(sk_client, _create_run_request(request))
    assert response.status_code == 200

    response = TScenarioRunResponse.FromString(response.content)

    directives = response.ResponseBody.Layout.Directives
    assert len(directives) == 1

    directive = directives[0]
    assert directive.WhichOneof('Directive') == 'FindContactsDirective'
    assert directive.FindContactsDirective.AddAsrContactBook is True


def test_features(sk_client):
    request = TInput()
    request.Text.Utterance = 'отсыпь фичей'
    request.Text.RawUtterance = request.Text.Utterance

    response = _run_request(sk_client, _create_run_request(request))
    response = TScenarioRunResponse.FromString(response.content)

    assert response.Features.Intent == 'add_features'
    assert response.Features.VinsFeatures.IsContinuing is False
    assert response.Features.IsIrrelevant is False

    request = TInput()
    request.Text.Utterance = 'а еще'
    request.Text.RawUtterance = request.Text.Utterance

    response = _run_request(sk_client, _create_run_request(request))
    response = TScenarioRunResponse.FromString(response.content)

    assert response.Features.Intent == 'add_features__continue'
    assert response.Features.VinsFeatures.IsContinuing is True
    assert response.Features.IsIrrelevant is False

    request = TInput()
    request.Text.Utterance = 'давай поболтаем'
    request.Text.RawUtterance = request.Text.Utterance

    response = _run_request(sk_client, _create_run_request(request))
    response = TScenarioRunResponse.FromString(response.content)

    assert response.Features.VinsFeatures.IsPureGC is True

    request = TInput()
    request.Text.Utterance = 'неуместный запрос'
    request.Text.RawUtterance = request.Text.Utterance

    response = _run_request(sk_client, _create_run_request(request))
    response = TScenarioRunResponse.FromString(response.content)

    assert response.Features.Intent == 'irrelevant'
    assert response.Features.VinsFeatures.IsContinuing is False
    assert response.Features.IsIrrelevant is True


def test_json_util():
    request = TScenarioRunRequest()
    request.BaseRequest.ServerTimeMs = 1 << 33

    data = MessageToDict(request)
    assert data.get('base_request').get('server_time_ms') == (1 << 33)


def test_serialize_div_card():
    card = DivCard(body={
        'cells': [
            {},
            {
                'horizontal_alignment': 'left',
                'text': '<font color=\"#7F7F7F\">весть</font>',
                'vertical_alignment': 'bottom',
            },
        ],
        'type': 'row_element',
    })
    actual = TLayout.TCard()
    serialize_card(card, actual, {})
    expected = TLayout.TCard()
    expected.DivCard['type'] = 'row_element'
    cells = expected.DivCard.get_or_create_list('cells')
    cells.add_struct().CopyFrom(struct_pb2.Struct())
    node = cells.add_struct()
    node['horizontal_alignment'] = 'left'
    node['text'] = '<font color=\"#7F7F7F\">весть</font>'
    node['vertical_alignment'] = 'bottom'
    assert actual == expected


def test_skip_update_datasync_directive():
    history = '{"":null,"last_card":null,"last_payment_method":{"id":"cash","type":"cash"},"previous_tariff":""}'

    directive = ActionDirective.from_dict({
        'name': 'update_datasync',
        'payload': {
            'key': '/v1/personality/profile/alisa/kv/taxi_history',
            'listening_is_possible': True,
            'method': 'PUT',
            'value': history,
        },
        'type': 'uniproxy_action',
    })

    actual = serialize_directive(directive)
    assert actual is None


def test_should_listen_by_default(sk_client):
    request = TInput()
    request.Voice.Utterance = 'привет'
    asr_result = request.Voice.AsrData.add()
    asr_result.Utterance = 'привет'
    asr_result.Confidence = 1.0
    word = asr_result.Words.add()
    word.Value = 'привет'
    word.Confidence = 1.0

    response = _run_request(sk_client, _create_run_request(request, request_id='d00d3'))
    response = TScenarioRunResponse.FromString(response.content)

    assert response.ResponseBody.Layout.ShouldListen is True

    request = TInput()
    request.Text.Utterance = 'привет'

    response = _run_request(sk_client, _create_run_request(request, request_id='d00d35'))
    response = TScenarioRunResponse.FromString(response.content)

    assert response.ResponseBody.Layout.ShouldListen is False


def test_client_time(sk_client):
    request_input = TInput()
    request_input.Text.RawUtterance = 'который час'

    request = _create_run_request(request_input)
    request.BaseRequest.ClientInfo.ClientTime = '20200225T153336'
    request.BaseRequest.ClientInfo.Timezone = ''
    request.BaseRequest.ClientInfo.Epoch = '1582634016'

    response = _run_request(sk_client, request)
    response = TScenarioRunResponse.FromString(response.content)
    assert response.ResponseBody.Layout.Cards[0].Text == '15:33'


def test_image_search_granet(sk_client):
    request_input = TInput()
    request_input.Text.RawUtterance = 'фотки котиков'
    request_input.SemanticFrames.add().Name = 'personal_assistant.scenarios.search.images'

    response = _run_request(sk_client, _create_run_request(request_input))
    response = TScenarioRunResponse.FromString(response.content)
    assert response.ResponseBody.Layout.Cards[0].Text == 'Держите!'.encode('utf-8')

    request_input = TInput()
    request_input.Text.RawUtterance = 'фотки котиков'
    response = _run_request(sk_client, _create_run_request(request_input))
    response = TScenarioRunResponse.FromString(response.content)
    assert response.ResponseBody.Layout.Cards[0].Text == 'Поищите в Яндексе!'.encode('utf-8')


def test_callback_with_integer_payload():
    request_input = TInput()
    request_input.Callback.Name = 'test_callback'
    request_input.Callback.Payload['value'] = 1.23e10
    event = get_event(_create_run_request(request_input))
    assert isinstance(event, ServerActionEvent)
    assert isinstance(event.payload['value'], int)
    assert event.payload['value'] == 12300000000


def make_vins_directive(name, directive_type=None, sub_name=None, **kwargs):
    assert directive_type is not None
    directive = {
        'name': name,
        'type': directive_type,
        'payload': kwargs,
    }
    if sub_name is not None:
        directive['sub_name'] = sub_name
    return ActionDirective.from_dict(directive)


def fill_proto_fields(proto, dict_):
    for k, v in six.iteritems(dict_):
        if isinstance(v, dict):
            fill_proto_fields(getattr(proto, k), v)
        elif isinstance(v, list):
            container = getattr(proto, k)
            for item in v:
                if isinstance(item, (dict, list, tuple)):
                    fill_proto_fields(container.add(), item)
                else:
                    container.append(item)
        else:
            if isinstance(v, struct_pb2.Struct):
                getattr(proto, k).CopyFrom(v)
            else:
                setattr(proto, k, v)


def make_proto_directive(name, sub_name=None, **kwargs):
    base = TDirective()
    directive = getattr(base, name)
    if sub_name is not None:
        directive.Name = sub_name
    fill_proto_fields(directive, kwargs)
    return base


@attr.s
class DirectivesTestData(object):
    vins_directive_name = attr.ib(type=unicode)
    vins_directive_fields = attr.ib(type=dict)
    proto_directive_name = attr.ib(type=unicode)
    proto_directive_fields = attr.ib(type=dict)

    def __str__(self):
        return self.proto_directive_name


def assert_serialize_client_directive(test_data):
    sub_name = 'sub_name'
    vins_directive = make_vins_directive(
        name=test_data.vins_directive_name,
        directive_type='client_action',
        sub_name=sub_name,
        **test_data.vins_directive_fields
    )
    proto_directive = make_proto_directive(
        name=test_data.proto_directive_name,
        sub_name=sub_name,
        **test_data.proto_directive_fields
    )
    expected, actual = serialize_directive(vins_directive), proto_directive
    assert expected == actual, 'expected != actual\n{}\n!=\n{}'.format(expected, actual)


@pytest.mark.parametrize('test_data', [
    DirectivesTestData(
        'timer_stop_playing', {'timer_id': 'timer_id'}, 'TimerStopPlayingDirective', {'TimerId': 'timer_id'}
    ),
    DirectivesTestData(
        'pause_timer', {'timer_id': 'timer_id'},
        'PauseTimerDirective', {'TimerId': 'timer_id'}
    ),
    DirectivesTestData(
        'navigate_browser', {'command_name': 'clear_history'},
        'NavigateBrowserDirective', {'Command': TNavigateBrowserDirective.ECommand.ClearHistory}
    ),
    DirectivesTestData(
        'navigate_browser', {'command_name': 'close_browser'},
        'NavigateBrowserDirective', {'Command': TNavigateBrowserDirective.ECommand.CloseBrowser}
    ),
    DirectivesTestData(
        'navigate_browser', {'command_name': 'go_home'},
        'NavigateBrowserDirective', {'Command': TNavigateBrowserDirective.ECommand.GoHome}
    ),
    DirectivesTestData(
        'navigate_browser', {'command_name': 'new_tab'},
        'NavigateBrowserDirective', {'Command': TNavigateBrowserDirective.ECommand.NewTab}
    ),
    DirectivesTestData(
        'navigate_browser', {'command_name': 'open_bookmarks_manager'},
        'NavigateBrowserDirective', {'Command': TNavigateBrowserDirective.ECommand.OpenBookmarksManager}
    ),
    DirectivesTestData(
        'navigate_browser', {'command_name': 'open_history'},
        'NavigateBrowserDirective', {'Command': TNavigateBrowserDirective.ECommand.OpenHistory}
    ),
    DirectivesTestData(
        'navigate_browser', {'command_name': 'open_incognito_mode'},
        'NavigateBrowserDirective', {'Command': TNavigateBrowserDirective.ECommand.OpenIncognitoMode}
    ),
    DirectivesTestData(
        'navigate_browser', {'command_name': 'restore_tab'},
        'NavigateBrowserDirective', {'Command': TNavigateBrowserDirective.ECommand.RestoreTab}
    ),
    DirectivesTestData(
        'radio_play', {
            'active': True,
            'available': True,
            'color': '#0071BB',
            'frequency': '102.9',
            'imageUrl': 'avatars.mds.yandex.net/get-music-misc/28592/komsomolskaya_pravda-225/%%',
            'officialSiteUrl': 'http://mariafm.ru',
            'radioId': 'nashe',
            'score': 0.4435845617,
            'showRecognition': True,
            'streamUrl': 'https://strm.yandex.ru/fm/fm_dacha_main/fm_dacha_main0.m3u8',
            'title': 'Радио Комсомольская правда',
            'alarm_id': 'deadface-4487-421e-866d-a3f087990a34',
        },
        'RadioPlayDirective', {
            'IsActive': True,
            'IsAvailable': True,
            'Color': '#0071BB',
            'Frequency': '102.9',
            'ImageUrl': 'avatars.mds.yandex.net/get-music-misc/28592/komsomolskaya_pravda-225/%%',
            'OfficialSiteUrl': 'http://mariafm.ru',
            'RadioId': 'nashe',
            'Score': 0.4435845617,
            'ShowRecognition': True,
            'StreamUrl': 'https://strm.yandex.ru/fm/fm_dacha_main/fm_dacha_main0.m3u8',
            'Title': 'Радио Комсомольская правда',
            'AlarmId': 'deadface-4487-421e-866d-a3f087990a34',
        },
    ),
    DirectivesTestData(
        'radio_play', {
            'active': True,
            'available': True,
            'color': '#0071BB',
            'frequency': '102.9',
            'imageUrl': 'avatars.mds.yandex.net/get-music-misc/28592/komsomolskaya_pravda-225/%%',
            'officialSiteUrl': 'http://mariafm.ru',
            'radioId': 'nashe',
            'score': 0.4435845617,
            'showRecognition': True,
            'streamUrl': 'https://strm.yandex.ru/fm/fm_dacha_main/fm_dacha_main0.m3u8',
            'title': 'Радио Комсомольская правда',
        },
        'RadioPlayDirective', {
            'IsActive': True,
            'IsAvailable': True,
            'Color': '#0071BB',
            'Frequency': '102.9',
            'ImageUrl': 'avatars.mds.yandex.net/get-music-misc/28592/komsomolskaya_pravda-225/%%',
            'OfficialSiteUrl': 'http://mariafm.ru',
            'RadioId': 'nashe',
            'Score': 0.4435845617,
            'ShowRecognition': True,
            'StreamUrl': 'https://strm.yandex.ru/fm/fm_dacha_main/fm_dacha_main0.m3u8',
            'Title': 'Радио Комсомольская правда',
        },
    ),
    DirectivesTestData(
        'show_tv_gallery', {
            'items': [{
                'channel_type': 'personal',
                'description': 'Эфир этого канала формируется автоматически на основании '
                               'ваших предпочтений — того, что вы любите смотреть.',
                'name': 'Мой Эфир',
                'provider_item_id': '4461546c4debdcffbab506fd75246e19',
                'provider_name': 'strm',
                'relevance': 100500,
                'thumbnail_url_16x9': 'https://avatars.mds.yandex.net/'
                                      'get-vh/1583218/2a0000016ad564e3344385fcfa3a92eec7f4/640x360',
                'thumbnail_url_16x9_small': 'https://avatars.mds.yandex.net/'
                                            'get-vh/1583218/2a0000016ad564e3344385fcfa3a92eec7f4/640x360',
                'tv_episode_name': 'Пол это Лава в Роблокс! '
                                   'Котёнок Лайк против Котика Игромана / Roblox The Floor is Lava',
                'tv_stream_info': {
                    'channel_type': 'personal',
                    'deep_hd': 0,
                    'is_personal': 1,
                    'tv_episode_id': '45f85199853131d2b50b3c78410b5c59',
                    'tv_episode_name': 'Пол это Лава в Роблокс! '
                                       'Котёнок Лайк против Котика Игромана / Roblox The Floor is Lava'
                },
                'type': 'tv_stream'
            }]
        },
        'ShowTvGalleryDirective', {
            'Items': [{
                'ChannelType': 'personal',
                'Description': 'Эфир этого канала формируется автоматически на основании '
                               'ваших предпочтений — того, что вы любите смотреть.',
                'Name': 'Мой Эфир',
                'ProviderItemId': '4461546c4debdcffbab506fd75246e19',
                'ProviderName': 'strm',
                'Relevance': 100500,
                'ThumbnailUrl16x9': 'https://avatars.mds.yandex.net/'
                                    'get-vh/1583218/2a0000016ad564e3344385fcfa3a92eec7f4/640x360',
                'ThumbnailUrl16x9Small': 'https://avatars.mds.yandex.net/'
                                         'get-vh/1583218/2a0000016ad564e3344385fcfa3a92eec7f4/640x360',
                'TvEpisodeName': 'Пол это Лава в Роблокс! '
                                 'Котёнок Лайк против Котика Игромана / Roblox The Floor is Lava',
                'TvStreamInfo': {
                    'ChannelType': 'personal',
                    'DeepHD': 0,
                    'IsPersonal': 1,
                    'TvEpisodeId': '45f85199853131d2b50b3c78410b5c59',
                    'TvEpisodeName': 'Пол это Лава в Роблокс! '
                                     'Котёнок Лайк против Котика Игромана / Roblox The Floor is Lava'
                },
                'Type': 'tv_stream'
            }]
        }
    ),
    DirectivesTestData(
        'go_down', {},
        'GoDownDirective', {}
    ),
    DirectivesTestData(
        'go_top', {},
        'GoTopDirective', {}
    ),
    DirectivesTestData(
        'go_up', {},
        'GoUpDirective', {}
    ),
    DirectivesTestData(
        'player_pause', {'smooth': True},
        'PlayerPauseDirective', {'Smooth': True},
    ),
    DirectivesTestData(
        'screen_off', {},
        'ScreenOffDirective', {},
    ),
    DirectivesTestData(
        'set_timer', {
            'directives': [
                {
                    'name': 'player_pause',
                    'type': 'client_action',
                    'payload': {
                        'smooth': True
                    }
                },
                {
                    'name': 'go_home',
                    'type': 'client_action'
                },
                {
                    'name': 'screen_off',
                    'type': 'client_action'
                }
            ],
            'timestamp': 42
        },
        'SetTimerDirective', {
            'Timestamp': 42,
            'Directives': [
                {
                    'PlayerPauseDirective': {
                        'Name': '',
                        'Smooth': True
                    },
                },
                {
                    'GoHomeDirective': {
                        'Name': '',
                    }
                },
                {
                    'ScreenOffDirective': {
                        'Name': '',
                    }
                },
            ],
        },
    ),
    DirectivesTestData(
        'video_play', {
            'item': {
                'available': 1,
                'description': 'Михалков - власть, гимн, BadComedian / вДудь - YouTube. Открывай счет для бизнеса в Альфа-Банке...',
                'duration': 6548,
                'name': 'Михалков - власть, гимн, BadComedian',
                'next_items': [
                    {
                        'available': 1,
                        'description': 'вДудь VS Михалков..!  - YouTube. сергей михеев,николай стариков,владимир соловьёв,анатолий шарий,александр семченко,дмитрий пучков,геополитика...',
                        'duration': 2398,
                        'name': 'вДудь VS Михалков..!',
                        'play_uri': 'youtube://pl0vRkF8YWY',
                        'price_from': 4,
                        'provider_info': [
                            {
                                'available': 1,
                                'provider_item_id': 'pl0vRkF8YWY',
                                'provider_name': 'youtube',
                                'type': 'video'
                            }
                        ],
                        'provider_item_id': 'pl0vRkF8YWY',
                        'provider_name': 'youtube',
                        'source_host': 'www.youtube.com',
                        'thumbnail_url_16x9': 'https://avatars.mds.yandex.net/get-vthumb/903110/bf75f87d3ca3b056853e531e6d6f6e5b/800x360',
                        'thumbnail_url_16x9_small': 'https://avatars.mds.yandex.net/get-vthumb/903110/bf75f87d3ca3b056853e531e6d6f6e5b/800x360',
                        'type': 'video',
                        'view_count': 1
                    },
                    {
                        'available': 1,
                        'description': 'Михалков, Дудь, Путин. Михалков,Дудь,Путин.',
                        'duration': 313,
                        'name': 'Дудь загнал Михалкова в угол вопросами про Путина - фрагмент интервью у Дудя',
                        'play_uri': 'youtube://eQsqt55soek',
                        'price_from': 15,
                        'provider_info': [
                            {
                                'available': 1,
                                'provider_item_id': 'eQsqt55soek',
                                'provider_name': 'youtube',
                                'type': 'video'
                            }
                        ],
                        'provider_item_id': 'eQsqt55soek',
                        'provider_name': 'youtube',
                        'source_host': 'www.youtube.com',
                        'thumbnail_url_16x9': 'https://avatars.mds.yandex.net/get-vthumb/876582/90f2f04e641de50f896ae4cb2c131a25/800x360',
                        'thumbnail_url_16x9_small': 'https://avatars.mds.yandex.net/get-vthumb/876582/90f2f04e641de50f896ae4cb2c131a25/800x360',
                        'type': 'video',
                        'view_count': 2
                    }
                ],
                'normalized_name': 'михалков - власть гимн badcomedian english subs',
                'play_uri': 'youtube://6cjcgu865ok',
                'provider_info': [
                    {
                        'available': 1,
                        'provider_item_id': '6cjcgu865ok',
                        'provider_name': 'youtube',
                        'type': 'video'
                    }
                ],
                'provider_item_id': '6cjcgu865ok',
                'provider_name': 'youtube',
                'source_host': 'www.youtube.com',
                'thumbnail_url_16x9': 'https://avatars.mds.yandex.net/get-vthumb/752126/4e091aa4f646ef479e2f6aebd37b11ba/800x360',
                'thumbnail_url_16x9_small': 'https://avatars.mds.yandex.net/get-vthumb/752126/4e091aa4f646ef479e2f6aebd37b11ba/800x360',
                'type': 'video',
                'view_count': 3
            },
            'next_item': {
                'available': 1,
                'description': 'вДудь VS Михалков..!  - YouTube. сергей михеев,николай стариков,владимир соловьёв,анатолий шарий,александр семченко,дмитрий пучков,геополитика...',
                'duration': 2398,
                'name': 'вДудь VS Михалков..!',
                'play_uri': 'youtube://pl0vRkF8YWY',
                'price_from': 4,
                'provider_info': [
                    {
                        'available': 1,
                        'provider_item_id': 'pl0vRkF8YWY',
                        'provider_name': 'youtube',
                        'type': 'video'
                    }
                ],
                'provider_item_id': 'pl0vRkF8YWY',
                'provider_name': 'youtube',
                'source_host': 'www.youtube.com',
                'thumbnail_url_16x9': 'https://avatars.mds.yandex.net/get-vthumb/903110/bf75f87d3ca3b056853e531e6d6f6e5b/800x360',
                'thumbnail_url_16x9_small': 'https://avatars.mds.yandex.net/get-vthumb/903110/bf75f87d3ca3b056853e531e6d6f6e5b/800x360',
                'type': 'video',
                'view_count': 5
            },
            'uri': 'youtube://6cjcgu865ok'
        },
        'VideoPlayDirective', {
            'Name': 'sub_name',
            'Uri': 'youtube://6cjcgu865ok',
            'Item': {
                'Type': 'video',
                'ProviderName': 'youtube',
                'ProviderItemId': '6cjcgu865ok',
                'Available': 1,
                'ThumbnailUrl16x9': 'https://avatars.mds.yandex.net/get-vthumb/752126/4e091aa4f646ef479e2f6aebd37b11ba/800x360',
                'ThumbnailUrl16x9Small': 'https://avatars.mds.yandex.net/get-vthumb/752126/4e091aa4f646ef479e2f6aebd37b11ba/800x360',
                'Name': 'Михалков - власть, гимн, BadComedian',
                'NormalizedName': 'михалков - власть гимн badcomedian english subs',
                'Description': 'Михалков - власть, гимн, BadComedian / вДудь - YouTube. Открывай счет для бизнеса в Альфа-Банке...',
                'Duration': 6548,
                'SourceHost': 'www.youtube.com',
                'ViewCount': 3,
                'PlayUri': 'youtube://6cjcgu865ok',
                'ProviderInfo': [{
                    'Type': 'video',
                    'ProviderName': 'youtube',
                    'ProviderItemId': '6cjcgu865ok',
                    'Available': 1,
                }],
                'NextItems': [{
                    'Type': 'video',
                    'ProviderName': 'youtube',
                    'ProviderItemId': 'pl0vRkF8YWY',
                    'Available': 1,
                    'PriceFrom': 4,
                    'ThumbnailUrl16x9': 'https://avatars.mds.yandex.net/get-vthumb/903110/bf75f87d3ca3b056853e531e6d6f6e5b/800x360',
                    'ThumbnailUrl16x9Small': 'https://avatars.mds.yandex.net/get-vthumb/903110/bf75f87d3ca3b056853e531e6d6f6e5b/800x360',
                    'Name': 'вДудь VS Михалков..!',
                    'Description': 'вДудь VS Михалков..!  - YouTube. сергей михеев,николай стариков,владимир соловьёв,анатолий шарий,александр семченко,дмитрий пучков,геополитика...',
                    'Duration': 2398,
                    'SourceHost': 'www.youtube.com',
                    'ViewCount': 1,
                    'PlayUri': 'youtube://pl0vRkF8YWY',
                    'ProviderInfo': [{
                        'Type': 'video',
                        'ProviderName': 'youtube',
                        'ProviderItemId': 'pl0vRkF8YWY',
                        'Available': 1,
                    }],
                }, {
                    'Type': 'video',
                    'ProviderName': 'youtube',
                    'ProviderItemId': 'eQsqt55soek',
                    'Available': 1,
                    'PriceFrom': 15,
                    'ThumbnailUrl16x9': 'https://avatars.mds.yandex.net/get-vthumb/876582/90f2f04e641de50f896ae4cb2c131a25/800x360',
                    'ThumbnailUrl16x9Small': 'https://avatars.mds.yandex.net/get-vthumb/876582/90f2f04e641de50f896ae4cb2c131a25/800x360',
                    'Name': 'Дудь загнал Михалкова в угол вопросами про Путина - фрагмент интервью у Дудя',
                    'Description': 'Михалков, Дудь, Путин. Михалков,Дудь,Путин.',
                    'Duration': 313,
                    'SourceHost': 'www.youtube.com',
                    'ViewCount': 2,
                    'PlayUri': 'youtube://eQsqt55soek',
                    'ProviderInfo': [{
                        'Type': 'video',
                        'ProviderName': 'youtube',
                        'ProviderItemId': 'eQsqt55soek',
                        'Available': 1,
                    }],
                }],
            },
            'NextItem': {
                'Type': 'video',
                'ProviderName': 'youtube',
                'ProviderItemId': 'pl0vRkF8YWY',
                'Available': 1,
                'PriceFrom': 4,
                'ThumbnailUrl16x9': 'https://avatars.mds.yandex.net/get-vthumb/903110/bf75f87d3ca3b056853e531e6d6f6e5b/800x360',
                'ThumbnailUrl16x9Small': 'https://avatars.mds.yandex.net/get-vthumb/903110/bf75f87d3ca3b056853e531e6d6f6e5b/800x360',
                'Name': 'вДудь VS Михалков..!',
                'Description': 'вДудь VS Михалков..!  - YouTube. сергей михеев,николай стариков,владимир соловьёв,анатолий шарий,александр семченко,дмитрий пучков,геополитика...',
                'Duration': 2398,
                'SourceHost': 'www.youtube.com',
                'ViewCount': 5,
                'PlayUri': 'youtube://pl0vRkF8YWY',
                'ProviderInfo': [{
                    'Type': 'video',
                    'ProviderName': 'youtube',
                    'ProviderItemId': 'pl0vRkF8YWY',
                    'Available': 1,
                }],
            },
        },
    ),
    DirectivesTestData(
        'alarm_set_sound', {
            'server_action': {
                'name': 'TestInnerAnalyticsType',
                'payload': {},
                'type': 'server_action',
                'ignore_answer': True
            },
            'sound_alarm_setting': {
                'info': {
                    'active': True,
                    'album': {
                        'genre': 'local-indie',
                        'id': '4006712',
                        'title': 'Петь'
                    },
                    'artists': [
                        {
                            'composer': True,
                            'id': '2561847',
                            'is_various': True,
                            'name': 'Владимир Шахрин'
                        }
                    ],
                    'available': True,
                    'color': '#FF0000',
                    'coverUri': 'https://avatars.yandex.net/get-music-content/98892/06aff6d0.a.4006712-1/200x200',
                    'filters': {
                        'activity': 'wake-up',
                        'epoch': 'nineties',
                        'genre': 'meditation',
                        'isNew': True,
                        'isPersonal': True,
                        'isPopular': True,
                        'language': [],
                        'mood': 'energetic',
                        'personality': 'is_personal',
                        'releaseYears': [],
                        'special_playlist': 'playlist_of_the_day',
                        'vocal': []
                    },
                    'first_track': {
                        'album': {
                            'genre': 'local-indie',
                            'id': '4006712',
                            'title': 'Петь'
                        },
                        'artists': [
                            {
                                'composer': True,
                                'id': '2561847',
                                'is_various': True,
                                'name': 'Владимир Шахрин'
                            }
                        ],
                        'coverUri': 'https://avatars.yandex.net/get-music-content/98892/06aff6d0.a.4006712-1/200x200',
                        'id': '32858088',
                        'subtype': 'music',
                        'title': 'Петь',
                        'type': 'track',
                        'uri': 'https://music.yandex.ru/album/4006712/track/32858088/?from=alice&mob=0'
                    },
                    'first_track_uri': 'http://music.yandex.ru/track/32858088',
                    'for_alarm': True,
                    'frequency': '103.4',
                    'genre': 'soundtrack',
                    'id': '32858088',
                    'imageUrl': 'avatars.mds.yandex.net/get-music-misc/49997/mayak-225/%%',
                    'name': 'LOBODA',
                    'officialSiteUrl': 'https://radiomayak.ru',
                    'partnerId': '139316',
                    'radioId': 'mayak',
                    'session_id': 'DB5yi6vE',
                    'showRecognition': True,
                    'streamUrl': 'https://strm.yandex.ru/fm/fm_mayak/fm_mayak0.m3u8',
                    'subtype': 'music',
                    'techName': 'fm_love_novosibirsk',
                    'title': 'Петь',
                    'type': 'track',
                    'uri': 'https://music.yandex.ru/album/4006712/track/32858088/?from=alice&mob=0',
                    'uuid': '4dbcbdb4ae87d073a23a8d47cb7e35ab'
                },
                'repeat': True,
                'type': 'music'
            }
        },
        'AlarmSetSoundDirective', {
            'Callback': {
                'Name': 'TestInnerAnalyticsType',
                'IgnoreAnswer': True,
                'Payload': struct_pb2.Struct(),
            },
            'Settings': {
                'Info': {
                    'Active': True,
                    'Available': True,
                    'Color': '#FF0000',
                    'Filters': json_format.ParseDict({
                        'activity': 'wake-up',
                        'epoch': 'nineties',
                        'genre': 'meditation',
                        'isNew': True,
                        'isPersonal': True,
                        'isPopular': True,
                        'language': [],
                        'mood': 'energetic',
                        'personality': 'is_personal',
                        'releaseYears': [],
                        'special_playlist': 'playlist_of_the_day',
                        'vocal': []
                    }, struct_pb2.Struct()),
                    'FirstTrack': {
                        'Id': '32858088',
                        'Title': 'Петь',
                        'Type': 'track',
                        'Subtype': 'music',
                        'CoverUri': 'https://avatars.yandex.net/get-music-content/98892/06aff6d0.a.4006712-1/200x200',
                        'Uri': 'https://music.yandex.ru/album/4006712/track/32858088/?from=alice&mob=0',
                        'Album': {
                            'Id': '4006712',
                            'Title': 'Петь',
                            'Genre': 'local-indie',
                        },
                        'Artists': [{
                            'Id': '2561847',
                            'Name': 'Владимир Шахрин',
                            'Composer': True,
                            'IsVarious': True,
                        }],
                    },
                    'FirstTrackUri': 'http://music.yandex.ru/track/32858088',
                    'ForAlarm': True,
                    'Frequency': '103.4',
                    'Genre': 'soundtrack',
                    'ImageUrl': 'avatars.mds.yandex.net/get-music-misc/49997/mayak-225/%%',
                    'Name': 'LOBODA',
                    'OfficialSiteUrl': 'https://radiomayak.ru',
                    'PartnerId': '139316',
                    'RadioId': 'mayak',
                    'SessionId': 'DB5yi6vE',
                    'ShowRecognition': True,
                    'StreamUrl': 'https://strm.yandex.ru/fm/fm_mayak/fm_mayak0.m3u8',
                    'TechName': 'fm_love_novosibirsk',
                    'Uuid': '4dbcbdb4ae87d073a23a8d47cb7e35ab',
                    'Id': '32858088',
                    'Title': 'Петь',
                    'Type': 'track',
                    'Subtype': 'music',
                    'CoverUri': 'https://avatars.yandex.net/get-music-content/98892/06aff6d0.a.4006712-1/200x200',
                    'Uri': 'https://music.yandex.ru/album/4006712/track/32858088/?from=alice&mob=0',
                    'Album': {
                        'Id': '4006712',
                        'Title': 'Петь',
                        'Genre': 'local-indie',
                    },
                    'Artists': [{
                        'Id': '2561847',
                        'Name': 'Владимир Шахрин',
                        'Composer': True,
                        'IsVarious': True,
                    }],
                },
                'Repeat': True,
                'Type': 'music',
            },
        },
    ),
    DirectivesTestData(
        'alarm_stop', {},
        'AlarmStopDirective', {},
    ),
    DirectivesTestData(
        'alarm_set_max_level', {
            'new_level': 5,
        },
        'AlarmSetMaxLevelDirective', {
            'NewLevel': 5,
        },
    ),
    DirectivesTestData(
        'music_recognition', {
            "album": {
                "genre": "soundtrack",
                "id": "59592",
                "title": "The Matrix Reloaded: The Album"
            },
            "artists": [
                {
                    "composer": True,
                    "id": "1151",
                    "is_various": True,
                    "name": "Justin Timberlake"
                }
            ],
            "coverUri": "https://avatars.yandex.net/get-music-content/28589/a86f9db5.a.59592-1/200x200",
            "id": "555822",
            "subtype": "music",
            "title": "When The World Ends",
            "type": "track",
            "uri": "https://music.yandex.ru/album/59592/track/555822/?from=alice&mob=0&play=1",
        },
        'MusicRecognitionDirective', {
            "Album": {
                "Genre": "soundtrack",
                "Id": "59592",
                "Title": "The Matrix Reloaded: The Album"
            },
            "Artists": [
                {
                    "Composer": True,
                    "Id": "1151",
                    "IsVarious": True,
                    "Name": "Justin Timberlake"
                }
            ],
            "CoverUri": "https://avatars.yandex.net/get-music-content/28589/a86f9db5.a.59592-1/200x200",
            "Id": "555822",
            "Subtype": "music",
            "Title": "When The World Ends",
            "Type": "track",
            "Uri": "https://music.yandex.ru/album/59592/track/555822/?from=alice&mob=0&play=1",
        },
    ),
    DirectivesTestData(
        'player_continue', {'player': 'music'},
        'PlayerContinueDirective', {'Player': 'music'},
    ),
    DirectivesTestData(
        'player_dislike', {'uid': '907320517'},
        'PlayerDislikeDirective', {'Uid': '907320517'},
    ),
    DirectivesTestData(
        'player_like', {'uid': '907320517'},
        'PlayerLikeDirective', {'Uid': '907320517'},
    ),
    DirectivesTestData(
        'player_next_track', {'player': 'music', 'uid': '907320517'},
        'PlayerNextTrackDirective', {'Player': 'music', 'Uid': '907320517'},
    ),
    DirectivesTestData(
        'player_previous_track', {'player': 'music'},
        'PlayerPreviousTrackDirective', {'Player': 'music'},
    ),
    DirectivesTestData(
        'player_replay', {},
        'PlayerReplayDirective', {},
    ),
    DirectivesTestData(
        'player_rewind', {'amount': 10, 'type': 'forward'},
        'PlayerRewindDirective', {'Amount': 10, 'Type': TPlayerRewindDirective.EType.Forward},
    ),
    DirectivesTestData(
        'player_shuffle', {},
        'PlayerShuffleDirective', {},
    ),
    DirectivesTestData(
        'show_pay_push_screen', {
            'item': {
                'actors': '',
                'availability_request': {
                    'kinopoisk': {
                        'id': '4ac4dc48495e91d3a36f459617973cac'
                    },
                    'type': 'film'
                },
                'available': 1,
                'cover_url_16x9': 'http://avatars.mds.yandex.net/get-ott/223007/2a00000163bbdd86b76f17d504375c0891a5/1920x1080',
                'cover_url_2x3': 'http://avatars.mds.yandex.net/get-ott/374297/2a0000016f32a9ae7121216229ccebb8c279/328x492',
                'debug_info': {
                    'web_page_url': 'http://www.kinopoisk.ru/film/689774'
                },
                'description': '',
                'directors': '',
                'duration': 360,
                'episode': 10,
                'episodes_count': 154,
                'genre': 'мультфильм, мюзикл, фэнтези, комедия, приключения, семейный',
                'human_readable_id': '',
                'min_age': 18,
                'misc_ids': {
                    'kinopoisk': '689774'
                },
                'name': 'Фиксики - Сезон 1 - Серия 10 - Воздушный шар',
                'normalized_name': 'тролли',
                'provider_info': [
                    {
                        'human_readable_id': '',
                        'misc_ids': {
                            'kinopoisk': '689774'
                        },
                        'provider_item_id': '4ac4dc48495e91d3a36f459617973cac',
                        'provider_name': 'kinopoisk',
                        'type': 'movie'
                    }
                ],
                'provider_item_id': '41ed03fbe5b5c8388ac126f14dfebc7c',
                'provider_name': 'kinopoisk',
                'provider_number': 10,
                'rating': 6.948999882,
                'release_year': 2016,
                'season': 1,
                'seasons_count': 1,
                'soon': 0,
                'source': 'video_source_entity_search',
                'thumbnail_url_16x9': 'http://avatars.mds.yandex.net/get-ott/223007/2a00000163bbdd86b76f17d504375c0891a5/672x438',
                'thumbnail_url_16x9_small': 'http://avatars.mds.yandex.net/get-ott/223007/2a00000163bbdd86b76f17d504375c0891a5/88x88',
                'thumbnail_url_2x3_small': 'http://avatars.mds.yandex.net/get-ott/374297/2a0000016f32a9ae7121216229ccebb8c279/132x132',
                'tv_show_item_id': '45b8b70edc565908b50f4cb800324baa',
                'tv_show_season_id': '4eeaeb3984bd30a7aaad85a097481c8c',
                'type': 'tv_show_episode',
                'unauthorized': 0
            },
            'tv_show_item': {
                'actors': 'Дмитрий Назаров, Лариса Брохман, Инна Королёва',
                'cover_url_16x9': 'http://avatars.mds.yandex.net/get-ott/212840/2a00000163abeda240320b8681786af61eb6/1920x1080',
                'cover_url_2x3': 'http://avatars.mds.yandex.net/get-ott/374297/2a0000016f32a9ae7121216229ccebb8c279/328x492',
                'debug_info': {
                    'web_page_url': 'http://www.kinopoisk.ru/film/581653'
                },
                'description': '«А кто такие фиксики — большой-большой секрет!» — так поётся в песенке про фиксиков...',
                'directors': 'Васико Бедошвили, Андрей Колпин, Сергей Меринов',
                'genre': 'мультфильм, детский',
                'human_readable_id': '',
                'min_age': 18,
                'misc_ids': {
                    'kinopoisk': '581653'
                },
                'name': 'Фиксики',
                'progress': 0,
                'provider_info': [
                    {
                        'human_readable_id': '',
                        'misc_ids': {
                            'kinopoisk': '464963'
                        },
                        'provider_item_id': '47bab88d43ac0a82ad62bfbbaf302e07',
                        'provider_name': 'kinopoisk',
                        'type': 'tv_show'
                    }
                ],
                'provider_item_id': '45b8b70edc565908b50f4cb800324baa',
                'provider_name': 'kinopoisk',
                'rating': 6.948999882,
                'release_year': 2015,
                'seasons_count': 1,
                'thumbnail_url_16x9': 'http://avatars.mds.yandex.net/get-ott/212840/2a00000163abeda240320b8681786af61eb6/672x438',
                'thumbnail_url_16x9_small': 'http://avatars.mds.yandex.net/get-ott/212840/2a00000163abeda240320b8681786af61eb6/88x88',
                'thumbnail_url_2x3_small': 'http://avatars.mds.yandex.net/get-ott/374297/2a0000016f32a9ae7121216229ccebb8c279/132x132',
                'type': 'tv_show'
            }
        },
        'ShowPayPushScreenDirective', {
            'Name': 'sub_name',
            'Item': {
                'Type': 'tv_show_episode',
                'ProviderName': 'kinopoisk',
                'ProviderItemId': '41ed03fbe5b5c8388ac126f14dfebc7c',
                'TvShowSeasonId': '4eeaeb3984bd30a7aaad85a097481c8c',
                'TvShowItemId': '45b8b70edc565908b50f4cb800324baa',
                'MiscIds': {
                    'Kinopoisk': '689774',
                },
                'Available': 1,
                'Episode': 10,
                'Season': 1,
                'ProviderNumber': 10,
                'CoverUrl2x3': 'http://avatars.mds.yandex.net/get-ott/374297/2a0000016f32a9ae7121216229ccebb8c279/328x492',
                'CoverUrl16x9': 'http://avatars.mds.yandex.net/get-ott/223007/2a00000163bbdd86b76f17d504375c0891a5/1920x1080',
                'ThumbnailUrl2x3Small': 'http://avatars.mds.yandex.net/get-ott/374297/2a0000016f32a9ae7121216229ccebb8c279/132x132',
                'ThumbnailUrl16x9': 'http://avatars.mds.yandex.net/get-ott/223007/2a00000163bbdd86b76f17d504375c0891a5/672x438',
                'ThumbnailUrl16x9Small': 'http://avatars.mds.yandex.net/get-ott/223007/2a00000163bbdd86b76f17d504375c0891a5/88x88',
                'Name': 'Фиксики - Сезон 1 - Серия 10 - Воздушный шар',
                'NormalizedName': 'тролли',
                'Duration': 360,
                'Genre': 'мультфильм, мюзикл, фэнтези, комедия, приключения, семейный',
                'Rating': 6.948999882,
                'SeasonsCount': 1,
                'EpisodesCount': 154,
                'ReleaseYear': 2016,
                'ProviderInfo': [{
                    'Type': 'movie',
                    'ProviderName': 'kinopoisk',
                    'ProviderItemId': '4ac4dc48495e91d3a36f459617973cac',
                    'MiscIds': {
                        'Kinopoisk': '689774',
                    },
                }],
                'MinAge': 18,
                'DebugInfo': {
                    'WebPageUrl': 'http://www.kinopoisk.ru/film/689774',
                },
                'AvailabilityRequest': {
                    'Type': 'film',
                    'Kinopoisk': {
                        'Id': '4ac4dc48495e91d3a36f459617973cac',
                    },
                },
                'Source': 'video_source_entity_search',
            },
            'TvShowItem': {
                'Type': 'tv_show',
                'ProviderName': 'kinopoisk',
                'ProviderItemId': '45b8b70edc565908b50f4cb800324baa',
                'MiscIds': {
                    'Kinopoisk': '581653',
                },
                'CoverUrl2x3': 'http://avatars.mds.yandex.net/get-ott/374297/2a0000016f32a9ae7121216229ccebb8c279/328x492',
                'CoverUrl16x9': 'http://avatars.mds.yandex.net/get-ott/212840/2a00000163abeda240320b8681786af61eb6/1920x1080',
                'ThumbnailUrl2x3Small': 'http://avatars.mds.yandex.net/get-ott/374297/2a0000016f32a9ae7121216229ccebb8c279/132x132',
                'ThumbnailUrl16x9': 'http://avatars.mds.yandex.net/get-ott/212840/2a00000163abeda240320b8681786af61eb6/672x438',
                'ThumbnailUrl16x9Small': 'http://avatars.mds.yandex.net/get-ott/212840/2a00000163abeda240320b8681786af61eb6/88x88',
                'Name': 'Фиксики',
                'Description': '«А кто такие фиксики — большой-большой секрет!» — так поётся в песенке про фиксиков...',
                'Genre': 'мультфильм, детский',
                'Rating': 6.948999882,
                'SeasonsCount': 1,
                'ReleaseYear': 2015,
                'Directors': 'Васико Бедошвили, Андрей Колпин, Сергей Меринов',
                'Actors': 'Дмитрий Назаров, Лариса Брохман, Инна Королёва',
                'ProviderInfo': [{
                    'Type': 'tv_show',
                    'ProviderName': 'kinopoisk',
                    'ProviderItemId': '47bab88d43ac0a82ad62bfbbaf302e07',
                    'MiscIds': {
                        'Kinopoisk': '464963',
                    },
                }],
                'MinAge': 18,
                'DebugInfo': {
                    'WebPageUrl': 'http://www.kinopoisk.ru/film/581653',
                },
            },
        },
    ),
    DirectivesTestData(
        'show_description', {
            'item': {
                'actors': 'Леонардо ДиКаприо, Марк Руффало, Бен Кингсли',
                'availability_request': {
                    'kinopoisk': {
                        'id': '443c9a2dee446db0b8d3a6d7f930528e'
                    },
                    'type': 'film'
                },
                'available': 1,
                'cover_url_16x9': 'https://avatars.mds.yandex.net/get-vh/108198/7928789928936366275-z53cPscw5RLxvtb5GUICuQ-1530048627/1920x1080',
                'cover_url_2x3': 'https://avatars.mds.yandex.net/get-vh/1632697/7928789928936366275-V8pff8fXvQvgeOwWdzOBOg-1562827166/320x480',
                'debug_info': {
                    'web_page_url': 'http://www.kinopoisk.ru/film/397667'
                },
                'description': 'Молодого бизнесмена подозревают в убийстве возлюбленной...',
                'directors': 'Мартин Скорсезе',
                'duration': 6368,
                'genre': 'триллер, детектив, драма',
                'human_readable_id': '',
                'min_age': 16,
                'misc_ids': {
                    'kinopoisk': '397667'
                },
                'name': 'Невидимый гость',
                'play_uri': 'https://strm.yandex.ru/vh-ott-converted/ott-content/481949986-4a545c51c54a3d84a8686549901d031c/master.hd_quality.m3u8',
                'progress': 0,
                'provider_info': [
                    {
                        'available': 1,
                        'provider_item_id': '437cd6a3df3efdbc9b71ac6f18ade72b',
                        'provider_name': 'kinopoisk',
                        'type': 'movie'
                    }
                ],
                'provider_item_id': '4a545c51c54a3d84a8686549901d031c',
                'provider_name': 'kinopoisk',
                'rating': 7.75,
                'release_year': 2016,
                'thumbnail_url_16x9': 'https://avatars.mds.yandex.net/get-vh/108198/7928789928936366275-z53cPscw5RLxvtb5GUICuQ-1530048627/640x360',
                'thumbnail_url_16x9_small': 'https://avatars.mds.yandex.net/get-vh/108198/7928789928936366275-z53cPscw5RLxvtb5GUICuQ-1530048627/640x360',
                'thumbnail_url_2x3_small': 'http://avatars.mds.yandex.net/get-ott/1534341/2a00000170151491ba54bce0e12066cc94a2/132x132',
                'type': 'movie',
                'unauthorized': 0
            }
        },
        'ShowVideoDescriptionDirective', {
            'Name': 'sub_name',
            'Item': {
                'Type': 'movie',
                'ProviderName': 'kinopoisk',
                'ProviderItemId': '4a545c51c54a3d84a8686549901d031c',
                'MiscIds': {
                    'Kinopoisk': '397667',
                },
                'Available': 1,
                'CoverUrl2x3': 'https://avatars.mds.yandex.net/get-vh/1632697/7928789928936366275-V8pff8fXvQvgeOwWdzOBOg-1562827166/320x480',
                'CoverUrl16x9': 'https://avatars.mds.yandex.net/get-vh/108198/7928789928936366275-z53cPscw5RLxvtb5GUICuQ-1530048627/1920x1080',
                'ThumbnailUrl2x3Small': 'http://avatars.mds.yandex.net/get-ott/1534341/2a00000170151491ba54bce0e12066cc94a2/132x132',
                'ThumbnailUrl16x9': 'https://avatars.mds.yandex.net/get-vh/108198/7928789928936366275-z53cPscw5RLxvtb5GUICuQ-1530048627/640x360',
                'ThumbnailUrl16x9Small': 'https://avatars.mds.yandex.net/get-vh/108198/7928789928936366275-z53cPscw5RLxvtb5GUICuQ-1530048627/640x360',
                'Name': 'Невидимый гость',
                'Description': 'Молодого бизнесмена подозревают в убийстве возлюбленной...',
                'Duration': 6368,
                'Genre': 'триллер, детектив, драма',
                'Rating': 7.75,
                'ReleaseYear': 2016,
                'Directors': 'Мартин Скорсезе',
                'Actors': 'Леонардо ДиКаприо, Марк Руффало, Бен Кингсли',
                'PlayUri': 'https://strm.yandex.ru/vh-ott-converted/ott-content/481949986-4a545c51c54a3d84a8686549901d031c/master.hd_quality.m3u8',
                'ProviderInfo': [{
                    'Type': 'movie',
                    'ProviderName': 'kinopoisk',
                    'ProviderItemId': '437cd6a3df3efdbc9b71ac6f18ade72b',
                    'Available': 1,
                }],
                'MinAge': 16,
                'DebugInfo': {
                    'WebPageUrl': 'http://www.kinopoisk.ru/film/397667',
                },
                'AvailabilityRequest': {
                    'Type': 'film',
                    'Kinopoisk': {
                        'Id': '443c9a2dee446db0b8d3a6d7f930528e',
                    },
                },
            },
        },
    ),
    DirectivesTestData(
        'show_season_gallery', {
            'items': [
                {
                    'availability_request': {
                        'kinopoisk': {
                            'id': '4ec3e509124f2b3da4ab028e697e4ba0',
                            'season_id': '4f802f2fb0858db88666035e2fba8a60',
                            'tv_show_id': '4849c158c2870a9d8201faede0c6a511'
                        },
                        'type': 'episode'
                    },
                    'duration': 1383,
                    'episode': 26,
                    'human_readable_id': '',
                    'name': 'Щенячий патруль - Сезон 1 - Серия 26 - Щенки и сокровища пиратов',
                    'provider_info': [
                        {
                            'episode': 26,
                            'human_readable_id': '',
                            'provider_item_id': '4ec3e509124f2b3da4ab028e697e4ba0',
                            'provider_name': 'kinopoisk',
                            'provider_number': 26,
                            'season': 1,
                            'tv_show_item_id': '4849c158c2870a9d8201faede0c6a511',
                            'tv_show_season_id': '4f802f2fb0858db88666035e2fba8a60',
                            'type': 'tv_show_episode'
                        }
                    ],
                    'provider_item_id': '4ec3e509124f2b3da4ab028e697e4ba0',
                    'provider_name': 'kinopoisk',
                    'provider_number': 26,
                    'season': 1,
                    'seasons_count': 1,
                    'soon': 0,
                    'thumbnail_url_16x9': 'http://avatars.mds.yandex.net/get-ott/212840/2a0000016395f0333e059da3eb63e5846587/672x438',
                    'tv_show_item_id': '4849c158c2870a9d8201faede0c6a511',
                    'tv_show_season_id': '4f802f2fb0858db88666035e2fba8a60',
                    'type': 'tv_show_episode'
                }
            ],
            'season': 3,
            'tv_show_item': {
                'cover_url_2x3': 'http://avatars.mds.yandex.net/get-ott/1672343/2a0000016d5e6754bdcd2c1ce835e6adb70f/328x492',
                'debug_info': {
                    'web_page_url': 'http://www.kinopoisk.ru/film/1011528'
                },
                'genre': 'драма',
                'human_readable_id': '',
                'misc_ids': {
                    'kinopoisk': '1011528'
                },
                'name': 'Хороший доктор',
                'progress': 0,
                'provider_info': [
                    {
                        'human_readable_id': '',
                        'misc_ids': {
                            'kinopoisk': '464963'
                        },
                        'provider_item_id': '47bab88d43ac0a82ad62bfbbaf302e07',
                        'provider_name': 'kinopoisk',
                        'type': 'tv_show'
                    }
                ],
                'provider_item_id': '4f57cd386c268e9983cb28644c4adfce',
                'provider_name': 'kinopoisk',
                'release_year': 2017,
                'seasons': [
                    {
                        'number': 1
                    },
                ],
                'seasons_count': 3,
                'type': 'tv_show'
            }
        },
        'ShowSeasonGalleryDirective', {
            'Name': 'sub_name',
            'Items': [{
                'Type': 'tv_show_episode',
                'ProviderName': 'kinopoisk',
                'ProviderItemId': '4ec3e509124f2b3da4ab028e697e4ba0',
                'TvShowSeasonId': '4f802f2fb0858db88666035e2fba8a60',
                'TvShowItemId': '4849c158c2870a9d8201faede0c6a511',
                'Episode': 26,
                'Season': 1,
                'ProviderNumber': 26,
                'ThumbnailUrl16x9': 'http://avatars.mds.yandex.net/get-ott/212840/2a0000016395f0333e059da3eb63e5846587/672x438',
                'Name': 'Щенячий патруль - Сезон 1 - Серия 26 - Щенки и сокровища пиратов',
                'Duration': 1383,
                'SeasonsCount': 1,
                'ProviderInfo': [{
                    'Type': 'tv_show_episode',
                    'ProviderName': 'kinopoisk',
                    'ProviderItemId': '4ec3e509124f2b3da4ab028e697e4ba0',
                    'TvShowSeasonId': '4f802f2fb0858db88666035e2fba8a60',
                    'TvShowItemId': '4849c158c2870a9d8201faede0c6a511',
                    'Episode': 26,
                    'Season': 1,
                    'ProviderNumber': 26,
                }],
                'AvailabilityRequest': {
                    'Type': 'episode',
                    'Kinopoisk': {
                        'Id': '4ec3e509124f2b3da4ab028e697e4ba0',
                        'SeasonId': '4f802f2fb0858db88666035e2fba8a60',
                        'TvShowId': '4849c158c2870a9d8201faede0c6a511',
                    },
                },
            }],
            'TvShowItem': {
                'Type': 'tv_show',
                'ProviderName': 'kinopoisk',
                'ProviderItemId': '4f57cd386c268e9983cb28644c4adfce',
                'MiscIds': {
                    'Kinopoisk': '1011528',
                },
                'CoverUrl2x3': 'http://avatars.mds.yandex.net/get-ott/1672343/2a0000016d5e6754bdcd2c1ce835e6adb70f/328x492',
                'Name': 'Хороший доктор',
                'Genre': 'драма',
                'SeasonsCount': 3,
                'ReleaseYear': 2017,
                'ProviderInfo': [{
                    'Type': 'tv_show',
                    'ProviderName': 'kinopoisk',
                    'ProviderItemId': '47bab88d43ac0a82ad62bfbbaf302e07',
                    'MiscIds': {
                        'Kinopoisk': '464963',
                    },
                }],
                'Seasons': [{
                    'Number': 1,
                }],
                'DebugInfo': {
                    'WebPageUrl': 'http://www.kinopoisk.ru/film/1011528',
                },
            },
            'Season': 3,
        },
    ),
    DirectivesTestData(
        'alarm_set_sound', {
            'server_action': {
                'name': 'bass_action',
                'payload': {
                    'data': {
                        'object': {
                            'id': '19152669',
                            'type': 'track'
                        }
                    },
                    'name': 'quasar.music_play_object'
                },
                'type': 'server_action'
            },
            'sound_alarm_setting': {
                'info': {
                    'album': {
                        'genre': 'alternative',
                        'id': '9186476',
                        'title': 'Believers Never Die',
                        'type': 'Single',
                        'SOME_INVALID_FIELD': 'value',
                    },
                    'artists': [
                        {
                            'composer': True,
                            'id': '5976',
                            'is_various': True,
                            'name': 'Fall Out Boy'
                        }
                    ],
                    'coverUri': 'https://avatars.yandex.net/get-music-content/2399641/9712d381.a.9186476-1/200x200',
                    'id': '19152669',
                    'subtype': 'music',
                    'title': 'Centuries',
                    'type': 'track',
                    'uri': 'https://music.yandex.ru/album/9186476/track/19152669/?from=alice&mob=0'
                },
                'type': 'music'
            }
        },
        'AlarmSetSoundDirective', {
            'Callback': {
                'Name': 'bass_action',
                'Payload': json_format.ParseDict({
                    'data': {
                        'object': {
                            'id': '19152669',
                            'type': 'track'
                        }
                    },
                    'name': 'quasar.music_play_object'
                }, struct_pb2.Struct()),
            },
            'Settings': {
                'Info': {
                    'Id': '19152669',
                    'Title': 'Centuries',
                    'Type': 'track',
                    'Subtype': 'music',
                    'CoverUri': 'https://avatars.yandex.net/get-music-content/2399641/9712d381.a.9186476-1/200x200',
                    'Uri': 'https://music.yandex.ru/album/9186476/track/19152669/?from=alice&mob=0',
                    'Album': {
                        'Id': '9186476',
                        'Title': 'Believers Never Die',
                        'Genre': 'alternative',
                        'Type': 'Single',
                    },
                    'Artists': [{
                        'Id': '5976',
                        'Name': 'Fall Out Boy',
                        'Composer': True,
                        'IsVarious': True,
                    }],
                },
                'Type': 'music',
            },
        },
    ),
    DirectivesTestData(
        'send_bug_report', {
            'id': 'abacabadabacaba',
        },
        'SendBugReportDirective', {
            'RequestId': 'abacabadabacaba',
        }
    ),
    DirectivesTestData(
        'open_disk', {
            'disk': 'f',
        },
        'OpenDiskDirective', {
            'Disk': 'f',
        },
    ),
    DirectivesTestData(
        'start_image_recognizer', {},
        'StartImageRecognizerDirective', {},
    ),
    DirectivesTestData(
        'start_image_recognizer', {
            'camera_type': 'front',
        },
        'StartImageRecognizerDirective', {
            'CameraType': 'front',
        },
    ),
    DirectivesTestData(
        'start_image_recognizer', {
            'image_search_mode': 1,
            'image_search_mode_name': 'voice_text',
        },
        'StartImageRecognizerDirective', {
            'ImageSearchMode': 1,
            'ImageSearchModeName': 'voice_text',
        },
    ),
    DirectivesTestData(
        'start_image_recognizer', {
            'camera_type': 'front',
            'image_search_mode': 1,
            'image_search_mode_name': 'voice_text',
        },
        'StartImageRecognizerDirective', {
            'CameraType': 'front',
            'ImageSearchMode': 1,
            'ImageSearchModeName': 'voice_text',
        },
    ),
    DirectivesTestData(
        'mordovia_show', {
            'scenario': 'ether',
            'splash_div': '{"my":"json"}',
            'url': 'http://yandex.com',
            'callback_prototype': {
                'name': 'bass_action',
                'payload': {
                    'name': 'quasar.music_play_object'
                },
                'type': 'server_action'
            },
            'is_full_screen': True,
        },
        'MordoviaShowDirective', {
            'ViewKey': 'ether',
            'SplashDiv': '{"my":"json"}',
            'Url': 'http://yandex.com',
            'IsFullScreen': True,
            'CallbackPrototype': {
                'Name': 'bass_action',
                'Payload': json_format.ParseDict({
                    'name': 'quasar.music_play_object'
                }, struct_pb2.Struct()),
            }
        },
    ),
    DirectivesTestData(
        'mordovia_command', {
            'command': 'take',
            'meta': {
                'gold': '100 unc',
                'silver': '3 tonn',
            },
            'view_key': 'ether'
        },
        'MordoviaCommandDirective', {
            'ViewKey': 'ether',
            'Command': 'take',
            'Meta': json_format.ParseDict({
                'gold': '100 unc',
                'silver': '3 tonn',
            }, struct_pb2.Struct())
        },
    ),
    DirectivesTestData(
        'tts_play_placeholder', {},
        'TtsPlayPlaceholderDirective', {},
    ),
    DirectivesTestData(
        'draw_led_screen', {
            'animation_sequence': [
                {
                    'frontal_led_image': 'https://quasar.s3.yandex.net/led_screen/cloud-3.gif',
                },
                {
                    'frontal_led_image': 'https://quasar.s3.yandex.net/led_screen/cloud-4.gif',
                    'endless': True,
                },
            ],
            'till_end_of_speech': True,
        },
        'DrawLedScreenDirective', {
            'DrawItem': [
                {
                    'FrontalLedImage': 'https://quasar.s3.yandex.net/led_screen/cloud-3.gif',
                },
                {
                    'FrontalLedImage': 'https://quasar.s3.yandex.net/led_screen/cloud-4.gif',
                    'Endless': True,
                },
            ],
            'TillEndOfSpeech': True,
        },
    ),
    DirectivesTestData(
        'show_buttons', {
            'buttons': [
                {
                    'action_id': 'ac8670bf-5ea61b1a-cb02f7a-795132b3',
                    'text': 'У меня есть много сказок. Про кого хотите послушать?',
                    'theme': {
                        'image_url': 'https://avatars.mds.yandex.net/get-dialogs/1525540/onboard_FairyTale/mobile-logo-x2'
                    },
                    'title': 'Сказки',
                },
                {
                    'action_id': 'cbcf1bde-2ac91d44-db2112ca-c9da955f',
                    'text': 'Найду лучший маршрут из А в Б.',
                    'theme': {
                        'image_url': 'https://avatars.mds.yandex.net/get-dialogs/1017510/onboard_Route/mobile-logo-x2'
                    },
                    'title': 'Построй маршрут',
                },
                {
                    'action_id': 'af8d7617-6e791d14-42cdb6ba-19e58fee',
                    'text': '🏆Игра прятки - победитель премии Алисы!',
                    'theme': {
                        'image_url': 'https://avatars.mds.yandex.net/get-dialogs/1027858/ee124dce95b341e28c00/mobile-logo-x2'
                    },
                    'title': 'Игра прятки'
                },
            ],
            'screen_id': 'cloud_ui',
        },
        'ShowButtonsDirective', {
            'Buttons': [
                {
                    'ActionId': 'ac8670bf-5ea61b1a-cb02f7a-795132b3',
                    'Text': 'У меня есть много сказок. Про кого хотите послушать?',
                    'Theme': {
                        'ImageUrl': 'https://avatars.mds.yandex.net/get-dialogs/1525540/onboard_FairyTale/mobile-logo-x2'
                    },
                    'Title': 'Сказки',
                },
                {
                    'ActionId': 'cbcf1bde-2ac91d44-db2112ca-c9da955f',
                    'Text': 'Найду лучший маршрут из А в Б.',
                    'Theme': {
                        'ImageUrl': 'https://avatars.mds.yandex.net/get-dialogs/1017510/onboard_Route/mobile-logo-x2'
                    },
                    'Title': 'Построй маршрут',
                },
                {
                    'ActionId': 'af8d7617-6e791d14-42cdb6ba-19e58fee',
                    'Text': '🏆Игра прятки - победитель премии Алисы!',
                    'Theme': {
                        'ImageUrl': 'https://avatars.mds.yandex.net/get-dialogs/1027858/ee124dce95b341e28c00/mobile-logo-x2'
                    },
                    'Title': 'Игра прятки',
                },
            ],
            'ScreenId': 'cloud_ui',
        },
    ),
    DirectivesTestData(
        'reminders_set_directive',
        {
            'id': 'guid',
            'text': 'remind',
            'epoch': '12345',
            'timezone': 'Europe/Moscow',
            'on_success_callback': {
                'name': 'reminders_on_success_callback',
                'type': 'server_action',
                'payload': {
                    'success': True,
                    'type': 'Creation'
                }
            },
            'on_fail_callback': {
                'name': 'reminders_on_fail_callback',
                'type': 'server_action',
                'payload': {
                    'success': False,
                    'type': 'Creation'
                }
            },
            'on_shoot_frame': {
                'typed_semantic_frame': {
                    'reminders_on_shoot_semantic_frame': {
                        'epoch': {'epoch_value': '12345'},
                        'id': {'string_value': 'guid'},
                        'text': {'string_value': 'remind'},
                        'timezone': {'string_value': 'Europe/Moscow'}
                    }
                },
                'analytics': {
                    'origin': 'Scenario',
                    'origin_info': '',
                    'product_scenario': 'reminders',
                    'purpose': 'on_shoot'
                },
            }
        },
        'RemindersSetDirective',
        {
            'Id': 'guid',
            'Text': 'remind',
            'Epoch': '12345',
            'TimeZone': 'Europe/Moscow',
            'OnSuccessCallback': {
                'Name': 'reminders_on_success_callback',
                'Payload': json_format.ParseDict({
                    'type': 'Creation',
                    'success': True
                }, struct_pb2.Struct()),
            },
            'OnFailCallback': {
                'Name': 'reminders_on_fail_callback',
                'Payload': json_format.ParseDict({
                    'type': 'Creation',
                    'success': False
                }, struct_pb2.Struct()),
            },
            'OnShootFrame': {
                'Analytics': {
                    'Origin': 2,
                    'OriginInfo': '',
                    'ProductScenario': 'reminders',
                    'Purpose': 'on_shoot'
                },
                'TypedSemanticFrame': {
                    'RemindersOnShootSemanticFrame': {
                        'Id': {'StringValue': 'guid'},
                        'Text': {'StringValue': 'remind'},
                        'Epoch': {'EpochValue': '12345'},
                        'TimeZone': {'StringValue': 'Europe/Moscow'}
                    }
                }
            }
        }
    ),
    DirectivesTestData(
        'reminders_cancel_directive',
        {
            'id': ['guid'],
            'action': 'id',
            'on_success_callback': {
                'name': 'reminders_on_success_callback',
                'type': 'server_action',
                'payload': {
                    'success': True,
                    'type': 'Cancelation'
                }
            },
            'on_fail_callback': {
                'name': 'reminders_on_fail_callback',
                'type': 'server_action',
                'payload': {
                    'success': False,
                    'type': 'Cancelation'
                }
            }
        },
        'RemindersCancelDirective',
        {
            'Ids': ['guid'],
            'Action': 'id',
            'OnSuccessCallback': {
                'Name': 'reminders_on_success_callback',
                'Payload': json_format.ParseDict({
                    'type': 'Cancelation',
                    'success': True
                }, struct_pb2.Struct()),
            },
            'OnFailCallback': {
                'Name': 'reminders_on_fail_callback',
                'Payload': json_format.ParseDict({
                    'type': 'Cancelation',
                    'success': False
                }, struct_pb2.Struct()),
            }
        }
    ),
    DirectivesTestData(
        'draw_scled_animations',
        {
            'animations': [
                {
                    'name': 'animation_1',
                    'base64_encoded_value': 'aHR0cHM6Ly93d3cueW91dHViZS5jb20vd2F0Y2g/dj1kY3EtNDdGa3BfNA==',
                    'compression_type': 'gzip'
                }
            ],
            'animation_stop_policy': 'play_once'
        },
        'DrawScledAnimationsDirective',
        {
            'Animations': [
                {
                    'Name': 'animation_1',
                    'Base64EncodedValue': 'aHR0cHM6Ly93d3cueW91dHViZS5jb20vd2F0Y2g/dj1kY3EtNDdGa3BfNA==',
                    'Compression': TDrawScledAnimationsDirective.TAnimation.ECompressionType.Gzip
                }
            ],
            'AnimationStopPolicy': TDrawScledAnimationsDirective.EAnimationStopPolicy.PlayOnce
        }
    )
])
def test_serialize_client_directives(test_data):
    assert_serialize_client_directive(test_data)


@pytest.mark.parametrize('test_data', [
    DirectivesTestData(
        'save_voiceprint', {
            'requests': [
                'bf0d764f-064d-411f-916a-98f4d4fc8b60',
                'f3ec7027-60f7-4972-93ef-d6a6ce8a984b',
                'd3421dfa-84eb-4d20-8b40-b559b9c21da0',
                '9db13883-fd5d-4ea3-abe6-7f6fa2bb7838',
                'e57a7c3e-a248-4737-a602-8e6352b0f1d9',
            ],
            'user_id': '687820164',
        },
        'SaveVoiceprintDirective', {
            'UserId': '687820164',
            'RequestIds': [
                'bf0d764f-064d-411f-916a-98f4d4fc8b60',
                'f3ec7027-60f7-4972-93ef-d6a6ce8a984b',
                'd3421dfa-84eb-4d20-8b40-b559b9c21da0',
                '9db13883-fd5d-4ea3-abe6-7f6fa2bb7838',
                'e57a7c3e-a248-4737-a602-8e6352b0f1d9',
            ],
        },
    ),
    DirectivesTestData(
        'remove_voiceprint', {
            'user_id': '851083725',
        },
        'RemoveVoiceprintDirective', {
            'UserId': '851083725',
        },
    ),
])
def test_serialize_uniproxy_directives(test_data):
    vins_directive = make_vins_directive(
        name=test_data.vins_directive_name,
        directive_type='uniproxy_action',
        **test_data.vins_directive_fields
    )
    proto_directive = make_proto_directive(
        name=test_data.proto_directive_name,
        **test_data.proto_directive_fields
    )
    expected, actual = serialize_directive(vins_directive), proto_directive
    assert expected == actual, 'expected != actual\n{}\n!=\n{}'.format(expected, actual)


def test_serialize_open_settings_directive():
    for target, enum_target in SETTINGS_TARGET_MAPPING.iteritems():
        assert_serialize_client_directive(DirectivesTestData(
            'open_settings', {
                'target': target,
            },
            'OpenSettingsDirective', {
                'Target': enum_target,
            }
        ))


def test_parse_gc_meta():
    assert parse_gc_meta(meta=[]) is None

    actual = parse_gc_meta(meta=[
        ExternalSkillMeta(deactivating=False, skill_name='Чат с Алисой'),
        GeneralConversationMeta(pure_gc=True),
        GeneralConversationSourceMeta(source='source'),
    ])

    expected = TVinsGcMeta()
    expected.Intent = 'personal_assistant.scenarios.external_skill_gc'
    expected.IsPureGc = True
    expected.Source = 'source'

    assert expected == actual


def test_req_with_gc_meta(sk_client):
    request = TInput()
    request.Text.Utterance = 'давай поболтаем'
    request.Text.RawUtterance = request.Text.Utterance

    response = _run_request(sk_client, _create_run_request(request))
    assert response.status_code == 200
    response = TScenarioRunResponse.FromString(response.content)

    found = False
    for obj in response.ResponseBody.AnalyticsInfo.Objects:
        payload = getattr(obj, obj.WhichOneof('Payload'))
        if isinstance(payload, TVinsGcMeta):
            found = True
            assert payload.Intent == 'personal_assistant.scenarios.external_skill_gc'
            assert payload.IsPureGc

    assert found


def test_req_with_error_meta(sk_client):
    request = TInput()
    request.Text.Utterance = 'дай ошибку'
    request.Text.RawUtterance = request.Text.Utterance

    response = _run_request(sk_client, _create_run_request(request))
    assert response.status_code == 200
    response = TScenarioRunResponse.FromString(response.content)

    found = False
    for obj in response.ResponseBody.AnalyticsInfo.Objects:
        payload = getattr(obj, obj.WhichOneof('Payload'))
        if isinstance(payload, TVinsErrorMeta):
            found = True
            assert payload.Type == 'noresponse'

    assert found


def fill_asr_result_word(word, value, confidence=1.0):
    word.Value = value
    word.Confidence = confidence


def test_voice_input_with_biometry():
    expected = RequestEvent.from_dict({
        'type': 'voice_input',
        'asr_result': [
            {
                'confidence': 0.8,
                'utterance': 'a 1',
                'normalized': 'a 1',
                'words': [{'confidence': 1.0, 'value': 'a'}, {'confidence': 1.0, 'value': 'one'}]
            },
            {
                'confidence': 0.9,
                'utterance': 'b',
                'normalized': 'b',
                'words': [{'confidence': 1.0, 'value': 'b'}]
            },
        ],
        'biometry_scoring': {
            'status': 'ok',
            'scores': [{'user_id': '123', 'score': 1.0}],
            'request_id': 'req',
            'group_id': 'grp',
            'scores_with_mode': [
                {
                    'mode': 'default',
                    'scores': [{'user_id': '321', 'score': 0.1}],
                },
                {
                    'mode': 'max_accuracy',
                    'scores': [],  # required field
                },
            ],
            'partial_number': 0
        },
        'biometry_classification': {
            'status': 'ok',
            'simple': [
                {
                    'tag': 'children',
                    'classname': 'child',
                }
            ],
            'scores': [],
            'partial_number': 0
        }
    })

    request = TScenarioRunRequest()
    voice = request.Input.Voice

    asr_result = voice.AsrData.add()
    asr_result.Confidence = 0.8
    asr_result.Utterance = 'a 1'
    asr_result.Normalized = 'a 1'
    fill_asr_result_word(asr_result.Words.add(), value='a')
    fill_asr_result_word(asr_result.Words.add(), value='one')

    asr_result = voice.AsrData.add()
    asr_result.Confidence = 0.9
    asr_result.Utterance = 'b'
    asr_result.Normalized = 'b'
    fill_asr_result_word(asr_result.Words.add(), value='b')

    biometry_scoring = voice.BiometryScoring
    biometry_scoring.Status = 'ok'
    biometry_scoring.RequestId = 'req'
    biometry_scoring.GroupId = 'grp'
    score = biometry_scoring.Scores.add()
    score.UserId = '123'
    score.Score = 1

    score_with_mode = biometry_scoring.ScoresWithMode.add()
    score_with_mode.Mode = 'default'
    score = score_with_mode.Scores.add()
    score.UserId = '321'
    score.Score = 0.1

    score_with_mode = biometry_scoring.ScoresWithMode.add()
    score_with_mode.Mode = 'max_accuracy'

    biometry_classification = voice.BiometryClassification
    biometry_classification.Status = 'ok'
    simple_classification = biometry_classification.Simple.add()
    simple_classification.Tag = 'children'
    simple_classification.ClassName = 'child'

    assert get_event(request).to_dict() == expected.to_dict()


def test_no_biometry_scoring():
    expected = RequestEvent.from_dict({
        'type': 'voice_input',
        'asr_result': [
            {
                'confidence': 0.8,
                'utterance': 'a 1',
                'normalized': 'a 1',
                'words': [{'confidence': 1.0, 'value': 'a'}, {'confidence': 1.0, 'value': 'one'}],
            },
            {
                'confidence': 0.9,
                'utterance': 'b',
                'normalized': 'b',
                'words': [{'confidence': 1.0, 'value': 'b'}],
            },
        ],
    })
    request = TScenarioRunRequest()
    voice = request.Input.Voice

    asr_result = voice.AsrData.add()
    asr_result.Confidence = 0.8
    asr_result.Utterance = 'a 1'
    asr_result.Normalized = 'a 1'
    fill_asr_result_word(asr_result.Words.add(), value='a')
    fill_asr_result_word(asr_result.Words.add(), value='one')

    asr_result = voice.AsrData.add()
    asr_result.Confidence = 0.9
    asr_result.Utterance = 'b'
    asr_result.Normalized = 'b'
    fill_asr_result_word(asr_result.Words.add(), value='b')

    assert get_event(request).to_dict() == expected.to_dict()


def compare_features(actual, expected):
    assert isinstance(actual, list)

    diff = expected - set(actual)
    assert len(diff) == 0, 'unable to find following features ["{}"]'.format('", "'.join(diff))


def test_get_supported_features():
    interfaces = TInterfaces()
    interfaces.HasReliableSpeakers = False
    interfaces.HasBluetooth = False
    interfaces.HasAccessToBatteryPowerState = True
    interfaces.HasCEC = False
    interfaces.HasMicrophone = False
    interfaces.HasMusicPlayerShots = True

    supported_expected = {
        'no_reliable_speakers',
        'no_bluetooth',
        'battery_power_state',
        'no_microphone',
        'music_player_allow_shots',
    }

    unsupported_expected = {
        'cec_available',
        'change_alarm_sound',
        'set_alarm',
        'set_timer',
        'open_link',
        'open_link_turboapp',
        'synchronized_push_implementation',
        'tts_play_placeholder',
        'video_protocol',
        'quasar_screen',
    }

    supported_actual, unsupported_actual = get_supported_features(interfaces)
    compare_features(supported_actual, supported_expected)
    compare_features(unsupported_actual, unsupported_expected)


def test_get_features():
    wizard_rules = {
        'AliceRequest': {
            'IsPASkills': '0',
        },
        'Text': {
            'RequestLenTruncated': '0'
        },
    }
    entity_search = {'entity': {'key': 'value'}}

    run_request = TScenarioRunRequest(DataSources={
        VINS_WIZARD_RULES: TDataSource(VinsWizardRules=TVinsWizardRules(RawJson=json.dumps(wizard_rules))),
        ENTITY_SEARCH: TDataSource(EntitySearch=TEntitySearch(RawJson=json.dumps(entity_search))),
        BEGEMOT_EXTERNAL_MARKUP: TDataSource(BegemotExternalMarkup=TBegemotExternalMarkup(OriginalRequest='Req')),
    })

    expected = {
        'wizard': {
            'rules': wizard_rules,
            'markup': {
                'Date': [],
                'Delimiters': [],
                'Fio': [],
                'GeoAddr': [],
                'GeoAddrRoute': [],
                'MeasurementUnits': [],
                'Morph': [],
                'OriginalRequest': 'Req',
                'ProcessedRequest': '',
                'Tokens': [],
            }
        },
        'entity_search': entity_search,
    }
    assert get_features(run_request.DataSources) == expected


def test_data_sources():
    user_info = TBlackBoxUserInfo()
    user_info.Uid = '1234'
    user_info.Email = 'e-mail'

    request_input = TInput()
    request_input.Text.RawUtterance = 'query'
    request = _create_run_request(request_input)
    request.DataSources[BLACK_BOX].CopyFrom(TDataSource(UserInfo=user_info))

    req_info = create_req_info(request, get_event(request))
    expected = {
        '2': {
            'user_info': {
                'uid': '1234',
                'email': 'e-mail',
                'firstName': '',
                'hasMusicSubscription': False,
                'musicSubscriptionRegionId': 0L,
                'hasYandexPlus': False,
                'isBetaTester': False,
                'isStaff': False,
                'lastName': '',
                'phone': ''
            }
        }
    }
    assert req_info.data_sources == expected


def test_permissions():
    request_input = TInput()
    request_input.Text.RawUtterance = 'check permissions'
    request = _create_run_request(request_input)
    permission = request.BaseRequest.Options.Permissions.add()
    permission.Name = 'location'
    permission.Granted = True

    permission = request.BaseRequest.Options.Permissions.add()
    permission.Name = 'call'
    permission.Granted = False

    req_info = create_req_info(request, get_event(request))

    # according to https://a.yandex-team.ru/arc/trunk/arcadia/alice/vins/apps/personal_assistant/personal_assistant/api/personal_assistant.py?rev=6040721#L289  # noqa
    assert req_info.additional_options['permissions'] == [{
        'name': 'location',
        'status': True
    }, {
        'name': 'call',
        'status': False
    }]


def test_user_ticket():
    request_input = TInput()
    request_input.Text.RawUtterance = 'check permissions'
    request = _create_run_request(request_input)
    req_info = create_req_info(request, get_event(request), headers=Headers.from_dict({'X-YA-USER-TICKET': b'ticket'}))
    assert req_info.additional_options['user_ticket'] == 'ticket'


@pytest.mark.parametrize('params, srcrwr', [
    ({}, {}),
    (
        {
            'x-srcrwr': ['a:b', 'b:c'],
            'srcrwr': 'c:d:port',
        },
        {
            'a': 'b',
            'b': 'c',
            'c': 'd:port',
        },
    ),
    (
        {
            'srcrwr': 'b%3Ahttp%3A%2F%2Ftest.foo%2Fbar%3Fbaz%3Dquz',
        },
        {
            'b': 'http://test.foo/bar?baz=quz',
        }
    ),
])
def test_parse_srcrwr(params, srcrwr):
    assert parse_srcrwr(params) == srcrwr


def test_suggest(sk_client):
    request_input = TInput()
    request_input.Text.RawUtterance = 'дай саджест'
    request = _create_run_request(request_input)

    response = _run_request(sk_client, request)
    response = TScenarioRunResponse.FromString(response.content)

    response_body = response.ResponseBody
    suggest = response_body.Layout.SuggestButtons[0].ActionButton
    assert suggest.Title == 'test suggest'
    directive = response_body.FrameActions[suggest.ActionId].Directives.List[0]
    assert directive.OpenUriDirective.Uri == 'https://ya.ru/'
    assert len(response_body.Layout.Suggests) == 0


def test_search_suggest(sk_client):
    request_input = TInput()
    request_input.Text.RawUtterance = 'дай поисковый саджест'
    request = _create_run_request(request_input)

    response = _run_request(sk_client, request)
    response = TScenarioRunResponse.FromString(response.content)

    response_body = response.ResponseBody
    suggest = response_body.Layout.SuggestButtons[0].SearchButton
    assert suggest.Title.decode('utf-8') == 'Погода завтра какая будет...'
    assert suggest.Query.decode('utf-8') == 'Погода завтра какая будет скажи мне пожалуйста'


def test_image_capture_mode():
    modes = {
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

    request_input = TInput()
    request_input.Image.Url = 'img'
    request = _create_run_request(request_input)
    event = get_event(request)
    assert 'capture_mode' not in event.payload

    for mode, expectedMode in modes.iteritems():
        request_input.Image.CaptureMode = mode
        request = _create_run_request(request_input)
        event = get_event(request)
        assert event.payload['capture_mode'] == expectedMode


def test_get_form_from_update_form():
    semantic_frame = TSemanticFrame()
    slot = semantic_frame.Slots.add()
    slot.Type = 'string'
    slot.Value = 'какое-то значение'
    slot.Name = 'query'
    slot = semantic_frame.Slots.add()
    slot.Type = 'sys.site'
    slot.Value = '{"serp": {"url": "значение"}}'
    slot.Name = 'search_results'
    slot = semantic_frame.Slots.add()
    slot.Type = 'string'
    slot.Value = '{"data": {"ключ": "значение"}}'
    slot.Name = 'unicode_data'
    slot = semantic_frame.Slots.add()
    slot.Type = 'sys.num'
    slot.Value = '42'
    slot.Name = 'number'
    slot = semantic_frame.Slots.add()
    slot.Type = 'custom.day_part'
    slot.Value = 'morning'
    slot.Name = 'when'
    semantic_frame.Name = 'personal_assistant.scenarios.search'

    payload = get_form_from_semantic_frame(semantic_frame)
    assert payload == {
        'slots': [{
            'type': 'string',
            'value': 'какое-то значение',
            'name': 'query',
        }, {
            'type': 'site',
            'value': {
                'serp': {
                    'url': 'значение',
                },
            },
            'name': 'search_results',
        }, {
            'type': 'string',
            'value': {
                'data': {
                    'ключ': 'значение',
                }
            },
            'name': 'unicode_data',
        }, {
            'type': 'num',
            'value': 42,
            'name': 'number',
        }, {
            'type': 'day_part',
            'value': 'morning',
            'name': 'when',
        }],
        'name': 'personal_assistant.scenarios.search',
    }


def test_serialize_server_directive():
    history = {
        '': None,
        'last_card': None,
        'last_payment_method': {
            'id': 'cash',
            'type': 'cash'
        },
        'previous_tariff': ''
    }

    directive = ActionDirective.from_dict({
        'name': 'update_datasync',
        'payload': {
            'key': '/v1/personality/profile/alisa/kv/taxi_history',
            'listening_is_possible': True,
            'method': 'PUT',
            'value': history,
        },
        'type': 'uniproxy_action',
    })

    expected = TServerDirective()
    update_datasync = expected.UpdateDatasyncDirective
    update_datasync.Key = '/v1/personality/profile/alisa/kv/taxi_history'
    update_datasync.Method = TUpdateDatasyncDirective.EDataSyncMethod.Put
    update_datasync.StructValue.CopyFrom(dict_to_struct(history))

    actual = serialize_server_directive(directive)

    assert actual == expected

    directive = ActionDirective.from_dict({
        'name': 'update_datasync',
        'payload': {
            'key': '/v1/personality/profile/alisa/kv/taxi_history',
            'listening_is_possible': True,
            'method': 'PUT',
            'value': 'history',
        },
        'type': 'uniproxy_action',
    })

    expected.UpdateDatasyncDirective.StringValue = 'history'

    actual = serialize_server_directive(directive)

    assert actual == expected


def test_serialize_update_memento_directive():
    directive = ActionDirective.from_dict({
        'name': 'memento_change_user_objects_directive',
        'payload': {
            'protobuf': 'CmcKZQgXEmEKMnR5cGUuZ29vZ2xlYXBpcy5jb20vTkFsaWNlLk5EYXRhLk5SZW1pbmRlcnMuVFN0YXRlEisKKQoNMTIzNC00NTYtMTIzNBIEVGVzdCINRXVyb3BlL01vc2NvdzCVmu86'
        },
        'type': 'uniproxy_action',
    })

    expected = TServerDirective()
    memnto_update_directive = expected.MementoChangeUserObjectsDirective

    configPair = TConfigKeyAnyPair()
    configPair.Key = CK_REMINDERS

    reminder = TRemindersState.TReminder()
    reminder.Id = '1234-456-1234'
    reminder.Text = 'Test'
    reminder.ShootAt = 123456789
    reminder.TimeZone = 'Europe/Moscow'
    state = TRemindersState()
    state.Reminders.append(reminder)

    configPair.Value.Pack(state)
    memnto_update_directive.UserObjects.UserConfigs.append(configPair)

    actual = serialize_server_directive(directive)

    assert actual == expected


def test_serialize_send_push_directive():
    directive = ActionDirective.from_dict({
        'name': 'send_push',
        'payload': {
            'id': 'alice_video_buy',
            'personal_card_image_url': 'http://avatars.mds.yandex.net/get-ott/224348/2a0000016128aa76c17a22aa251619a76717/672x438',
            'personal_card_text': 'Гладиатор',
            'personal_card_title': 'Купить и смотреть',
            'personal_card_url': 'https://yandex.ru/video/quasar/billingLanding/?deviceid=123456&from=personal_card&uuids=4964f658870ba92086f5bc1f7d675331',
            'remove_existing_cards': False,
            'tag': 'alice_video_buy',
            'text': 'Нажмите для продолжения покупки',
            'throttle': 'bass-default-push',
            'title': 'Гладиатор',
            'ttl': 600,
            'url': 'https://yandex.ru/video/quasar/billingLanding/?deviceid=123456&from=push_message&uuids=4964f658870ba92086f5bc1f7d675331'
        },
        'type': 'client_action',
    })

    expected = TServerDirective()
    send_push = expected.SendPushDirective
    send_push.DoNotDeleteCardsWithSameTag = False
    send_push.PersonalCard.ImageUrl = 'http://avatars.mds.yandex.net/get-ott/224348/2a0000016128aa76c17a22aa251619a76717/672x438'
    send_push.PersonalCard.Settings.Link = 'https://yandex.ru/video/quasar/billingLanding/?deviceid=123456&from=personal_card&uuids=4964f658870ba92086f5bc1f7d675331'
    send_push.PersonalCard.Settings.Text = 'Гладиатор'
    send_push.PersonalCard.Settings.Title = 'Купить и смотреть'
    send_push.PushId = 'alice_video_buy'
    send_push.PushMessage.AppTypes.append(EAppType.AT_SEARCH_APP)
    send_push.PushMessage.Settings.Text = ''
    send_push.PushMessage.ThrottlePolicy = 'bass-default-push'
    send_push.PushTag = 'alice_video_buy'
    send_push.RemoveExistingCards = False
    send_push.Settings.Link = 'https://yandex.ru/video/quasar/billingLanding/?deviceid=123456&from=push_message&uuids=4964f658870ba92086f5bc1f7d675331'
    send_push.Settings.Text = 'Нажмите для продолжения покупки'
    send_push.Settings.Title = 'Гладиатор'
    send_push.Settings.TtlSeconds = 600

    actual = serialize_server_directive(directive)
    assert actual == expected


@pytest.mark.parametrize('original_form_name, fixed_form_name', [
    pytest.param('personal_assistant.scenarios.player.continue', 'personal_assistant.scenarios.player_continue', id='continue'),
    pytest.param('personal_assistant.scenarios.player.next_track', 'personal_assistant.scenarios.player_next_track', id='next_track'),
    pytest.param('personal_assistant.scenarios.player.what_is_playing', 'personal_assistant.scenarios.music_what_is_playing', id='what_is_playing'),
    pytest.param('personal_assistant.handcrafted.hello', 'personal_assistant.handcrafted.hello', id='no_rename'),
])
def test_player_command_forms_renaming(original_form_name, fixed_form_name):
    def build_req_info(input):
        input.SemanticFrames.add().Name = original_form_name
        request = _create_run_request(input)
        return create_req_info(request, get_event(request))

    text_input = TInput()
    text_input.Text.RawUtterance = 'Плеерная команда текст'
    assert build_req_info(text_input).semantic_frames[0]['name'] == original_form_name  # no renaming

    voice_input = TInput()
    voice_input.Voice.Utterance = 'Плеерная команда голос'
    asr_result = voice_input.Voice.AsrData.add()
    asr_result.Utterance = 'Плеерная команда голос'
    asr_result.Confidence = 1.0
    assert build_req_info(voice_input).semantic_frames[0]['name'] == original_form_name  # no renaming

    push_input = TInput()
    push_input.Text.RawUtterance = ''  # a push is a "text input" with empty utterance
    push_input.Text.Utterance = ''
    assert build_req_info(push_input).semantic_frames[0]['name'] == fixed_form_name  # has renaming


def _fill_last_play_timestamps(request, music_lpt, video_lpt, bluetooth_lpt, radio_lpt):
    if music_lpt is not None:
        request.BaseRequest.DeviceState.Music.LastPlayTimestamp = music_lpt
    if video_lpt is not None:
        request.BaseRequest.DeviceState.Video.LastPlayTimestamp = video_lpt
    if bluetooth_lpt is not None:
        request.BaseRequest.DeviceState.Bluetooth.LastPlayTimestamp = bluetooth_lpt
    if radio_lpt is not None:
        radio = struct_pb2.Struct()
        radio['last_play_timestamp'] = radio_lpt
        request.BaseRequest.DeviceState.Radio.CopyFrom(radio)


@pytest.mark.parametrize(
    'music_lpt, video_lpt, bluetooth_lpt, radio_lpt, seconds_since_pause, restore_player, player_type',
    [
        (0, 0, 0, 0, 0, False, None),

        (0, 1600703663000, 0, 0, 1, True, None),
        (1600703662000, 0, 0, 0, 2, True, None),
        (0, 0, 1600703661000, 0, 3, True, None),
        (0, 0, 0, 1600703660000, 4, True, None),

        (None, None, None, None, 0, False, None),
        (None, None, 1600703661000, None, 3, True, None),

        (1600703660000, 1600703650000, 0, 0, 4, True, None),
        (1600703630000, 1600703640000, 1600703635000, 0, 24, True, None),
        (1600703630000, 1600703640000, 1600703664000, 1600703635000, 0, True, None),
        (0, 0, 1500000000000, 0, 0, False, None),

        (0, 1600703663000, 0, 0, 0, False, 'music'),
        (1600703662000, 1600703663000, 0, 0, 2, True, 'music'),
        (1600703662000, 0, 0, 0, 0, False, 'video'),
        (1600703662000, 1600703661000, 0, 0, 3, True, 'video'),
    ]
)
def test_player_features(sk_client, music_lpt, video_lpt, bluetooth_lpt,
                         radio_lpt, seconds_since_pause, restore_player, player_type):
    utterance = {'music': 'следующую песню', 'video': 'следующий фильм'}.get(player_type, 'давай играй дальше')
    request_input = TInput()
    request_input.Text.RawUtterance = utterance
    request_input.Text.Utterance = utterance

    request = _create_run_request(request_input)

    request.BaseRequest.ClientInfo.ClientTime = '20200921T185424'
    request.BaseRequest.ClientInfo.Timezone = ''
    request.BaseRequest.ClientInfo.Epoch = '1600703664'

    _fill_last_play_timestamps(request, music_lpt, video_lpt, bluetooth_lpt, radio_lpt)

    response = _run_request(sk_client, request)
    response = TScenarioRunResponse.FromString(response.content)

    assert response.Features.PlayerFeatures.RestorePlayer == restore_player
    assert response.Features.PlayerFeatures.SecondsSincePause == seconds_since_pause


def test_favourite_locations():
    request_input = TInput()
    request_input.Text.RawUtterance = 'check favourites'
    request = _create_run_request(request_input)
    favourite = request.BaseRequest.Options.FavouriteLocations.add()
    favourite.Lat = 55.68447841439027
    favourite.Lon = 37.53408203922966
    favourite.SubTitle = 'Дом'
    favourite.Title = 'Home'

    favourite = request.BaseRequest.Options.FavouriteLocations.add()
    favourite.Lat = 55.889
    favourite.Lon = 37.6505
    favourite.Title = 'Work'

    req_info = create_req_info(request, get_event(request))

    assert req_info.additional_options['favourites'] == [{
        'lat': 55.68447841439027,
        'lon': 37.53408203922966,
        'subtitle': 'Дом',
        'title': 'Home',
    }, {
        'lat': 55.889,
        'lon': 37.6505,
        'title': 'Work',
    }]


def test_irrelevant_intent(sk_client):
    # Request to scenario with semantic frame
    request_input = TInput()
    request_input.Text.RawUtterance = 'not irrelevant'
    request_input.Text.Utterance = 'not irrelevant'
    request_input.SemanticFrames.add().Name = 'personal_assistant.scenarios.show_traffic__ellipsis'

    request = _create_run_request(request_input)
    response = _run_scenario_request('show_traffic', sk_client, request)
    assert response.status_code == 200
    response = TScenarioRunResponse.FromString(response.content)
    assert response.Features.Intent == 'personal_assistant.scenarios.common.irrelevant'
    assert response.Features.IsIrrelevant is True

    # Request to scenario without semantic frame
    request_input = TInput()
    request_input.Text.RawUtterance = 'not irrelevant'
    request_input.Text.Utterance = 'not irrelevant'

    request = _create_run_request(request_input)
    response = _run_scenario_request('show_traffic', sk_client, request)
    assert response.status_code == 200
    response = TScenarioRunResponse.FromString(response.content)
    assert response.Features.Intent == 'not_irrelevant'
    assert response.Features.IsIrrelevant is False

    # Request to Vins with semantic frame
    request_input = TInput()
    request_input.Text.RawUtterance = 'not irrelevant'
    request_input.Text.Utterance = 'not irrelevant'
    request_input.SemanticFrames.add().Name = 'personal_assistant.scenarios.show_traffic__ellipsis'

    request = _create_run_request(request_input)
    response = _run_request(sk_client, request)
    assert response.status_code == 200
    response = TScenarioRunResponse.FromString(response.content)
    assert response.Features.Intent == 'not_irrelevant'
    assert response.Features.IsIrrelevant is False

    # Request to Vins without semantic frame
    request_input = TInput()
    request_input.Text.RawUtterance = 'not irrelevant'
    request_input.Text.Utterance = 'not irrelevant'

    request = _create_run_request(request_input)
    response = _run_request(sk_client, request)
    assert response.status_code == 200
    response = TScenarioRunResponse.FromString(response.content)
    assert response.Features.Intent == 'not_irrelevant'
    assert response.Features.IsIrrelevant is False


def test_frame_actions(sk_client):
    request_input = TInput()
    request_input.Text.RawUtterance = 'дай фрейм экшны'
    request = _create_run_request(request_input)

    response = _run_request(sk_client, request)
    response = TScenarioRunResponse.FromString(response.content)

    response_body = response.ResponseBody
    frame_actions = response_body.FrameActions
    assert set(frame_actions) == {'id0', 'id1'}

    id0 = frame_actions['id0']
    assert len(id0.Directives.List) == 2
    assert id0.Directives.List[0].TypeTextDirective.Text == 'Skol\'ko ehat\' do doma?'
    assert id0.Directives.List[1].CallbackDirective.Name == 'on_skolko_ehat'
    assert id0.Directives.List[1].CallbackDirective.IgnoreAnswer
    assert id0.Directives.List[1].CallbackDirective.Payload['date'] == 'today'

    id1 = frame_actions['id1']
    assert len(id1.Directives.List) == 1
    assert id1.Directives.List[0].TypeTextSilentDirective.Text == 'Tiho!'


def test_scenario_data(sk_client):
    request_input = TInput()
    request_input.Text.RawUtterance = 'данные сценария'
    request = _create_run_request(request_input)

    response = _run_request(sk_client, request)
    response = TScenarioRunResponse.FromString(response.content)

    response_body = response.ResponseBody
    scenario_data = response_body.ScenarioData
    assert scenario_data.ExampleScenarioData.hello == 'test_hello'


def test_serialize_response_body(sk_client):
    text = 'yo ho ho'
    should_listen = True

    state = TState()
    vins_response = VinsResponse()
    vins_response.say(text)

    # div2 templates
    vins_response.templates = {'super_duper': {'var1': 1}}

    response = TScenarioResponseBody()
    serialize_response_body(vins_response, response, state, should_listen, None)

    layout = response.Layout

    assert layout
    assert layout.Cards[0].Text == text
    assert layout.OutputSpeech == text
    assert layout.ShouldListen == should_listen
    assert vins_response.templates == MessageToDict(layout.Div2Templates)
