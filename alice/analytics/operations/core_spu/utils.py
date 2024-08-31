import json

from nile.api.v1 import with_hints
from alice.analytics.utils.json_utils import get_path


def mapper_wrapper(cls):
    def wrapped(*args, **kwargs):
        instance = cls(*args, **kwargs)
        return with_hints(instance.output_schema)(instance)

    return wrapped


def get_music_slots(analytics_info):
    slots = get_path(
        analytics_info,
        ['analytics_info', 'HollywoodMusic', 'matched_semantic_frames', 0, 'slots']
    ) or get_path(
        analytics_info,
        ['analytics_info', 'alice.vinsless.music', 'matched_semantic_frames', 0, 'slots']
    )

    return slots or []


def get_music_filters(slots):
    filters = list(map(
        lambda x: x.get('name'),
        filter(lambda x: x.get('name') != 'action_request' and x.get('value') != 'autoplay', slots)
    ))

    return filters or []


def get_video_filters(analytics_info):
    frames = get_path(analytics_info, ['analytics_info', 'Video', 'matched_semantic_frames'], [])
    slots = get_path(list(filter(lambda x: x.get('name') == 'personal_assistant.scenarios.video_play', frames)), [0, 'slots'], [])
    filters = list(map(lambda x: {'name': x.get('name'), 'value': x.get('value')}, slots))

    return filters or []


def is_regular_alarm(analytics_info):
    slots = get_path(
        analytics_info,
        ['analytics_info', 'Vins', 'semantic_frame', 'slots']
    ) or []

    for slot in slots:
        try:
            setting = json.loads(slot['typed_value']['string'])
            if setting['repeat']:
                return True
        except:
            continue
    return False


def is_custom_iot_scenario(intent, analytics_info, query):
    if intent in ('IoT', 'iot'):
        try:
            objects = get_path(
                analytics_info,
                ['analytics_info', 'IoT', 'scenario_analytics_info', 'objects']
            )
            for obj in objects:
                hypotheses = get_path(
                    obj,
                    ['hypotheses', 'hypotheses']
                )

                for hypothese in hypotheses:
                    if hypothese.get('scenario'):
                        return True
        except:
            return False

        return False
    elif intent == 'personal_assistant\tscenarios\tiot_do':
        scenarios = get_path(
            analytics_info,
            ['users_info', 'alice.iot_do', 'scenario_user_info', 'properties', 0, 'iot_profile', 'scenarios'], []
        )

        scenarios_names_words = list(map(lambda x: set(x.get('name').lower().split(' ')), scenarios))
        if set(query.lower().split(' ')) in scenarios_names_words:
            return True

        return False
