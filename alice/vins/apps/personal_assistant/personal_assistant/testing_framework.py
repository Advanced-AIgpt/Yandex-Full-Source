# coding: utf-8
from __future__ import unicode_literals

import logging
import os
import re
import json
import attr
import yaml
import mock

from collections import OrderedDict
from copy import deepcopy, copy
from yaml.constructor import ConstructorError

from vins_core.dm.request_events import RequestEvent, TextInputEvent
from vins_core.utils.strings import smart_unicode
from vins_core.utils.data import (
    list_resource_files_with_prefix, open_resource_file, load_data_from_file, is_resource_exists
)
from vins_sdk.connectors import YANDEX_OFFICE_LOCATION
from vins_core.dm.request import AppInfo, configure_experiment_flags, Experiments, get_experiments
from vins_core.ext.entitysearch import EntitySearchHTTPAPI
from vins_core.utils.config import get_setting, get_bool_setting

from personal_assistant.api.personal_assistant import (PersonalAssistantAPI, ANY_BASS_QUERY,
                                                       ALL_SUPPORTED_BALANCER_TYPES, FAST_BASS_QUERY, SLOW_BASS_QUERY)
from vins_core.nlu.exact_match_token_tagger import ExactMatchTokenTagger
from vins_core.nlu.exact_match_classifier import ExactMatchClassifier

logger = logging.getLogger(__name__)

PYTEST_TO_STUB_REPLACEMENTS = [('::', '@'), ('=', '~')]
DEFAULT_LANG = 'ru'
TEST_REQUEST_ID = 'conftest-request-id'
APP_PRESETS_RESOURCE_KEY = '/load_app_infos/app_presets.yaml'
SPLIT_MODE_APP_PRESET_NAME = '@SPLIT_MODE'
IMPLICIT_APP_PRESET_NAME = '@IMPLICIT'  # for 'custom' or 'None' app suits


def make_labels(labels):
    """ Create regexp labels for `format_string` function """
    res = {'': '(?:.*?)'}

    for label_name, regexp in labels.iteritems():
        regexp = regexp.replace('{}', '.*?').format(**res)
        res[label_name] = '(?:' + regexp + ')'

    return res


def json_dumps(*args, **kwargs):
    return json.dumps(*args, default=repr, **kwargs)


@attr.s
class Token(object):
    name = attr.ib()
    value = attr.ib()


def _make_dict_from_experiments_list(experiments):
    if isinstance(experiments, list):
        return {key: Experiments.ENABLED_VALUE for key in experiments}
    return experiments


def split_tokens(string):
    sstart = 0
    send = len(string)

    for label in re.finditer(r'{\w*?}', string):
        tstart, tend = label.span()
        if sstart < tstart:
            yield Token('str', string[sstart:tstart])

        yield Token('label', string[tstart + 1: tend - 1])
        sstart = tend
    else:
        if sstart < send:
            yield Token('str', string[sstart:send])


def format_string(string, labels):
    """ Replace labels in string with values """
    res = []
    for tok in split_tokens(string):
        if tok.name == 'str':
            res.append(re.escape(tok.value))
        elif tok.name == 'label':
            res.append(unicode(labels[tok.value]))

    return ''.join(res)


def convert_for_assert(input_object):
    """Fast fix for problem: https://github.com/pytest-dev/pytest/issues/2322.
    Usage example:

    def test_u_appears_in_diff_when_the_test_fails_and_it_is_verbose():
    expected = {
        'foo': 'фуу',
        'bar': 'бар',
    }
    result = {
        'foo': u'фуу',
        'bar': 'not-бар',
    }

    assert convert(expected) == convert(result)

    :param input_object: same object for assert
    :return: prepared object to pytest's assert
    """

    if isinstance(input_object, dict):
        return dict(
            (convert_for_assert(key), convert_for_assert(value))
            for key, value in input_object.iteritems()
        )
    elif isinstance(input_object, list):
        return [convert_for_assert(element) for element in input_object]
    elif isinstance(input_object, str):
        return input_object.decode('utf-8')
    else:
        return input_object


@attr.s
class VinsResponseForTests(object):
    voice_text = attr.ib()
    text = attr.ib()
    meta = attr.ib()
    cards = attr.ib()
    suggests = attr.ib()
    directives = attr.ib()
    should_listen = attr.ib()
    special_buttons = attr.ib()


def convert_vins_response_for_tests(response):
    return VinsResponseForTests(
        response['voice_text'] or '',
        '\n'.join(c['text'] for c in response['cards']),
        response['meta'],
        response['cards'],
        response.get('suggests'),
        response.get('directives'),
        response.get('should_listen'),
        response.get('special_buttons'),
    )


def check_form(req_info, form, check_form_data):
    if check_form_data:
        name = check_form_data.get('name')
        if name and name != form.name:
            raise AssertionError(
                "Unexpected form name: {} expected, {} found for utterance '{}'".format(
                    name, form.name, req_info.utterance.text
                )
            )

        name_regexp = check_form_data.get('name_regexp')
        if name_regexp and not re.match(name_regexp, form.name, re.UNICODE):
            raise AssertionError(
                "Unexpected form name: {} regexp match expected, {} found for utterance '{}'".format(
                    name_regexp, form.name, req_info.utterance.text
                )
            )

        check_slots_map = check_form_data.get('slots')
        if check_slots_map:
            for slot in form.slots:
                slot_name = slot.name

                if slot_name in check_slots_map:
                    check_against = check_slots_map[slot_name]
                    if 'value' in check_against and slot.value != check_against['value']:
                        raise AssertionError(
                            "Unexpected value of slot '{}': {} expected, {} found for utterance '{}'.".format(
                                slot_name, check_against['value'], slot.value, req_info.utterance.text
                            )
                        )
                    if 'optional' in check_against and slot.optional != check_against['optional']:
                        raise AssertionError(
                            "Unexpected optionality of slot '{}': {} expected, {} found for utterance '{}'.".format(
                                slot_name, check_against['optional'], slot.optional, req_info.utterance.text
                            )
                        )


def spy_submit_form_and_check(source_method, check_form_data):
    def wrapper(
        self, req_info, form, action, balancer_type, bass_session_state=None, precomputed_data={}, session=None,
        is_banned=None
    ):
        check_form(req_info, form, check_form_data)
        return source_method(
            self, req_info, form, action, balancer_type, bass_session_state, session=session, is_banned=is_banned
        )

    return wrapper


def get_exact_matched_intents_mock(mock_intents):
    def _get_exact_matched_intents(self):
        return True, mock_intents

    return _get_exact_matched_intents


def get_classifiers_mocks(mock_intents):
    exact_matched_intents_mock = get_exact_matched_intents_mock(mock_intents)
    return mock.patch.object(
        ExactMatchTokenTagger, '_get_exact_matched_intents', exact_matched_intents_mock
    ), mock.patch.object(
        ExactMatchClassifier, '_get_exact_matched_intents', exact_matched_intents_mock
    )


