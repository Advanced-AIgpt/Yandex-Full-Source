import alice.tests.library.scenario as scenario
import pytest


def is_iot(response):
    # FIXME: iot scenario should fill `intent` field in analytics_info
    return response.scenario in [scenario.IoT, scenario.IoTVoiceDiscovery, scenario.IoTScenarios]


# https://wiki.yandex-team.ru/users/aaulayev/iotanalyticsinfo#novyjjformatanalyticsinfo.
# At the moment there cannot be more than one reaction.
def get_iot_reaction(response):
    iot_reactions = response.scenario_analytics_info.object('iot_reactions')
    if not iot_reactions:
        return
    reactions = iot_reactions['reactions']

    return reactions[0] if reactions else None


def get_selected_hypothesis(response):
    hypotheses = response.scenario_analytics_info.object('hypotheses')
    selected_hypotheses = response.scenario_analytics_info.object('selected_hypotheses')
    assert hypotheses is not None and selected_hypotheses is not None
    hypotheses = hypotheses['hypotheses']
    selected_hypotheses = selected_hypotheses['selected_hypotheses']
    assert len(selected_hypotheses) == 1 and len(hypotheses) == 1
    selected_hypothesis = selected_hypotheses[0]
    hypothesis = hypotheses[0]
    assert selected_hypothesis['id'] == hypothesis['id']
    parsed_response = dict()
    for key, value in hypothesis:
        parsed_response[key] = value
    if 'scenario' not in parsed_response:
        parsed_response['devices'] = selected_hypothesis['devices']
    return parsed_response


def _normalize(response):
    return response.strip(' ')


def assert_response_text(response, valid_responses):
    assert _normalize(response) in valid_responses, f'{response} couldn\'t be ' \
                                                    f'matched to anything from {valid_responses}'


@pytest.fixture(autouse=True)
def no_config_skip(request):
    if request.node.get_closest_marker('iot') is None:
        pytest.skip('No config, skipping test')
