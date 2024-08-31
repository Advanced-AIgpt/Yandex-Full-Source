import json
import re

from google.protobuf import json_format

from alice.megamind.protos.scenarios.directives_pb2 import TCallbackDirective
from alice.protos.extensions.extensions_pb2 import SpeechKitName

from vins_core.utils.strings import smart_utf8


def get_directive_case(directive):
    directive_case = directive.WhichOneof('Directive')
    assert directive_case is not None, 'Directive must be initialized'
    return getattr(directive, directive_case)


def get_sk_name(directive):
    return directive.DESCRIPTOR.GetOptions().Extensions[SpeechKitName]


def _is_server_action(directive):
    return isinstance(directive, TCallbackDirective)


def _is_client_action(directive):
    return not _is_server_action(directive)


def _match_payload_client_action(payload, directive):
    directive_dict = json_format.MessageToDict(directive)
    for k, v in payload.iteritems():
        if directive_dict.get(k) != v:
            return False
    return True


def _match_payload_server_action(payload, directive):
    directive_payload = json_format.MessageToDict(directive.Payload)
    return payload == directive_payload


def _match_utterance(utterance_regexp, directives, directive_name):
    if utterance_regexp is None:
        return None

    type_client_action_found = False
    for directive in directives:
        if _is_client_action(directive) and get_sk_name(directive) == directive_name:
            if re.match(utterance_regexp, directive.Text.decode('utf-8'), re.UNICODE):
                type_client_action_found = True

    if not type_client_action_found:
        return 'Unable to find "{}" directive with text matching regexp: "{}" in directives: {}'.format(
            smart_utf8(directive_name),
            smart_utf8(utterance_regexp),
            [json_format.MessageToJson(directive) for directive in directives]
        )
    return None


def _match_directive_payload(expected_name, expected_payload, actual_directive):
    matched_client_action_directive = (
        _is_client_action(actual_directive) and
        get_sk_name(actual_directive) == expected_name and
        _match_payload_client_action(expected_payload, actual_directive)
    )

    matched_server_action_directive = (
        _is_server_action(actual_directive) and
        actual_directive.Name == expected_name and
        _match_payload_server_action(expected_payload, actual_directive)
    )

    return matched_client_action_directive or matched_server_action_directive


def _match_directive(suggest_directive, payload, directives, forbidden_directive_names):
    if suggest_directive is None or payload is None or suggest_directive in forbidden_directive_names:
        return None

    for directive in directives:
        if _match_directive_payload(suggest_directive, payload, directive):
            return None

    return 'Unable to find: {}, payload: {} in directives: {}'.format(
        smart_utf8(suggest_directive),
        json.dumps(payload),
        [json_format.MessageToJson(directive) for directive in directives]
    )


def _match_suggest(expected_data_suggest, directives, forbidden_directive_names):
    checks = (
        _match_utterance(expected_data_suggest.get('utterance'), directives, 'type'),
        _match_utterance(expected_data_suggest.get('user_utterance'), directives, 'type_silent'),
        _match_directive(expected_data_suggest.get('directive'), expected_data_suggest.get('payload'), directives,
                         forbidden_directive_names)
    )

    return next((check for check in checks if check is not None), None)


def _match_suggests(expected_data_suggests, actual_suggests, actions, forbidden_directive_names):
    if len(expected_data_suggests) != len(actual_suggests):
        return "size of actual suggest list ({}) doesn't match size of expected data suggest list ({})".format(
            len(actual_suggests), len(expected_data_suggests))

    for expected_data_suggest, actual_suggest in zip(expected_data_suggests, actual_suggests):
        if not _match_suggest(expected_data_suggest,
                              [get_directive_case(d) for d in actions[actual_suggest.ActionId].Directives.List],
                              forbidden_directive_names):
            return "expected data suggest {} doesn't match test suggest {}, directives: {}".format(
                json.dumps(expected_data_suggest),
                json_format.MessageToJson(actual_suggest),
                [json_format.MessageToJson(directive) for directive in actions[actual_suggest.ActionId].Directives.List]
            )
    return None


def _find_suggest(expected_suggest, suggests_response, actions, forbidden_directive_names):
    for candidate_suggest in suggests_response:
        if _match_suggest(expected_suggest,
                          [get_directive_case(d) for d in actions[candidate_suggest.ActionId].Directives.List],
                          forbidden_directive_names) is None:
            return True
    return False


def suggests(expected_data_suggests, exact_suggests_match, suggests, actions, forbidden_directive_names):
    if exact_suggests_match:
        return _match_suggests(expected_data_suggests, suggests, actions, forbidden_directive_names)
    else:
        for expected_data_suggest in expected_data_suggests or []:
            if not _find_suggest(expected_data_suggest, suggests, actions, forbidden_directive_names):
                return 'Unable to find {} inside response {}, actions: {}'.format(
                    json.dumps(expected_data_suggest),
                    [json_format.MessageToJson(suggest) for suggest in suggests],
                    {k: json_format.MessageToJson(actions[k]) for k in actions}
                )
        return None


def _find_button_action(expected_button_action, cards_response, actions):
    for card in cards_response:
        if card.WhichOneof('Card') != 'TextWithButtons':
            continue

        for button in card.TextWithButtons.Buttons:
            if button.Title == smart_utf8(expected_button_action['title']):
                for directive in map(get_directive_case, actions[button.ActionId].Directives.List):
                    if _match_directive_payload(expected_button_action['name'], expected_button_action.get('payload'),
                                                directive):
                        return True

    return False


def button_actions(expected_button_actions, cards, actions, forbidden_directive_names):
    for expected_button_action in expected_button_actions or []:
        if expected_button_action['name'] in forbidden_directive_names:
            continue

        if not _find_button_action(expected_button_action, cards, actions):
            return 'Unable to find action button: {} in cards: {}, actions: {}'.format(
                json.dumps(expected_button_action),
                [json_format.MessageToJson(card) for card in cards],
                {k: json_format.MessageToJson(actions[k]) for k in actions}
            )

    return None