def get_pa_submit_form_mock(utterance_test_data):
    submit_form_wrapper = spy_submit_form_and_check(PersonalAssistantAPI.submit_form, utterance_test_data.vins_form)
    return mock.patch.object(PersonalAssistantAPI, 'submit_form', submit_form_wrapper)


def get_vins_response(vins_app, uuid, dialog_test_data, utterance_test_data, request_id=None):
    additional_options = dialog_test_data.additional_options
    if additional_options and 'oauth_token' in additional_options:
        # change oauth_token value from setting name to setting value
        additional_options = deepcopy(additional_options)
        try:
            additional_options['oauth_token'] = get_setting(additional_options['oauth_token'])
        except Exception:
            logger.warning(
                'can not get additional_options/oauth_token={} value from settings'
                ' (skip using oauth)'.format(additional_options['oauth_token'])
            )
            additional_options.pop('oauth_token', None)

    if additional_options and 'test_user_oauth_token' in additional_options:
        additional_options = deepcopy(additional_options)
        additional_options['oauth_token'] = additional_options['test_user_oauth_token']
        additional_options.pop('test_user_oauth_token', None)

    if dialog_test_data.device_state and 'filtration_level' in dialog_test_data.device_state:
        additional_options = additional_options or {}
        if 'bass_options' not in additional_options:
            additional_options['bass_options'] = {}
        additional_options['bass_options']['filtration_level'] = dialog_test_data.device_state['filtration_level']

    tagger_mock, classifier_mock = get_classifiers_mocks(dialog_test_data.mock_intents)
    with get_pa_submit_form_mock(utterance_test_data), tagger_mock, classifier_mock:
        response = vins_app.handle_event(
            uuid,
            utterance_test_data.event,
            text_only=False,
            experiments=configure_experiment_flags(get_experiments(), utterance_test_data.experiments),
            location=dialog_test_data.geo_info,
            app_info=dialog_test_data.app_info,
            device_state=dialog_test_data.device_state,
            additional_options=additional_options,
            reset_session=dialog_test_data.dialog[0] is utterance_test_data,
            dialog_id=utterance_test_data.dialog_id,
            request_id=request_id,
        )

    return convert_vins_response_for_tests(response)


@attr.s
class TestStatus(object):
    NORMAL = 'normal'
    XFAIL = 'xfail'
    FLAKY = 'flaky'
    SKIP = 'skip'

    status = attr.ib(default=NORMAL)
    reason = attr.ib(default=None)


@attr.s
class SingleUtteranceTestData(object):
    request = attr.ib(default=attr.Factory(dict))
    text_regexp = attr.ib(default='')
    voice_regexp = attr.ib(default='')
    meta = attr.ib(default=attr.Factory(list))
    exact_meta_match = attr.ib(default=False)
    should_listen = attr.ib(default=None)
    suggests = attr.ib(default=attr.Factory(list))
    cards = attr.ib(default=attr.Factory(list))
    exact_suggests_match = attr.ib(default=False)
    button_actions = attr.ib(default=attr.Factory(list))
    directives = attr.ib(default=attr.Factory(list))
    exact_directives_match = attr.ib(default=False)
    bass_answer = attr.ib(default=attr.Factory(list))
    vins_form = attr.ib(default=attr.Factory(dict))
    experiments = attr.ib(default=attr.Factory(dict))
    special_buttons = attr.ib(default=attr.Factory(dict))
    lang = attr.ib(default=None)

    @property
    def event(self):
        if isinstance(self.request, basestring):
            return TextInputEvent(smart_unicode(self.request))
        elif isinstance(self.request, dict):
            req = dict(self.request)
            req.pop('dialog_id', None)
            return RequestEvent.from_dict(req)
        else:
            raise ValueError('Unexpected request type %s, should be string or dict' % type(self.request).__name__)

    @property
    def dialog_id(self):
        if isinstance(self.request, dict):
            return self.request.get('dialog_id', None)
        else:
            return None


@attr.s
class DialogSessionTestData(object):
    name = attr.ib()
    dialog = attr.ib()
    freeze_time = attr.ib(default=None)
    status = attr.ib(default=TestStatus.NORMAL)
    status_reason = attr.ib(default=None)
    geo_info = attr.ib(default=None)
    app_info = attr.ib(default=None)
    user_tags = attr.ib(default=None)
    device_state = attr.ib(default=attr.Factory(dict))
    additional_options = attr.ib(default=attr.Factory(dict))
    mock_intents = attr.ib(default=attr.Factory(list))
    app_preset = attr.ib(default=None)

    def get_app_info(self):
        return 'app_preset=' + self.app_preset

    def get_full_test_name(self):
        return self.name + '-' + self.get_app_info()


def turn_into_stub_filename(dialog_test_data_name):
    result = dialog_test_data_name
    for c, r in PYTEST_TO_STUB_REPLACEMENTS:
        result = result.replace(c, r)
    return result


def check_response(test_data, vins_response):
    unexpected = 'Unexpected {} response "{}" for request "{}" instead of "{}"'
    if test_data.text_regexp and not re.match(test_data.text_regexp,
                                              vins_response.text,
                                              re.UNICODE | re.DOTALL):
        return False, unexpected.format('text', vins_response.text, test_data.request, test_data.text_regexp)

    if test_data.voice_regexp and not re.match(test_data.voice_regexp,
                                               vins_response.voice_text,
                                               re.UNICODE | re.DOTALL):
        return False, unexpected.format('voice', vins_response.voice_text, test_data.request, test_data.voice_regexp)

    if test_data.should_listen is not None and test_data.should_listen != vins_response.should_listen:
        return False, 'should_listen is %s for request "%s", while the opposite is expected' % (vins_response.should_listen, test_data.request)  # noqa

    if test_data.exact_suggests_match:
        return exact_check_collections(vins_response.suggests, test_data.suggests, match_suggest, 'suggest')
    else:
        for suggest in test_data.suggests or []:
            if not find_suggest(vins_response.suggests, suggest):
                return False, somedict_not_found_msg(vins_response.suggests, suggest, 'suggest')

    for action in test_data.button_actions or []:
        if not find_button_action(vins_response.cards, action):
            return False, somedict_not_found_msg(vins_response.cards, action, 'action')

    if test_data.exact_directives_match:
        return exact_check_collections(vins_response.directives, test_data.directives, match_directive, 'directive')
    else:
        for directive in test_data.directives or []:
            if not find_directive(vins_response.directives, directive):
                return False, somedict_not_found_msg(vins_response.directives, directive, 'directive')

    if test_data.exact_meta_match:
        return exact_check_collections(vins_response.meta, test_data.meta, match_meta, 'meta')
    else:
        for meta in test_data.meta or []:
            if not find_meta(vins_response.meta, meta):
                return False, somedict_not_found_msg(vins_response.meta, meta, 'meta')

    for card_type in test_data.cards:
        for card in vins_response.cards:
            if card_type == card['type']:
                break
        else:
            return False, somedict_not_found_msg(vins_response.cards, card_type, 'card')

    if test_data.special_buttons is not None:
        assert len(vins_response.special_buttons) == len(test_data.special_buttons)

        for button_test, button_response in zip(test_data.special_buttons, vins_response.special_buttons):
            def texts_match():
                return button_test['text'] == button_response['text']

            def sub_lists_match():
                sub_list_response = [x['payload'] for x in button_response['directives']
                                     if x['name'] == 'special_button_list']
                if 'sub_list' in button_test:
                    if not sub_list_response:
                        return False
                    response_subbuttons = [x['text'] for x in sub_list_response[0]['special_buttons']]
                    return all(test_sb == response_sb for test_sb, response_sb in
                               zip(button_test['sub_list'], response_subbuttons))
                return len(sub_list_response) == 0

            if not texts_match() or not sub_lists_match():
                return False, somedict_not_found_msg(vins_response.special_buttons, button_test,
                                                     'special_button')

    return True, ''


