import re

music_intents = (
    "personal_assistant.scenarios.music_play",
    "personal_assistant.scenarios.vinsless.music",
)

video_intents = (
    "personal_assistant.scenarios.video_play",
    "personal_assistant.scenarios.quasar.goto_video_screen",
    "personal_assistant.scenarios.quasar.payment_confirmed",
    "personal_assistant.scenarios.quasar.authorize_video_provider",
    "personal_assistant.scenarios.quasar.open_current_video",
    "personal_assistant.scenarios.quasar.select_video_from_gallery_by_text",
    "personal_assistant.scenarios.quasar.select_video_from_gallery",
    "mm.personal_assistant.scenarios.video_play",
    "mm.personal_assistant.scenarios.quasar.goto_video_screen",
    "mm.personal_assistant.scenarios.quasar.payment_confirmed",
    "mm.personal_assistant.scenarios.quasar.authorize_video_provider",
    "mm.personal_assistant.scenarios.quasar.open_current_video",
    "mm.personal_assistant.scenarios.quasar.select_video_from_gallery_by_text",
    "mm.personal_assistant.scenarios.quasar.select_video_from_gallery",
)

non_video_intents = (
    "personal_assistant.scenarios.tv_broadcast",
)

player_intents = (
    "personal_assistant.scenarios.player_continue",
    "personal_assistant.scenarios.player_replay",
    "personal_assistant.scenarios.player_shuffle",
    "personal_assistant.scenarios.player_rewind",
)

vins_intents = (
    "personal_assistant.scenarios.bluetooth_on",
    "personal_assistant.scenarios.radio_play",
    "personal_assistant.scenarios.tv_stream",
    "personal_assistant.scenarios.timer_set",
    "personal_assistant.scenarios.quasar.go_forward",
    "personal_assistant.handcrafted",
    "personal_assistant.scenarios.alarm_set",
)

non_music_intents = (
    "personal_assistant.scenarios.sleep_timer_set",
)

non_music_toloka_intents = (
    "personal_assistant.scenarios.music_sing_song",
)

search_intents = (
    "personal_assistant.scenarios.search",
)

non_search_intents = (
    "personal_assistant.scenarios.get_time",
    "personal_assistant.scenarios.translate",
    "personal_assistant.scenarios.convert",
    "personal_assistant.scenarios.show_route",
    "personal_assistant.scenarios.get_date",
    "personal_assistant.scenarios.how_much",
    "personal_assistant.scenarios.timer_show",
)

protocol_intents = (
    "personal_assistant.scenarios.get_weather",
    "personal_assistant.scenarios.get_news",
    "personal_assistant.scenarios.ether.quasar.video_select",
    "personal_assistant.scenarios.sound_quiter",
    "personal_assistant.scenarios.sound_louder",
    "personal_assistant.scenarios.sound_set_level",
    "personal_assistant.scenarios.sound_get_level",
    "personal_assistant.scenarios.sound_mute",
    "personal_assistant.scenarios.sound_unmute",
    "personal_assistant.scenarios.iot_do",
    "personal_assistant.scenarios.search",
    "IoT",
    "personal_assistant.scenarios.common.irrelevant",
)

toloka_protocol_intents = (
    "action.iot_do",
)

non_gc_intents = (
    # "personal_assistant.scenarios.voiceprint_enroll",
    # "personal_assistant.scenarios.what_is_my_name",
)

gc_handcrafted_intents = (
    # 'general_conversation.feedback.negative.insult_general',
    # 'general_conversation.feedback.negative.insult_swearing',
    # 'general_conversation.feedback.negative.swearing',
    'general_conversation.tell.joke',
    'general_conversation.tell.poem',
)

music_sub_intents = (
    "personal_assistant.scenarios.music_fairy_tale",
    "personal_assistant.scenarios.music_ambient_sound",
)


def check_prefixes(intent, prefixes):
    tab_intent = intent.replace('\t', '.')
    return any([prefix in intent for prefix in prefixes]) or \
           any([prefix in tab_intent for prefix in prefixes])


def force_scenario(targets, forced_scenario):
    for scenario in targets:
        targets[scenario] = 0
    targets[forced_scenario] = 1
    return targets


def null_scenario(targets, null_scenario):
    targets[null_scenario] = 0
    return targets


def get_column(row, key, default):
    return row[key] if key in row and row[key] is not None else default


def mark_column_name(scenario):
    return scenario + '_mark'