def smart_update(target, source):
    for slot, value in source.items():
        if not value:
            if slot in target:
                target.pop(slot)
            continue

        if slot in target:
            target_value = target[slot]

            if isinstance(value, dict) and isinstance(target_value, dict):
                smart_update(target_value, value)
            else:
                target[slot] = value
        else:
            target[slot] = value


def get_joint_bass_response(bass_response_list):
    if not bass_response_list:
        return None

    joint_bass_response = {'form': {}, 'blocks': []}
    for bass_response in bass_response_list:
        bass_response = deepcopy(bass_response)
        smart_update(joint_bass_response['form'], bass_response.get('form', {}))
        joint_bass_response['blocks'] += bass_response.get('blocks', [])
        joint_bass_response['form_name'] = bass_response.get('form_name')
    return joint_bass_response


def build_test_data(request, response, labels, group_bass_response_list, experiments, special_buttons, lang, test_name=''):
    bass_answers = copy(group_bass_response_list)
    experiments = copy(experiments)
    special_buttons = copy(special_buttons)

    vins_text_response = vins_voice_response = None
    if isinstance(response, basestring):
        vins_text_response = '^' + format_string(response, labels) + '$'
        response = {}
    else:
        if 'text' in response:
            vins_text_response = '^' + format_string(response['text'], labels) + '$'
        if 'voice' in response:
            vins_voice_response = '^' + format_string(response['voice'], labels) + '$'

    should_listen = response.get('should_listen')

    if 'bass' in response:
        bass_answers.append(response['bass'])

    if 'experiments' in response:
        experiments.update(_make_dict_from_experiments_list(response['experiments']))

    if 'special_buttons' in response:
        if special_buttons is not None:
            special_buttons.extend(response['special_buttons'])
        else:
            special_buttons = response['special_buttons']

    suggests = response.get('suggests')
    exact_suggests_match = False
    exact_directives_match = False
    exact_meta_match = False

    if suggests and isinstance(suggests, dict):
        if suggests.get('exact_match'):
            exact_suggests_match = True
        suggests = suggests['data']

    if suggests:
        for suggest in suggests:
            for key in ['caption', 'utterance', 'user_utterance']:
                if key in suggest:
                    suggest[key] = '^' + format_string(suggest[key], labels) + '$'

    directives = response.get('directives')
    if directives and isinstance(directives, dict):
        if directives.get('exact_match'):
            exact_directives_match = True
        directives = directives['data']

    meta = response.get('meta')
    if meta and isinstance(meta, dict):
        if meta.get('exact_match'):
            exact_meta_match = True
        meta = meta['data']

    cards = response.get('cards', [])
    button_actions = response.get('button_actions')
    vins_form = response.get('vins_form')
    special_buttons = response.get('special_buttons', [])

    if vins_form:
        redundant_keys = set(vins_form.keys()).difference({'slots', 'name', 'name_regexp'})
        if redundant_keys:
            raise ValueError(
                'Redundant keys %s detected within vins_form check for utterance "%s"' %
                (redundant_keys, request)
            )

    if not any([vins_text_response, vins_voice_response, suggests, cards, button_actions,
                (directives or exact_directives_match), vins_form, special_buttons]):
        raise ValueError('Empty test data for utterance "%s" in test "%s"' % (request, test_name))

    return SingleUtteranceTestData(
        request=request,
        text_regexp=vins_text_response,
        voice_regexp=vins_voice_response,
        meta=meta,
        exact_meta_match=exact_meta_match,
        should_listen=should_listen,
        suggests=suggests,
        cards=cards,
        exact_suggests_match=exact_suggests_match,
        button_actions=button_actions,
        directives=directives,
        exact_directives_match=exact_directives_match,
        bass_answer=get_joint_bass_response(bass_answers),
        vins_form=vins_form,
        experiments=experiments,
        special_buttons=special_buttons,
        lang=lang
    )


def get_dialog_items(dialog):
    if isinstance(dialog, list):
        return ((item['request'], item['response']) for item in dialog)
    else:
        return dialog.iteritems()


def make_dialog(dialog, labels, group_bass_response_list, experiments, special_buttons, lang, test_name=''):
    return [
        build_test_data(
            request, response, labels, group_bass_response_list, experiments, special_buttons, lang, test_name=test_name
        )
        for request, response in get_dialog_items(dialog)
    ]


def suitable_app_infos(app_info, app_infos):
    split_mode_enabled = SPLIT_MODE_APP_PRESET_NAME in app_infos
    implicit_tests = IMPLICIT_APP_PRESET_NAME in app_infos

    if app_info:
        ignore = app_info.get('ignore')
        if ignore:
            res_app_infos = [(app_info_id, app_infos[app_info_id]) for app_info_id in app_infos if
                             app_info_id not in ignore]
        else:
            res_app_infos = [('custom', app_info)]
            if split_mode_enabled:
                return res_app_infos if implicit_tests else None
    else:
        res_app_infos = app_infos.items()

    if res_app_infos:
        res_app_infos = [item for item in res_app_infos if not item[0].startswith('@')]

    if not res_app_infos:
        res_app_infos = [('None', None)]
        if split_mode_enabled:
            return res_app_infos if implicit_tests else None

    if split_mode_enabled and implicit_tests:
        return None

    return res_app_infos


def make_test_data(
    name, dialog, labels, group_bass_response_list, mock_intents, status,
    app_infos, is_fast_mode, **kwargs
):
    """ Make tests from yaml
    test could be marked as 'skip' or 'xfail'

    test_smth:
      flags:
        xfail:
          value: true
          reason: just because
    """
    app_infos = app_infos or {}
    app_info = kwargs.get('app_info')

    cur_app_infos = suitable_app_infos(app_info, app_infos)
    if cur_app_infos is None:
        return

    if is_fast_mode and cur_app_infos:
        random_app_index = hash(name) % len(cur_app_infos)
        cur_app_infos = [cur_app_infos[random_app_index]]

    dialog_data = make_dialog(dialog, labels, group_bass_response_list, kwargs['experiments'],
                              kwargs['special_buttons'], kwargs['lang'], test_name=name)
    for app_preset, app_info in cur_app_infos:
        if app_info:
            app_info = AppInfo(
                app_id=app_info['app_id'],
                app_version=app_info['app_version'],
                os_version=app_info['os_version'],
                platform=app_info['platform'],
                device_manufacturer=app_info.get('device_manufacturer', '')
            )

        test_user_info = kwargs.get('test_user_info')
        if test_user_info:
            user_tags = test_user_info['tags']
        else:
            user_tags = None

        dialog_session_test_data = DialogSessionTestData(
            name=name,
            freeze_time=kwargs.get('freeze_time'),
            geo_info=kwargs.get('geo'),
            app_info=app_info,
            user_tags=user_tags,
            device_state=kwargs.get('device_state', {}),
            status=status.status,
            status_reason=status.reason,
            dialog=dialog_data,
            additional_options=kwargs.get('additional_options', {}),
            mock_intents=mock_intents,
            app_preset=app_preset
        )
        yield dialog_session_test_data


FRAMEWORK_PROXY_KEYWORDS = ('freeze_time', 'geo', 'app_info', 'test_user_info', 'device_state', 'additional_options')
FRAMEWORK_KEYWORDS = FRAMEWORK_PROXY_KEYWORDS + ('bass', 'experiments')


def process_dialog_group(group_name, labels, body, bass_response_list, mock_intents,
                         is_root=False, app_infos=None, is_fast_mode=False, **kwargs):
    bass_response = body.get('bass')
    if bass_response:
        bass_response_list += [bass_response]

    for proxy_keyword in FRAMEWORK_PROXY_KEYWORDS:
        if proxy_keyword in body:
            kwargs[proxy_keyword] = body[proxy_keyword]

    if 'experiments' in body:
        experiments = copy(kwargs['experiments'])
        experiments.update(_make_dict_from_experiments_list(body['experiments']))
        kwargs['experiments'] = experiments

    if 'special_buttons' in body:
        special_buttons = copy(kwargs['special_buttons'])
        special_buttons.extend((body['special_buttons']))
        kwargs['special_buttons'] = special_buttons

    flags = body.pop('flags', {})

    status = TestStatus(TestStatus.NORMAL, None)
    for flag_name in (TestStatus.XFAIL, TestStatus.SKIP, TestStatus.FLAKY):
        flag = flags.get(flag_name, {})
        if flag and flag.get('value', True):
            status = TestStatus(flag_name, flag.get('reason'))

    if 'dialog' in body:
        for test_data in make_test_data(group_name, body['dialog'], labels, bass_response_list,
                                        mock_intents, status, app_infos, is_fast_mode, **kwargs):
            yield test_data
    elif any(keyword in body for keyword in FRAMEWORK_KEYWORDS) or is_root:
        for inner_group_name, inner_body in body.iteritems():
            if inner_group_name in FRAMEWORK_KEYWORDS:
                continue

            new_group_name = '%s::%s' % (group_name, inner_group_name)
            for dialog_test_data in process_dialog_group(
                new_group_name,
                labels,
                inner_body,
                copy(bass_response_list),
                mock_intents,
                False,
                app_infos,
                is_fast_mode,
                **kwargs
            ):
                yield dialog_test_data
    else:
        for test_data in make_test_data(group_name, body, labels, bass_response_list, mock_intents,
                                        status, app_infos, is_fast_mode, **kwargs):
            yield test_data


def load_yaml(file_or_string, placeholders_data=None):
    """
    Extend yaml syntax to support `!Any[ foo]` syntax
    !Any could be used insted of any object inside tests
    and equals to anything
    Example:
      directives:
      - type: client_action
         payload: !Any

    optional `name` parameter add checks that objects with same name
    are truly equals

    `!Placeholder ['label', 'default']` or `!Placeholder label`
    Special object that could be replaced with value that comes from `placeholders_data` mapping.
    """

    Loader = yaml.Loader
    any_labels = {}

    class Any(object):
        def __init__(self, loader, node):
            self._label = node.value or None

        def __eq__(self, value):
            if self._label:
                if self._label in any_labels:
                    return any_labels[self._label] == value
                else:
                    any_labels[self._label] = value
                    return True

            else:
                return True

        def __repr__(self):
            if self._label in any_labels:
                res = 'Any(%s, %s)' % (self._label, any_labels[self._label])
            else:
                res = 'Any(%s)' % self._label

            return res.encode('utf-8')

    if placeholders_data is None:
        placeholders_data = {}

    def placeholder(loader, node):
        if isinstance(node.value, basestring):
            label, default = node.value, None
        elif isinstance(node.value, list):
            label, default = map(loader.construct_object, node.value)
        else:
            raise TypeError(
                'Placeholder constructor expects either string or list, got %s' % (
                    type(node.value)
                )
            )

        if label in placeholders_data:
            return placeholders_data[label]
        else:
            if default is None:
                raise yaml.YAMLError(
                    'Missed placeholder "%s", line %s' % (
                        label, loader.line
                    )
                )
            return default

    def order_dict_constructor(loader, node):
        loader.flatten_mapping(node)
        pairs = loader.construct_pairs(node)
        keys = set()
        for key, value in pairs:
            if key in keys:
                raise ConstructorError(u'The key {} in the node {} is duplicated'.format(key, node.start_mark))
            keys.add(key)
        return OrderedDict(pairs)

    Loader.add_constructor('!Any', Any)
    Loader.add_constructor('!Placeholder', placeholder)
    Loader.add_constructor(yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, order_dict_constructor)

    return yaml.load(file_or_string, Loader=Loader)


def parse_placeholders(params_string):
    res = {}
    params_string = params_string.strip()
    for kv in params_string.split(';'):
        bits = kv.split(':', 1)
        if len(bits) != 2:
            continue
        else:
            k, v = bits
            res[k] = v
    return res


def process_file(yaml_file_or_string, name, placeholders=None, app_infos=None, is_fast_mode=False, experiments=None):
    data = load_yaml(yaml_file_or_string, placeholders)
    if not data:
        return
    lang = data.pop('lang', DEFAULT_LANG)
    labels = make_labels(data.pop('labels', {}))
    mock_intents = data.pop('mock_intents', [])
    if not isinstance(mock_intents, list):
        if isinstance(mock_intents, str):
            mock_intents = mock_intents.split(',')
            mock_intents = [intent_name.strip() for intent_name in mock_intents]
        else:
            raise ValueError("Cannot parse mock_intents: %s", mock_intents)

    for dialog_test_data in process_dialog_group(
        group_name=name, labels=labels, body=data, lang=lang,
        bass_response_list=[], experiments=experiments or {}, geo=YANDEX_OFFICE_LOCATION,
        mock_intents=mock_intents, is_root=True, special_buttons=None, app_infos=app_infos,
        is_fast_mode=is_fast_mode
    ):
        yield dialog_test_data