def bias_make_vins_target(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    vins_mark = targets['vins']
    if check_prefixes(vins_intent, video_intents) and targets.get('video', -100) >= vins_mark:
        return null_scenario(targets, 'vins'), True
    if check_prefixes(vins_intent, music_intents) and targets.get('music', -100) >= vins_mark:
        return null_scenario(targets, 'vins'), True
    if check_prefixes(vins_intent, search_intents) and targets.get('search', -100) >= vins_mark:
        return null_scenario(targets, 'vins'), True
    return targets, False


def bias_make_music_target(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    music_mark = targets['music']
    vins_mark = targets.get('vins', -100)
    if check_prefixes(vins_intent, non_music_intents) and vins_mark >= music_mark:
        return null_scenario(targets, 'music'), True
    if check_prefixes(vins_intent, vins_intents) and vins_mark >= music_mark:
        return null_scenario(targets, 'music'), True
    if check_prefixes(vins_intent, ['personal_assistant.scenarios.tv_broadcast']) and vins_mark >= music_mark:
        return null_scenario(targets, 'music'), True
    return targets, False


def bias_make_video_target(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    video_mark = targets['video']
    vins_mark = targets.get('vins', -100)
    if check_prefixes(vins_intent, non_video_intents) and vins_mark >= video_mark:
        return null_scenario(targets, 'video'), True
    if check_prefixes(vins_intent, vins_intents) and vins_mark >= video_mark:
        return null_scenario(targets, 'video'), True
    return targets, False


def bias_make_search_target(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    search_mark = targets['search']
    vins_mark = targets.get('vins', -100)
    if check_prefixes(vins_intent, vins_intents) and vins_mark >= search_mark:
        return null_scenario(targets, 'search'), True
    if check_prefixes(vins_intent, non_search_intents) and vins_mark >= search_mark:
        return null_scenario(targets, 'search'), True
    return targets, False


def bias_make_search_target_on_touch(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    search_mark = targets['search']
    vins_mark = targets.get('vins', -100)
    non_search_on_touch = (
        "how_much",
        "personal_assistant.scenarios.taxi",
        "personal_assistant.scenarios.open_site_or_app",
        "personal_assistant.scenarios.find_poi",
        "personal_assistant.scenarios.direct_gallery",
        "personal_assistant.scenarios.image_gallery",
    )
    if check_prefixes(vins_intent, non_search_on_touch) and vins_mark >= search_mark:
        return null_scenario(targets, 'search'), True
    return targets, False


def bias_disable_search_on_vins_tv_broadcast(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    search_mark = targets['search']
    vins_mark = targets.get('vins', -100)
    if check_prefixes(vins_intent, ['personal_assistant.scenarios.tv_broadcast']) and vins_mark >= search_mark:
        return null_scenario(targets, 'search'), True
    return targets, False


def bias_make_search_target_music_intents(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    search_mark = targets['search']
    music_mark = targets.get('music', -100)
    vins_mark = targets.get('vins', -100)
    if (check_prefixes(vins_intent, music_sub_intents) or check_prefixes(toloka_intent, music_sub_intents)) and \
            (music_mark >= search_mark or vins_mark >= search_mark):
        return null_scenario(targets, 'search'), True
    return targets, False


def bias_make_gc_target(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    gc_mark = targets['gc']
    vins_mark = targets['vins']
    if vins_mark > 0 and check_prefixes(vins_intent, ['personal_assistant.general_conversation.general_conversation']):
        targets['gc'] = max(vins_mark, gc_mark)
        return targets, True
    return targets, False


def bias_make_gc_target_without_dummy(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    gc_mark = targets['gc']
    vins_mark = targets['vins']
    if vins_mark > 0 and check_prefixes(vins_intent, ['personal_assistant.general_conversation.general_conversation']) and \
            not check_prefixes(vins_intent, ['general_conversation_dummy']):
        targets['gc'] = max(vins_mark, gc_mark)
        return targets, True
    return targets, False


def bias_swap_vins_to_music(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    if check_prefixes(new_vins_intent, music_intents) and \
            not check_prefixes(toloka_intent, non_music_toloka_intents):
        targets['music'] = max(targets['music'], targets.get('vins', 0))
        return null_scenario(targets, 'vins'), True
    return targets, False


def bias_not_video_on_tv_stream(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    if toloka_intent == 'personal_assistant.scenarios.tv_stream':
        return null_scenario(targets, 'video'), True
    return targets, False


def bias_force_music_by_toloka(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    if toloka_intent == 'personal_assistant.scenarios.music_play' and \
            (marks['music'] > 0 or targets.get('video', -1) == 0) and \
            not check_prefixes(vins_intent, player_intents):
        return force_scenario(targets, 'music'), True
    return targets, False


def bias_force_video_by_toloka(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    device_state = get_column(row, 'device_state', None)
    is_tv_plugged_in = device_state.get('is_tv_plugged_in', False) if device_state is not None else False
    if is_tv_plugged_in and toloka_intent in (
            'personal_assistant.scenarios.quasar.payment_confirmed',
            'personal_assistant.scenarios.quasar.open_current_video',
            'personal_assistant.scenarios.quasar.select_video_from_gallery'
            ):
        return force_scenario(targets, 'video'), True  # force video
    if toloka_intent == 'personal_assistant.scenarios.video_play':
        if marks.get('music', 0) > 0 and not is_tv_plugged_in:
            return force_scenario(targets, 'music'), True
        elif marks['video'] > 0 or targets['music'] == 0:
            return force_scenario(targets, 'video'), True
    return targets, False


def bias_disable_search_on_what_date(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    if check_prefixes(raw_toloka_intent, ['action.time.what_date']):
        return null_scenario(targets, 'search'), True
    return targets, False


def bias_force_search_by_toloka_or_gc(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    if targets['search'] > 0 and (toloka_intent == 'personal_assistant.scenarios.search' or
                                  check_prefixes(vins_intent, ['personal_assistant.general_conversation.general_conversation'])):
        targets['search'] = 1
        if 'vins' in targets:
            targets['vins'] = 0
        if 'gc' in targets:
            targets['gc'] = 0
        return targets, True
    return targets, False


def bias_disable_vins_on_gc(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    if targets['gc'] > 0 and check_prefixes(vins_intent, ['personal_assistant.general_conversation.general_conversation']):
        return null_scenario(targets, 'vins'), True
    return targets, False


def could_force_search_by_search_or_toloka(vins_intent, toloka_intent, text):
    regexp = '(^| )(кто|что|как|почему|во сколько|сколько.* в|сколько лет|сколько будет|какие|какой|какая|какое|когда|найди|найти|поиск|где.* купить'
    regexp += '|покажи|скажи|расскажи|готовить|приготовить|приготовление|рецепт|рецепты|варить|сварить|ингредиент)($| )'
    from_vins = check_prefixes(vins_intent, search_intents) and not check_prefixes(toloka_intent, ['personal_assistant.scenarios.games_onboarding'])
    from_toloka = toloka_intent == 'personal_assistant.scenarios.search' and re.match(regexp, text, re.UNICODE)
    return from_vins, from_toloka


def bias_make_search_from_vins(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    return bias_make_search_from_vins_or_toloka(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row, True, False)


def bias_make_search_from_toloka(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    return bias_make_search_from_vins_or_toloka(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row, False, True)


def bias_make_search_from_vins_and_toloka(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    return bias_make_search_from_vins_or_toloka(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row, True, True)


def bias_make_search_from_vins_or_toloka(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row,
                                         use_search_from_vins, use_search_from_toloka):
    if targets.get('music', 0) + targets.get('video', 0) == 0:
        from_vins, from_toloka = could_force_search_by_search_or_toloka(vins_intent, toloka_intent, row['text'])
        if (use_search_from_vins and use_search_from_toloka and from_vins and from_toloka) or \
                (use_search_from_vins and not use_search_from_toloka and from_vins) or \
                (use_search_from_toloka and not use_search_from_vins and from_toloka):
            targets['search'] = 1
            if 'vins' in targets:
                targets['vins'] = 0
            return targets, True
    return targets, False


def bias_vins_fallback(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row, should_filter_protocol=False):
    filter_protocol = check_prefixes(raw_toloka_intent, toloka_protocol_intents) and should_filter_protocol
    if (targets.get('music', 0) + targets.get('video', 0) + targets.get('search', 0) + targets.get('gc', 0)) <= 0 and \
            not filter_protocol and \
            targets['vins'] < 1:
        targets['vins'] = 1
        return targets, True
    return targets, False


def bias_vins_fallback_filter_protocol(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    return bias_vins_fallback(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row, should_filter_protocol=True)


def bias_vins_fallback_on_touch(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    if (targets.get('music', 0) + targets.get('video', 0) + targets.get('search', 0) + targets.get('gc', 0)) <= 0 and \
            targets['vins'] < 1:
        targets['vins'] = 1
        return targets, True
    return targets, False


def bias_disable_vins_on_protocol_searchapp(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    if (check_prefixes(vins_intent, protocol_intents) or check_prefixes(new_vins_intent, protocol_intents)) and \
            not check_prefixes(toloka_intent, ['personal_assistant.scenarios.games_onboarding']):
        return null_scenario(targets, 'vins'), True
    return targets, False


def bias_disable_vins_on_protocol(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    if check_prefixes(vins_intent, protocol_intents) or check_prefixes(new_vins_intent, protocol_intents):
        return null_scenario(targets, 'vins'), True
    return targets, False


def bias_gc_by_toloka_dialog(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    gc_from_toloka = check_prefixes(raw_toloka_intent, ['general_conversation.dialog']) and \
        not check_prefixes(new_vins_intent, non_gc_intents)
    targets['gc'] = gc_from_toloka
    if gc_from_toloka:
        return targets, True
    else:
        return targets, False


def bias_gc_by_toloka_dialog_maxed(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    gc_from_toloka = check_prefixes(raw_toloka_intent, ['general_conversation.dialog']) and \
        not check_prefixes(new_vins_intent, non_gc_intents)
    targets['gc'] = max(gc_from_toloka, targets['gc'])
    if gc_from_toloka:
        return targets, True
    else:
        return targets, False


def bias_gc_by_vins_handcrafted(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    gc_handcrafted = marks['vins'] > 0 and check_prefixes(raw_toloka_intent, gc_handcrafted_intents) and \
        check_prefixes(new_vins_intent, ['personal_assistant.handcrafted'])
    if gc_handcrafted:
        targets['gc'] = max(targets['gc'], gc_handcrafted)
        return targets, True
    return targets, False


def bias_disable_vins_on_toloka_gc(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    gc_from_toloka = check_prefixes(raw_toloka_intent, ['general_conversation.dialog']) and \
        not check_prefixes(new_vins_intent, non_gc_intents)
    if (gc_from_toloka and check_prefixes(new_vins_intent, ['personal_assistant.general_conversation.general_conversation'])):
        return null_scenario(targets, 'vins'), True
    return targets, False


def bias_disable_toponyms_in_search(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    if raw_toloka_intent == 'search_on_map.toponym':
        return null_scenario(targets, 'search'), True
    return targets, False


def bias_disable_music_sub_intents_in_search(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    if check_prefixes(toloka_intent, music_sub_intents) or check_prefixes(new_vins_intent, music_sub_intents):
        targets['vins'] = 1
        targets['search'] = 0
        return targets, True
    return targets, False


def bias_force_vins_on_games_onboarding(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row):
    if check_prefixes(toloka_intent, ['personal_assistant.scenarios.games_onboarding']):
        return force_scenario(targets, 'vins'), True
    return targets, False


class TBias(object):
    def __init__(self, clients, scenarios, flags, condition):
        self.clients = clients
        self.scenarios = scenarios
        self.flags = flags
        self.condition = condition

    def is_enabled(self, client, scenarios, flags):
        is_enabled = client in self.clients
        is_enabled = is_enabled and all([scenario in scenarios for scenario in self.scenarios])
        is_enabled = is_enabled and all([flag in flags for flag in self.flags])
        return is_enabled

    def call(self, *args):
        targets, aplied = self.condition(*args)
        return targets, aplied, self.condition.__name__

all_clients = ['ECT_SMART_SPEAKER', 'ECT_SMART_TV', 'ECT_TOUCH', 'ECT_NAVIGATION']
speakers_and_tv = ['ECT_SMART_SPEAKER', 'ECT_SMART_TV']
touch_and_navigation = ['ECT_TOUCH', 'ECT_NAVIGATION']

target_biases = [
    TBias(
        clients=all_clients,
        scenarios=['vins'],
        flags=[],
        condition=bias_make_vins_target
    ),
    TBias(
        clients=all_clients,
        scenarios=['search', 'vins'],
        flags=[],
        condition=bias_make_search_target
    ),
    TBias(
        clients=touch_and_navigation,
        scenarios=['search', 'vins'],
        flags=[],
        condition=bias_make_search_target_on_touch
    ),
    TBias(
        clients=touch_and_navigation,
        scenarios=['search'],
        flags=[],
        condition=bias_make_search_target_music_intents
    ),
    TBias(
        clients=touch_and_navigation,
        scenarios=['search'],
        flags=[],
        condition=bias_disable_search_on_vins_tv_broadcast
    ),
    TBias(
        clients=all_clients,
        scenarios=['music'],
        flags=[],
        condition=bias_make_music_target
    ),
    TBias(
        clients=all_clients,
        scenarios=['video'],
        flags=[],
        condition=bias_make_video_target
    ),
    TBias(
        clients=speakers_and_tv,
        scenarios=['gc', 'vins'],
        flags=['add_gc'],
        condition=bias_make_gc_target
    ),
    TBias(
        clients=touch_and_navigation,
        scenarios=['gc', 'vins'],
        flags=['add_gc'],
        condition=bias_make_gc_target_without_dummy
    ),
    TBias(
        clients=speakers_and_tv,
        scenarios=['gc', 'vins'],
        flags=['add_gc', 'not_raw_scores'],
        condition=bias_disable_vins_on_gc
    ),
    TBias(
        clients=['ECT_NAVIGATION'],
        scenarios=['vins', 'search'],
        flags=['classification_bias', 'not_raw_scores'],
        condition=bias_disable_toponyms_in_search
    ),
    TBias(
        clients=touch_and_navigation,
        scenarios=['vins', 'search'],
        flags=['classification_bias', 'not_raw_scores'],
        condition=bias_disable_music_sub_intents_in_search
    ),
    TBias(
        clients=all_clients,
        scenarios=['music'],
        flags=['classification_bias', 'not_raw_scores'],
        condition=bias_swap_vins_to_music
    ),
    TBias(
        clients=all_clients,
        scenarios=['video'],
        flags=['classification_bias', 'not_raw_scores'],
        condition=bias_not_video_on_tv_stream
    ),
    TBias(
        clients=all_clients,
        scenarios=['music'],
        flags=['classification_bias', 'not_raw_scores'],
        condition=bias_force_music_by_toloka
    ),
    TBias(
        clients=all_clients,
        scenarios=['video'],
        flags=['classification_bias', 'not_raw_scores'],
        condition=bias_force_video_by_toloka
    ),
    TBias(
        clients=speakers_and_tv,
        scenarios=['search', 'vins'],
        flags=['classification_bias', 'not_raw_scores'],
        condition=bias_disable_search_on_what_date
    ),
    TBias(
        clients=all_clients,
        scenarios=['search'],
        flags=['classification_bias', 'search_bias_toloka', 'not_raw_scores'],
        condition=bias_force_search_by_toloka_or_gc
    ),
    TBias(
        clients=touch_and_navigation,
        scenarios=['vins'],
        flags=['classification_bias', 'not_raw_scores'],
        condition=bias_force_vins_on_games_onboarding
    ),
    TBias(
        clients=all_clients,
        scenarios=['search', 'vins'],
        flags=['not_raw_scores', 'search_from_vins'],
        condition=bias_make_search_from_vins
    ),
    TBias(
        clients=all_clients,
        scenarios=['search'],
        flags=['not_raw_scores', 'search_from_toloka'],
        condition=bias_make_search_from_toloka
    ),
    TBias(
        clients=all_clients,
        scenarios=['search', 'vins'],
        flags=['not_raw_scores', 'search_from_vins', 'search_from_toloka'],
        condition=bias_make_search_from_vins_and_toloka
    ),
    TBias(
        clients=touch_and_navigation,
        scenarios=['gc'],
        flags=['not_raw_scores', 'add_gc'],
        condition=bias_gc_by_toloka_dialog_maxed,
    ),
    TBias(
        clients=speakers_and_tv,
        scenarios=['gc'],
        flags=['not_raw_scores', 'add_gc'],
        condition=bias_gc_by_toloka_dialog,
    ),
    TBias(
        clients=speakers_and_tv,
        scenarios=['gc', 'vins'],
        flags=['not_raw_scores', 'add_gc'],
        condition=bias_gc_by_vins_handcrafted,
    ),
    TBias(
        clients=touch_and_navigation,
        scenarios=['gc', 'vins'],
        flags=['not_raw_scores', 'add_gc', 'gc_on_handcrafted'],
        condition=bias_gc_by_vins_handcrafted,
    ),
    TBias(
        clients=speakers_and_tv,
        scenarios=['vins'],
        flags=['not_raw_scores', 'add_gc'],
        condition=bias_disable_vins_on_toloka_gc,
    ),
    TBias(
        clients=touch_and_navigation,
        scenarios=['vins', 'search'],
        flags=['not_raw_scores'],
        condition=bias_disable_music_sub_intents_in_search,
    ),
    TBias(
        clients=speakers_and_tv,
        scenarios=['vins'],
        flags=['not_raw_scores', 'vins_fallback'],
        condition=bias_vins_fallback_filter_protocol,
    ),
    TBias(
        clients=touch_and_navigation,
        scenarios=['vins'],
        flags=['not_raw_scores', 'vins_fallback'],
        condition=bias_vins_fallback,
    ),
    TBias(
        clients=speakers_and_tv,
        scenarios=['vins'],
        flags=['not_raw_scores'],
        condition=bias_disable_vins_on_protocol,
    ),
    TBias(
        clients=touch_and_navigation,
        scenarios=['vins'],
        flags=['not_raw_scores'],
        condition=bias_disable_vins_on_protocol_searchapp,
    ),
]