def list_data_files(test_data_path):
    res_prefix = os.path.join('personal_assistant', 'tests', test_data_path)
    tests = list_resource_files_with_prefix(res_prefix)
    for fname in tests:
        name, _ = os.path.splitext(fname)
        if all([part.startswith('test_') for part in fname.split('/')]):
            yield name.replace('/', '::'), os.path.join(res_prefix, fname)


def load_app_infos(test_data_path):
    if is_resource_exists(APP_PRESETS_RESOURCE_KEY):
        fname = APP_PRESETS_RESOURCE_KEY
    else:
        fname = os.path.join('personal_assistant', 'tests', test_data_path, 'app_presets.yaml')

    app_infos = load_data_from_file(fname)

    env_platform = get_setting('TEST_PLATFORM', '')
    if env_platform:
        if env_platform not in app_infos:
            raise AssertionError(
                "Unexpected app platform in VINS_TEST_PLATFORM: '{}'".format(env_platform)
            )
        app_infos = {env_platform: app_infos[env_platform]}

    return app_infos


def load_testcase(test_data_path, placeholders=None, is_fast_mode=False, experiments=None):
    app_infos = load_app_infos(test_data_path)
    for name, path in list_data_files(test_data_path):
        logger.debug('Loading test cases from {}'.format(path))
        for dialog_test_data in process_file(
                open_resource_file(path), name, placeholders, app_infos, is_fast_mode, experiments=experiments
        ):
            yield dialog_test_data


def exact_check_collections(real_collection, test_collection, matching_function, name):
    if len(real_collection) != len(test_collection):
        return False, "size of real %s list (%d) doesn't match size of test %s list (%d)" % (
            name, len(real_collection),
            name, len(test_collection)
        )

    test_iterator = iter(test_collection)

    for real_element in real_collection:
        test_element = test_iterator.next()

        if not matching_function(test_element, real_element):
            test_string = json_dumps(test_element, ensure_ascii=False, indent=2)
            real_string = json_dumps(real_element, ensure_ascii=False, indent=2)
            return False, "real %s %s doesn't match test %s %s" % (name, real_string, name, test_string)

    return True, ''


def match_directive(test_directive, real_directive):
    return test_directive == real_directive


def find_directive(directives_response, directive):
    for candidate_directive in directives_response:
        if match_directive(directive, candidate_directive):
            return True

    return False


def match_meta(test_meta, real_meta):
    return test_meta == real_meta


def find_meta(meta_response, meta):
    for candidate_meta in meta_response:
        if match_meta(meta, candidate_meta):
            return True

    return False


def find_button_action(cards_response, action):
    for card in cards_response:
        for button in card.get('buttons', []):
            if button['type'] == 'action' and button['title'] == action['title']:
                for directive in button.get('directives', []):
                    if (
                        directive['name'] == action['name'] and
                        directive.get('payload') == action.get('payload')
                    ):
                        return True
    return False


def match_suggest(suggest_from_test, real_suggest):
    if 'caption' in suggest_from_test:
        caption_regexp = suggest_from_test['caption']
        if not re.match(caption_regexp, real_suggest.get('title', ''), re.UNICODE):
            return False

        on_suggest_server_action_found = False

        for directive in real_suggest.get('directives', []):
            if 'server_action' == directive.get('type') and 'on_suggest' == directive.get('name'):
                if 'payload' in directive and re.match(
                    caption_regexp, directive['payload'].get('caption', ''), re.UNICODE
                ):
                    on_suggest_server_action_found = True
                    break

        if not on_suggest_server_action_found:
            return False

    if 'utterance' in suggest_from_test:
        utterance_regexp = suggest_from_test['utterance']
        on_suggest_server_action_found = False
        type_client_action_found = False

        for directive in real_suggest.get('directives', []):
            if 'client_action' == directive.get('type') and 'type' == directive.get('name'):
                if 'payload' in directive and re.match(
                    utterance_regexp, directive['payload'].get('text', ''), re.UNICODE
                ):
                    type_client_action_found = True
            if 'server_action' == directive.get('type'):
                if 'payload' in directive and re.match(
                    utterance_regexp, directive['payload'].get('utterance', ''), re.UNICODE
                ):
                    on_suggest_server_action_found = True

        if not on_suggest_server_action_found or not type_client_action_found:
            return False

    if 'user_utterance' in suggest_from_test:
        user_utterance_regexp = suggest_from_test['user_utterance']
        type_silent_client_action_found = False

        for directive in real_suggest.get('directives', []):
            if 'client_action' == directive.get('type') and 'type_silent' == directive.get('name'):
                if re.match(user_utterance_regexp, directive.get('payload', {}).get('text', ''), re.UNICODE):
                    type_silent_client_action_found = True

        if not type_silent_client_action_found:
            return False

    if 'directive' in suggest_from_test:
        suggest_directive = suggest_from_test['directive']
        payload = suggest_from_test['payload']
        for directive in real_suggest.get('directives', []):
            if directive['name'] == suggest_directive and directive['payload'] == payload:
                break
        else:
            return False

    return True


def find_suggest(suggests_response, suggest):
    for candidate_suggest in suggests_response:
        if match_suggest(suggest, candidate_suggest):
            return True
    return False


def somedict_not_found_msg(somedicts_response, somedict, name):
    somedict_string = json_dumps(somedict, ensure_ascii=False, indent=2)
    somedicts_response_string = json_dumps(somedicts_response, ensure_ascii=False, indent=2)
    return 'Unable to find %s %s inside %ss response %s' % (name, somedict_string, name, somedicts_response_string)


def init_mock(mock, mock_entity_search=True):
    import requests_mock

    fixlist = [{
        'intent': 'personal_assistant.general_conversation.general_conversation',
        'text': 'погода на марсе'
    }]

    mock.register_uri(
        requests_mock.ANY,
        requests_mock.ANY,
        text='Real HTTP requests are prohibited',
        status_code=500,
    )

    allowed_http_get_requests = [
        re.compile(r'/wizard(\?.*)?$'),                            # wizard
        re.compile(r'/misspell.json/check(\?.*)?$'),               # misspell
        re.compile(r'/suggest/query-wizard-features(\?.*)?$'),     # serp features
        re.compile(r'https?://proxy\.sandbox\.yandex-team\.ru/'),  # sandbox resources
        re.compile(r'https?://sandbox\.yandex-team\.ru/api/v1.0/resource'),  # sandbox resources api
        re.compile(r'https?://.*\.s3\.mds\.yandex\.net/\d+'),      # s3 resources
    ]
    if not mock_entity_search:
        allowed_http_get_requests.append(re.compile(re.escape(EntitySearchHTTPAPI.ENTITYSEARCH_URL)))

    allowed_http_post_requests = [
        re.compile(r'/test_user(\?.*)?$')  # bass test users
    ]

    for allowed_request in allowed_http_get_requests:
        mock.get(allowed_request, real_http=True)

    for allowed_request in allowed_http_post_requests:
        mock.post(allowed_request, real_http=True)

    # Mock the fixlist
    fixlist_text = '\n'.join(json.dumps(item, ensure_ascii=False) for item in fixlist)
    mock.get('http://hahn.yt.yandex.net/api/v3/read_table?path=//home/voice/vins/fixlist', text=fixlist_text)


def entitysearch_callback(request, context, base):
    if not base:
        return {'cards': []}
    entity_ids = []
    for entity in re.finditer(r'(?P<entity_id>ruw\d+)', request.path_url,
                              flags=re.UNICODE):
        entity_ids.append(entity.group('entity_id'))
    return {'cards': [base[entity_id] for entity_id in entity_ids if entity_id in base]}


def _pa_mock(url_suffixes, data, status_code, balancer_type, entity_base):
    import requests_mock

    mock = requests_mock.Mocker()
    mock_entity_search = not get_bool_setting('TURN_OFF_ENTITY_SEARCH_MOCK')
    init_mock(mock, mock_entity_search)

    pa = PersonalAssistantAPI
    if balancer_type == ANY_BASS_QUERY:
        balancer_types = ALL_SUPPORTED_BALANCER_TYPES
    else:
        assert balancer_type in ALL_SUPPORTED_BALANCER_TYPES
        balancer_types = [balancer_type]

    for url_suffix in url_suffixes:
        for b_type in balancer_types:
            url = pa._get_url_prefix(req_info=None, balancer_type=b_type) + pa._urls[url_suffix]
            mock.post(url, json=data, status_code=status_code)

    if mock_entity_search:
        mock.get(
            EntitySearchHTTPAPI.ENTITYSEARCH_URL,
            json=lambda request, context: entitysearch_callback(request, context, entity_base)
        )

    return mock


def form_handling_mock(data, url_suffixes=('vins',), status_code=200, balancer_type=FAST_BASS_QUERY, entity_base=None):
    return _pa_mock(url_suffixes=url_suffixes, data=data, status_code=status_code,
                    balancer_type=balancer_type, entity_base=entity_base)


def form_handling_fail_mock(balancer_type=FAST_BASS_QUERY, entity_base=None):
    return _pa_mock(['vins'], None, 500, balancer_type, entity_base)


def universal_form_mock(set_slots_map, check_slots_map=None, blocks=(), form_name=None, entity_base=None):
    def response_callback(request, context):
        request = json.loads(request.body)
        form = request['form']

        if form_name is not None:
            form['name'] = form_name
            slot_names = {slot['name'] for slot in form['slots']}

            for name in set_slots_map:
                if name not in slot_names:
                    form['slots'].append({
                        'name': name,
                        'value': None,
                        'type': 'any',
                    })

        for i, slot in enumerate(form['slots']):
            slot_name = slot['name']

            if slot_name in set_slots_map:
                fill_with = set_slots_map[slot_name]
                if 'value' in fill_with:
                    form['slots'][i]['value'] = fill_with['value']
                if 'optional' in fill_with:
                    form['slots'][i]['optional'] = fill_with['optional']

        request.pop('meta')
        request['blocks'] = blocks
        return request

    return form_handling_mock(response_callback, balancer_type=ANY_BASS_QUERY, entity_base=entity_base)


def user_context_mock():
    storage = {}

    def response_callback(request, context):
        data = json.loads(request.body)
        if 'set' in data:
            key = data['set'][0]['key']
            value = data['set'][0]['value']
            storage[(data['uuid'], key)] = value
            return {}
        elif 'get' in data:
            key = data['get'][0]
            return {
                'context': [{
                    'key': key,
                    'value': storage.get((data['uuid'], key))
                }]
            }
        else:
            return data

    return _pa_mock(['user-context'], response_callback, 200, balancer_type=FAST_BASS_QUERY, entity_base=None)


def check_meta_mock(expected_meta=None):
    if expected_meta is None:
        expected_meta = {}

    def response_callback(request, context):
        data = json.loads(request.body)
        meta = data['meta']

        for key, value in expected_meta.iteritems():
            if value != meta[key]:
                raise AssertionError(
                    "Unexpected value of meta field '{}': {} expected, {} found.".format(key, value, meta[key])
                )

        return data

    return form_handling_mock(response_callback)


def user_context_fail_mock():
    return _pa_mock(['user-context'], None, 500, balancer_type=FAST_BASS_QUERY, entity_base=None)


def search_api_mock(query, factoid=None, factoid_tts=None, calculator=None, blocks=()):
    _blocks = [
        {
            'type': 'suggest',
            'suggest_type': 'search__serp'
        }
    ]
    _blocks.extend(blocks)
    blocks = _blocks

    search_url = 'https://yandex.ru/search/?text=%s' % query

    def response_callback(request, context):
        data = json.loads(request.body)
        form_slots = {slot['name']: slot for slot in data['form']['slots']}

        query_match = query == form_slots['query']['value']

        assert query_match, 'Unexpected POI query!'

        form_slots['search_results']['value'] = {
            'factoid': {
                'text': factoid,
                'tts': factoid_tts
            },
            'serp': {
                'url': search_url
            }
        }

        if calculator:
            form_slots['search_results']['value']['calculator'] = calculator

        data.pop('meta')
        data['blocks'] = blocks

        return data

    return form_handling_mock(response_callback, balancer_type=SLOW_BASS_QUERY)


def _fill_form_from_slots_map(form, slots_map):
    slot_names = set()
    for i, slot in enumerate(form['slots']):
        slot_name = slot['name']
        if slot_name in slots_map:
            form['slots'][i]['value'] = slots_map[slot_name]
        slot_names.add(slot_name)

    for name, value in slots_map.items():
        if name not in slot_names:
            form['slots'].append({
                'name': name,
                'value': value,
                'type': 'any',
            })


def _weather_api_mock(slots_map, blocks=()):
    location = slots_map.get('forecast_location', {})

    location_default = {
        'city': 'Москва',
        'city_cases': {
            'preposition': 'в',
            'prepositional': 'Москве'
        },
        'country': 'Россия',
        'street': None,
        'house': None,
        'address_line': 'Россия, Москва',
        'geoid': 213,
    }

    location_default.update(location)
    slots_map['forecast_location'] = location_default

    def response_callback(request, context):
        request = json.loads(request.body)
        form = request['form']

        _fill_form_from_slots_map(form, slots_map)

        request.pop('meta')
        request['blocks'] = blocks
        return request

    return form_handling_mock(response_callback)


def suggests_mock(suggest_names=(), balancer_type=FAST_BASS_QUERY, form_name=None, blocks=None):
    blocks = blocks or []
    blocks.extend([{'type': 'suggest', 'suggest_type': type_} for type_ in suggest_names])

    def response_callback(request, context):
        request = json.loads(request.body)

        form = request['form']
        if form_name is not None:
            form['name'] = form_name

        request.pop('meta')
        request['blocks'] = blocks
        return request

    return form_handling_mock(response_callback, balancer_type=balancer_type)


def bass_action_mock(action, balancer_type=FAST_BASS_QUERY):
    def response_callback(request, context):
        request = json.loads(request.body)

        assert request['action'] == action

        request.pop('meta')
        request['blocks'] = []
        return request

    return form_handling_mock(response_callback, balancer_type=balancer_type)


def cards_mock(cards=None, slots_map=None, form_name=None):
    blocks = cards or []

    def response_callback(request, context):
        request = json.loads(request.body)

        request.pop('meta')
        request['blocks'] = blocks

        form = request['form']
        if form_name is not None:
            form['name'] = form_name

        if slots_map:
            _fill_form_from_slots_map(form, slots_map)

        return request

    return form_handling_mock(response_callback)


def current_weather_api_mock(temperature, condition,
                             location=None, blocks=()):
    slots_name_value = {
        'weather_forecast': {
            'condition': condition,
            'temperature': temperature,
            'type': 'weather_current',
        },
    }

    if location:
        slots_name_value['forecast_location'] = location

    return _weather_api_mock(slots_name_value, blocks)


def weather_for_day_api_mock(dt, temperature, condition,
                             location=None, tz='Europe/Moscow', blocks=()):
    slots_name_value = {
        'weather_forecast': {
            'condition': condition,
            'temperature': temperature,
            'date': dt.strftime('%Y-%m-%d'),
            'tz': tz,
            'type': 'weather_for_date',
        },
    }

    if location:
        slots_name_value['forecast_location'] = location

    return _weather_api_mock(slots_name_value, blocks)


def weather_for_range_api_mock(dates, temperatures, conditions,
                               location=None, blocks=()):
    res = []
    for dt, temperature, condition in zip(dates, temperatures, conditions):
        res.append({
            'condition': condition,
            'temperature': temperature,
            'date': dt.strftime('%Y-%m-%d'),
            'tz': str(dt.tzinfo)
        })

    slots_name_value = {
        'weather_forecast': {
            'days': res,
            'type': 'weather_for_range',
        },
    }

    if location:
        slots_name_value['forecast_location'] = location

    return _weather_api_mock(slots_name_value, blocks)


def poi_opening_hours(opening_time, closing_time, is_open=True):
    return {
        'from': opening_time,
        'to': closing_time,
        'current_status': 'open' if is_open else 'closed'
    }


def poi_opening_hours_list(opening_hours, is_open=True):
    return {
        'working': [
            {'from': h[0], 'to': h[1]}
            for h in opening_hours
        ],
        'current_status': 'open' if is_open else 'closed'
    }


def poi_opening_hours_round_the_clock():
    return {
        '24_hours': True
    }


def geo_search_api_mock(country='Россия', city='Москва', street=None,
                        house=None, new_result_index=1, in_user_city=True,
                        geo_uri=None, what=None, where=None, form=None):
    blocks = [
        {
            'type': 'suggest',
            'suggest_type': 'find_poi__next'
        },
        {
            'type': 'suggest',
            'suggest_type': 'find_poi__show_on_map'
        }
    ]

    found_poi = {
        'country': country,
        'city': city,
        'street': street,
        'geo_uri': geo_uri,
        'house': house,
        'in_user_city': in_user_city,
    }

    return _poi_search_api_mock(
        found_poi=found_poi, blocks=blocks, what=what, where=where, form=form,
        new_result_index=new_result_index, make_where_required=False, make_what_required=False)


def patch_contacts_slot(slots, contact_slot):
    added = False
    for i, slot in enumerate(slots):
        if slot.get('value_type') == 'contact_search_results':
            slots[i] = contact_slot
            added = True
    if not added:
        slots.append(contact_slot)


def phone_call_mock(contacts=None):
    _blocks = [
        {
            'type': 'sensitive',
            'data': {
                'slots': ['recipient', 'contact_search_results']
            }
        }
    ]

    def response_callback(request, context):
        data = json.loads(request.body)
        data.pop('meta')
        data['blocks'] = _blocks
        slots = data['form']['slots']

        contact_slot = {
            'active': False,
            'allow_multiple': False,
            'concatenation': 'forbid',
            'disabled': False,
            'expected_values': None,
            'import_entity_pronouns': [],
            'import_entity_tags': [],
            'import_entity_types': [],
            'import_tags': [],
            'matching_type': 'exact',
            'normalize_to': None,
            'optional': True,
            'share_tags': [
                'call__contact_search_results'
            ],
            'slot': 'contact_search_results',
            'source_text': None,
            'types': [
                'contact_search_results'
            ],
            'value': contacts,
            'value_type': 'contact_search_results'
        }
        patch_contacts_slot(slots, contact_slot)

        return data

    return form_handling_mock(response_callback)


def poi_search_api_mock(
    name=None, street=None, house=None, country='Россия', city='Москва', object_id=None,
    hours=None, in_user_city=True, what=None, where=None, form=None, new_result_index=1,
    make_where_required=False, make_what_required=False
):
    blocks = [
        {
            'type': 'suggest',
            'suggest_type': 'find_poi__next'
        }
    ]
    found_poi = {
        'name': name,
        'company_name': name,
        'object_uri': None if object_id is None else 'https://yandex.ru/maps/org/%s' % object_id,
        'object_id': object_id,
        'hours': hours,
        'geo': {
            'country': country,
            'city': city,
            'street': street,
            'house': house,
            'in_user_city': in_user_city,
        }
    }

    if object_id is not None:
        blocks.append({
            'type': 'suggest',
            'suggest_type': 'find_poi__details'
        })

    return _poi_search_api_mock(
        found_poi=found_poi, blocks=blocks, what=what, where=where, form=form,
        new_result_index=new_result_index, make_where_required=make_where_required,
        make_what_required=make_what_required)


def poi_search_api_mock_nothing_found(what=None, where=None, form='find_poi', new_result_index=1):
    return _poi_search_api_mock(found_poi=None, blocks=[], what=what, where=where, form=form,
                                new_result_index=new_result_index, make_where_required=False, make_what_required=False)


def _poi_search_api_mock(found_poi, blocks, what, where, form, new_result_index,
                         make_where_required, make_what_required):
    def response_callback(request, context):
        data = json.loads(request.body)
        form_slots = {slot['name']: slot for slot in data['form']['slots']}

        form_match = form is None or ('personal_assistant.scenarios.%s' % form) == data['form']['name']
        what_match = what == form_slots['what']['value']
        where_match = where == form_slots['where']['value']

        assert form_match and what_match and where_match, 'Unexpected POI query!'

        form_slots['last_found_poi']['value'] = found_poi

        if new_result_index is not None:
            form_slots['result_index']['value'] = new_result_index

        if make_where_required:
            form_slots['where']['optional'] = False
        if make_what_required:
            form_slots['what']['optional'] = False

        data.pop('meta')
        data['blocks'] = blocks

        return data

    return form_handling_mock(response_callback, balancer_type=SLOW_BASS_QUERY)


def show_route_api_mock(
    what_from=None, where_from=None, what_to=None, where_to=None,
    street_from=None, house_from=None, country_from='Россия', city_from='Москва', in_user_city_from=True,
    street_to=None, house_to=None, country_to='Россия', city_to='Москва', in_user_city_to=True,
    time_by_car=10.0 * 60.0, time_by_public_transport=20.0 * 60.0, transfers=0, walking_dist=1000.0,
    time_on_foot=15.0 * 60.0, ask_where_to=False,
):
    def make_address_line(city, street, house):
        result = city
        if street:
            result += ', %s' % street
            if house:
                result += ', %s' % house
        return result

    resolved_location_form = {
        'country': country_from,
        'city': city_from,
        'street': street_from,
        'house': house_from,
        'in_user_city': in_user_city_from,
        'address_line': make_address_line(city_from, street_from, house_from)
    }
    resolved_location_to = {
        'country': country_to,
        'city': city_to,
        'street': street_to,
        'house': house_to,
        'in_user_city': in_user_city_to,
        'address_line': make_address_line(city_from, street_from, house_from)
    }

    route_info = None if not time_by_car and not time_by_public_transport and not time_on_foot else {}
    route_maps_uri = 'http://maps.ru/route' if route_info else 'http://maps.ru/fallback'

    if time_by_car:
        route_info['car'] = {
            'jams_time': {
                'value': time_by_car
            },
            'maps_uri': 'http://maps.ru/car'
        }
    if time_by_public_transport:
        route_info['public_transport'] = {
            'time': {
                'value': time_by_public_transport
            },
            'transfers': transfers,
            'walking_dist': {
                'value': walking_dist
            },
            'maps_uri': 'http://maps.ru/public_transport'
        }
    if time_on_foot:
        route_info['pedestrian'] = {
            'time': {
                'value': time_on_foot
            },
            "walking_dist": {
                "text": "6.66 км",
                "value": 666
            },
            'maps_uri': 'http://maps.ru/pedestrian'
        }

    blocks = []
    if route_maps_uri:
        blocks.append({
            'type': 'suggest',
            'suggest_type': 'show_route__show_on_map'
        })
    if time_by_car:
        blocks.append({
            'type': 'suggest',
            'suggest_type': 'show_route__go_by_car'
        })
    if time_by_public_transport:
        blocks.append({
            'type': 'suggest',
            'suggest_type': 'show_route__go_by_public_transport'
        })
    if time_on_foot:
        blocks.append({
            'type': 'suggest',
            'suggest_type': 'show_route__go_by_foot'
        })

    return _show_route_api_mock(
        what_from=what_from, what_to=what_to, where_from=where_from, where_to=where_to,
        location_from=resolved_location_form, location_to=resolved_location_to,
        route_info=route_info, route_maps_uri=route_maps_uri, blocks=blocks, ask_where_to=ask_where_to)


def _show_route_api_mock(
    what_from, what_to, where_from, where_to, location_from, location_to,
    route_info, route_maps_uri, blocks, ask_where_to
):
    def response_callback(request, context):
        data = json.loads(request.body)
        form_slots = {slot['name']: slot for slot in data['form']['slots']}

        assert form_slots['what_from']['value'] == what_from
        assert form_slots['where_from']['value'] == where_from
        assert form_slots['what_to']['value'] == what_to
        assert form_slots['where_to']['value'] == where_to

        if ask_where_to:
            form_slots['where_to']['optional'] = False
        form_slots['resolved_location_from']['value'] = location_from
        form_slots['resolved_location_to']['value'] = location_to
        form_slots['route_info']['value'] = route_info
        form_slots['route_maps_uri']['value'] = route_maps_uri

        data.pop('meta')
        data['blocks'] = blocks

        return data

    return form_handling_mock(response_callback, balancer_type=SLOW_BASS_QUERY)


def traffic_api_mock(location, traffic_info, blocks=()):
    def response_callback(request, context):
        data = json.loads(request.body)
        form_slots = {slot['name']: slot for slot in data['form']['slots']}

        form_slots['resolved_where']['value'] = location
        form_slots['traffic_info']['value'] = traffic_info

        data.pop('meta')
        data['blocks'] = blocks

        return data

    return form_handling_mock(response_callback)


def general_conversation_mock():
    def response_callback(request, context):
        blocks = [
            {
                'type': 'suggest',
                'suggest_type': 'search_internet_fallback'
            }
        ]

        data = json.loads(request.body)
        data.pop('meta')
        data['blocks'] = blocks

        return data

    return form_handling_mock(response_callback)


def idfn(dialog_test_data):
    return getattr(dialog_test_data, 'get_app_info', lambda: None)()


def load_stubs(stubs_path='integration_data/stubs'):
    stubs_dict = {}
    for name, path in list_data_files(stubs_path):
        intents = []
        responses = []

        with open_resource_file(path) as f:
            timestamp = float(f.readline().strip())

            for line in f:
                # even lines correspond to intents and the odd ones correspond to responses
                if len(intents) == len(responses):
                    intents.append(line.strip())
                else:
                    responses.append(json.loads(line))

        stubs_dict[name] = (timestamp, intents, responses)

    return stubs_dict


def update_setup_bass_response(request, stubbed_response):
    """
    Updates list of forms in accordance to the list of request forms: rearranges them in the same order as in request
    and adds default form (without setup_meta) for forms not in stubs
    Post classifier is expected to correctly work both with setup_meta from new forms (in integration test)
    and without (in functional test), because this setup info corresponds to the incorrect intents
    """

    updated_response_forms = []

    for request_form in request['forms']:
        updated_response_form = {
            "setup_meta": {
                "is_feasible": True
            },
            "report_data": {
                "form": request_form
            }
        }
        for index, response_form in enumerate(stubbed_response['forms']):
            if response_form['report_data']['form']['name'] == request_form['name']:
                updated_response_form = stubbed_response['forms'][index]

        updated_response_forms.append(updated_response_form)

    stubbed_response['forms'] = updated_response_forms

    return stubbed_response
