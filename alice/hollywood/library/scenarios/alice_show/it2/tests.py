# To run tests in RUNNER mode: ya make -Ar
# To run tests in GENERATOR mode: ya make -Ar -DIT2_GENERATOR -Z

import pytest
import logging

from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import server_action, voice, callback

from alice.hollywood.library.scenarios.alice_show.proto.state_pb2 import TAliceShowState
from alice.hollywood.library.scenarios.alice_show.proto.scenario_data_pb2 import TAliceShowScenarioData

from alice.megamind.protos.common.frame_pb2 import TMusicPlayObjectTypeSlot
from alice.megamind.protos.common.directive_channel_pb2 import TDirectiveChannel


logger = logging.getLogger(__name__)

SCENARIO_NAME = 'AliceShow'
SCENARIO_HANDLE = 'alice_show'

DEFAULT_EXPERIMENTS = [
    'hw_enable_evening_show_good_evening',
    'hw_enable_good_night_show_good_night',
    'hw_alice_show_enable_push',
]

GET_NEXT_SHOW_BLOCK_CALLBACK = 'alice_show_get_next_show_block'
CONTINUE_PART_CALLBACK = 'alice_show_continue_part'

NEWS_RUBRICS = (
    "business", "computers", "culture", "incident", "index", "kurezy",
    "science", "showbusiness", "society", "sport", "world",
)
NEWS_SOURCES = (
    "1197544f-radio-kultura", "2e6b92ff-radio-maya", "35376ef1-ria-novosti", "43e0281d-gazeta-ru",
    "6f852374-serebryanyj-dozh", "74ee5150-disnej", "86db974e-biznes-fm",
    "945b76fa-vesti-fm", "9c8344ec-hit-fm", "9c9dc75f-kommersant-fm", "a7fce137-rambler-novosti", "bed86bf3-vc-ru", "c16d4bd9-n-1",
    "ced8d648-radio-rossii", "d0cb2ee9-life-ru", "d0f021cb-dtf", "d4777f11-kommersantu", "de9af378-rbk",
    "e3a1395f-lenta-ru", "fa30936e-komsomol-skaya-pravd", "fa58ac81-moskovskij-komsomolec",
)
NEWS_DEFAULT_SOURCE = "6e24a5bb-yandeks-novost"

DEFAULT_MEMENTO = {
    "UserConfigs": {
        "MorningShowNewsConfig": {
            "Default": True,
            "NewsProviders": [{
                "NewsSource": NEWS_DEFAULT_SOURCE,
                "Rubric": "__mixed_news__"
            }]
        },
        "MorningShowTopicsConfig": {
            "Default": True
        },
        "MorningShowSkillsConfig": {
            "Default": True
        }
    }
}

RUBRIC_MEMENTO = {
    "UserConfigs": {
        "MorningShowNewsConfig": {
            "Default": True,
            "NewsProviders": [{
                "NewsSource": NEWS_DEFAULT_SOURCE,
                "Rubric": "science"
            }]
        },
        "MorningShowTopicsConfig": {
            "Default": True
        },
        "MorningShowSkillsConfig": {
            "Default": True
        }
    }
}

MIXED_MEMENTO = {
    "UserConfigs": {
        "MorningShowNewsConfig": {
            "Default": True,
            "NewsProviders": [
                {
                    "NewsSource": NEWS_DEFAULT_SOURCE,
                    "Rubric": "science"
                },
                {
                    "NewsSource": NEWS_DEFAULT_SOURCE,
                    "Rubric": "culture"
                },
                {
                    "NewsSource": NEWS_DEFAULT_SOURCE,
                    "Rubric": "computers"
                },
                {
                    "NewsSource": "35376ef1-ria-novosti",
                    "Rubric": "main"
                }
            ]
        },
        "MorningShowTopicsConfig": {
            "Default": True
        },
        "MorningShowSkillsConfig": {
            "Default": True
        }
    }
}

# (callback_name, frame_name, show_index, part_index, track_index, news_index, hardcoded_show_index, {...})
SEQUENCE = (
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  'GetTimeSemanticFrame',     2, 0, 0, 0, 1),
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  'WeatherSemanticFrame',     3, 0, 0, 0, 2),
    (CONTINUE_PART_CALLBACK,        'NewsSemanticFrame',        4, 1, 0, 1, 3, {'news_source': '35376ef1-ria-novosti'}),
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  None,                       4, 0, 0, 1, 3, {'no_content': True}),
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  None,                       5, 0, 0, 1, 3, {'no_content': True}),
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  'MusicPlaySemanticFrame',   6, 0, 1, 1, 4),
    (None,                          'HardcodedMorningShowSemanticFrame', 7, 0, 1, 1, 4),
)


@pytest.fixture(scope='module')
def enabled_scenarios():
    return [SCENARIO_HANDLE]


def _get_seq_attr(seq, name, default=None):
    if len(seq) <= 7:
        return default
    return seq[7].get(name, default)


def _get_state(response):
    state = TAliceShowState()
    response.ResponseBody.State.Unpack(state)
    return state


def _get_scenario_data(any):
    scenario_data = TAliceShowScenarioData()
    any.Unpack(scenario_data)
    return scenario_data


def _assert_channel(response_body):
    for directive in response_body.Layout.Directives:
        if directive.HasField('TtsPlayPlaceholderDirective'):
            assert directive.TtsPlayPlaceholderDirective.DirectiveChannel == TDirectiveChannel.Content
    for action in response_body.StackEngine.Actions:
        if action.HasField('ResetAdd'):
            for effect in action.ResetAdd.Effects:
                assert effect.Options.Channel == TDirectiveChannel.Content


def _assert_directive_order(response_body):
    directive_trait = {
        'DrawLedScreenDirective': {'order': 10, 'require': ['TtsPlayPlaceholderDirective']},
        'TtsPlayPlaceholderDirective': {'order': 20},
        'ListenDirective': {'order': 30},
    }

    max = 0
    found = set()
    for directive in response_body.Layout.Directives:
        type = directive.WhichOneof('Directive')
        found.add(type)
        trait = directive_trait.get(type, {})
        order = trait.get('order', None)
        if order:
            # strictly less, no duplicates allowed
            assert max < order, 'Incorrect directive order'
            max = order

    for type in found:
        trait = directive_trait.get(type, {})
        for req in trait.get('require', []):
            assert req in found, type + ' requires ' + req


def _assert_show_start_base(r, show_type, show_type_snake, age='adult'):
    assert r.scenario_stages() == {'run'}
    response_body = r.run_response.ResponseBody
    _assert_channel(response_body)
    _assert_directive_order(response_body)
    assert response_body.StackEngine.Actions[0].NewSession
    assert response_body.StackEngine.Actions[1].HasField('ResetAdd')
    state = _get_state(r.run_response)
    assert state.ShowType == show_type
    assert state.Stage.ShowIndex == 1
    assert response_body.AnalyticsInfo.Actions[0].Id == 'alice_show.start'
    assert response_body.AnalyticsInfo.Objects[0].Id == 'show.type'
    assert response_body.AnalyticsInfo.Objects[0].Name == show_type_snake
    assert response_body.AnalyticsInfo.Objects[1].Id == 'show.age'
    assert response_body.AnalyticsInfo.Objects[1].Name == age
    return response_body


def _try_find_server_directive(response_body, directive_type):
    for d in response_body.ServerDirectives:
        if d.HasField(directive_type):
            return getattr(d, directive_type)
    return None


def _find_server_directive(response_body, directive_type):
    d = _try_find_server_directive(response_body, directive_type)
    if d is None:
        raise AssertionError('{} not found in response'.format(directive_type))
    return d


def _assert_show_start(r, show_type, show_type_snake, age='adult'):
    response_body = _assert_show_start_base(r, show_type, show_type_snake, age)
    reset_add = response_body.StackEngine.Actions[1].ResetAdd
    assert reset_add.Effects[0].Callback.Name == GET_NEXT_SHOW_BLOCK_CALLBACK
    memento = _find_server_directive(response_body, 'MementoChangeUserObjectsDirective')
    scenario_data = _get_scenario_data(memento.UserObjects.ScenarioData)
    assert scenario_data.Onboarded
    _find_server_directive(response_body, 'SendPushDirective')
    assert scenario_data.PushesSent == 1
    return r


def _assert_show_start_music(r, play_object, show_type, show_type_snake, age='adult'):
    response_body = _assert_show_start_base(r, show_type, show_type_snake, age)
    assert len(response_body.StackEngine.Actions) > 1
    reset_add = response_body.StackEngine.Actions[1].ResetAdd
    assert len(reset_add.Effects) > 0
    frame = reset_add.Effects[0].ParsedUtterance.TypedSemanticFrame
    assert frame.HasField('MusicPlaySemanticFrame')
    assert frame.MusicPlaySemanticFrame.ObjectType.EnumValue == play_object
    return r


def _assert_show_start_playlist(r, show_type, show_type_snake, age='adult'):
    _assert_show_start_music(r, TMusicPlayObjectTypeSlot.EValue.Playlist, show_type, show_type_snake, age)


def _assert_show_start_album(r, show_type, show_type_snake, age='adult'):
    _assert_show_start_music(r, TMusicPlayObjectTypeSlot.EValue.Album, show_type, show_type_snake, age)


def _assert_show_start_hardcoded(r, show_type, show_type_snake, age='adult', type='', offset=0, next_track=0):
    response_body = _assert_show_start_base(r, show_type, show_type_snake, age)
    assert len(response_body.StackEngine.Actions) > 1
    reset_add = response_body.StackEngine.Actions[1].ResetAdd
    assert len(reset_add.Effects) > 0
    frame = reset_add.Effects[0].ParsedUtterance.TypedSemanticFrame
    assert frame.HasField('HardcodedMorningShowSemanticFrame')
    assert frame.HardcodedMorningShowSemanticFrame.ShowType.StringValue == type
    assert frame.HardcodedMorningShowSemanticFrame.Offset.NumValue == offset
    assert frame.HardcodedMorningShowSemanticFrame.NextTrackIndex.NumValue == next_track
    return r


def _assert_show_start_children(r):
    return _assert_show_start_hardcoded(r, TAliceShowState.EShowType.Children, 'children', 'children', 'children')


def _assert_show_action(r, action_id, seq):
    assert r.scenario_stages() == {'run'}
    response_body = r.run_response.ResponseBody
    _assert_directive_order(response_body)
    reset_add = response_body.StackEngine.Actions[0].ResetAdd
    state = _get_state(r.run_response)
    (callback_name, frame_name, show_index, part_index, track_index, news_index, hardcoded_show_index, *_) = seq
    if callback_name:
        assert reset_add.Effects[0].Callback.Name == callback_name
        if frame_name:
            assert len(reset_add.Effects) == 2
            assert reset_add.Effects[1].ParsedUtterance.TypedSemanticFrame.HasField(frame_name)
            frame = getattr(reset_add.Effects[1].ParsedUtterance.TypedSemanticFrame, frame_name)
            if frame_name == 'NewsSemanticFrame':
                assert frame.Provider.NewsProviderValue.NewsSource in _get_seq_attr(seq, 'news_source', NEWS_SOURCES)
                default_rubrics = NEWS_RUBRICS if frame.Provider.NewsProviderValue.NewsSource == NEWS_DEFAULT_SOURCE else ['main']
                assert frame.Provider.NewsProviderValue.Rubric in _get_seq_attr(seq, 'news_rubric', default_rubrics)
                if state.NewsSuggest.Accepted:
                    assert state.NewsSuggest.Provider == frame.Provider.NewsProviderValue
        else:
            assert len(reset_add.Effects) == 1
    else:
        assert reset_add.Effects[0].ParsedUtterance.TypedSemanticFrame.HasField(frame_name)
    assert state.Stage.ShowIndex == show_index
    assert state.Stage.ShowPartIndex == part_index
    assert state.Stage.TrackIndex == track_index
    assert state.Stage.NewsIndex == news_index
    assert state.Stage.HardcodedShowIndex == hardcoded_show_index
    assert response_body.AnalyticsInfo.Actions[0].Id == action_id
    memento = _try_find_server_directive(response_body, 'MementoChangeUserObjectsDirective')
    if memento:
        assert memento.HasField('UserObjects')
    return r


def _assert_irrelevant(r):
    assert r.run_response.Features.IsIrrelevant
    return r


def _assert_relevant(r):
    assert not r.run_response.Features.IsIrrelevant
    return r


def _get_callback(r):
    callback_pb = None
    for action in r.run_response.ResponseBody.StackEngine.Actions:
        if action.ResetAdd.Effects and action.ResetAdd.Effects[0].Callback:
            callback_pb = action.ResetAdd.Effects[0].Callback
            break
    assert callback_pb, 'No callback found'
    return callback(callback_pb.Name, {
        '@scenario_name': SCENARIO_NAME  # TODO(vitvlkv) Avoid this hack, HOLLYWOOD-269
    })


def _id_param(id, *params):
    return pytest.param(*params, id=id)


def _test_normal_flow(alice, sequence):
    r = _assert_show_start(alice(voice('включи утреннее шоу')), TAliceShowState.EShowType.Morning, 'morning')
    state = _get_state(r.run_response)
    cb = _get_callback(r)
    for seq in sequence:
        if not CheckCase(seq, state):
            continue
        v = _get_seq_attr(seq, 'voice')
        r = _assert_show_action(alice(voice(v) if v else cb), _get_seq_attr(seq, 'action', 'alice_show.continue'), seq)
        state = _get_state(r.run_response)
        cb = _get_callback(r)
    _assert_irrelevant(alice(voice('дальше')))


def _test_navigate(alice, sequence):
    h = [_assert_show_start(alice(voice('включи утреннее шоу')), TAliceShowState.EShowType.Morning, 'morning')]
    state = _get_state(h[0].run_response)
    for seq in sequence:
        if not CheckCase(seq, state):
            continue
        v = _get_seq_attr(seq, 'voice', 'дальше')
        r = _assert_show_action(alice(voice(v)), _get_seq_attr(seq, 'action', 'alice_show.next'), seq)
        state = _get_state(r.run_response)
        if not _get_seq_attr(seq, 'no_content'):
            h.append(r)
    h.pop()

    while h:
        prev_r = h.pop()
        previous_response_body = prev_r.run_response.ResponseBody
        r = alice(voice('назад'))
        assert r.scenario_stages() == {'run'}
        response_body = r.run_response.ResponseBody
        assert response_body.Layout.OutputSpeech == previous_response_body.Layout.OutputSpeech
        state = _get_state(r.run_response)
        prev_state = _get_state(prev_r.run_response)
        assert str(state.Stage) == str(prev_state.Stage)
        assert str(state.StageHistory) == str(prev_state.StageHistory)
        assert str(state.PlanHistory) == str(prev_state.PlanHistory)
        assert response_body.AnalyticsInfo.Actions[0].Id == 'alice_show.prev'


@pytest.mark.scenario(name=SCENARIO_NAME, handle=SCENARIO_HANDLE)
@pytest.mark.parametrize('surface', [surface.station, surface.station_pro])
@pytest.mark.experiments(*DEFAULT_EXPERIMENTS)
class TestBaseNoAuth:
    pass


@pytest.mark.memento(MIXED_MEMENTO)
class TestBaseNoAuthMixedMemento(TestBaseNoAuth):
    pass


@pytest.mark.oauth(auth.YandexPlus)
class TestBase(TestBaseNoAuthMixedMemento):
    pass


class TestShowTypes(TestBase):
    @pytest.mark.parametrize('command', [
        _id_param('morning1', 'запусти утреннее шоу'),
        _id_param('morning2', 'давай шоу с алисой'),
        _id_param('morning3', 'доброе утро'),
    ])
    def test_morning(self, alice, command):
        _assert_show_start(alice(voice(command)), TAliceShowState.EShowType.Morning, 'morning')

    @pytest.mark.parametrize('command', [
        _id_param('evening1', 'запусти вечернее шоу'),
        _id_param('evening2', 'добрый вечер'),
    ])
    def test_evening(self, alice, command):
        _assert_show_start(alice(voice(command)), TAliceShowState.EShowType.Evening, 'evening')

    @pytest.mark.parametrize('command', [
        _id_param('night1', 'запусти ночное шоу'),
        _id_param('night2', 'ложусь спать'),
        _id_param('night3', 'спокойной ночи'),
    ])
    def test_good_night(self, alice, command):
        _assert_show_start_playlist(alice(voice(command)), TAliceShowState.EShowType.Night, 'night')

    @pytest.mark.parametrize('command', [
        _id_param('children1', 'запусти детское шоу'),
    ])
    def test_children(self, alice, command):
        _assert_show_start_children(alice(voice(command)))

    @pytest.mark.parametrize('command', [
        _id_param('children_night1', 'запусти ночное детское шоу'),
        _id_param('children_night2', 'запусти вечернее детское шоу'),
        _id_param('pillow1', 'запусти подушки шоу'),
    ])
    def test_children_night(self, alice, command):
        _assert_show_start_album(alice(voice(command)), TAliceShowState.EShowType.Night, 'children_night', 'children')

    @pytest.mark.parametrize('command', [
        _id_param('children_morning1', 'запусти утреннее детское шоу'),
    ])
    def test_children_morning(self, alice, command):
        r = _assert_show_start_children(alice(voice(command)))
        _assert_relevant(r)


def CheckCase(seq, state):
    f = _get_seq_attr(seq, 'case', lambda state: True)
    return f(state)


class TestNormalFlow(TestBase):
    def test_normal_flow(self, alice):
        _test_normal_flow(alice, SEQUENCE)


class TestNavigate(TestBase):
    def test_navigate(self, alice):
        _test_navigate(alice, SEQUENCE)


SEQ_CHOOSE_NEWS = (
    (CONTINUE_PART_CALLBACK,        None,                       2, 1, 0, 0, 1),
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  None,                       2, 0, 0, 0, 1, {'no_content': True}),
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  'GetTimeSemanticFrame',     3, 0, 0, 0, 1),
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  'WeatherSemanticFrame',     4, 0, 0, 0, 2),
    (CONTINUE_PART_CALLBACK,        'NewsSemanticFrame',        5, 1, 0, 1, 3, {'news_rubric': 'main'}),
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  None,                       5, 0, 0, 1, 3, {'no_content': True}),
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  None,                       6, 0, 0, 1, 3, {'no_content': True}),
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  'MusicPlaySemanticFrame',   7, 0, 1, 1, 4),
    (None,                          'HardcodedMorningShowSemanticFrame', 8, 0, 1, 1, 4),
)


# (callback_name, frame_name, show_index, part_index, track_index, news_index, hardcoded_show_index, {...})
SEQ_CHOOSE_NEWS_YES = (
    (CONTINUE_PART_CALLBACK,        None,                       2, 1, 0, 0, 1),
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  None,                       2, 0, 0, 0, 1, {'voice': 'хочу', 'action': 'alice_show.accept_suggested_news', 'no_content': True}),
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  'GetTimeSemanticFrame',     3, 0, 0, 0, 1),
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  'WeatherSemanticFrame',     4, 0, 0, 0, 2),
    (CONTINUE_PART_CALLBACK,        'NewsSemanticFrame',        5, 1, 0, 1, 3, {'news_rubric': 'main'}),
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  None,                       5, 0, 0, 1, 3, {'no_content': True}),
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  None,                       6, 0, 0, 1, 3, {'no_content': True}),  # convert
    (GET_NEXT_SHOW_BLOCK_CALLBACK,  'MusicPlaySemanticFrame',   7, 0, 1, 1, 4),  # joke
    (None,                          'HardcodedMorningShowSemanticFrame', 8, 0, 1, 1, 4),
)


@pytest.mark.experiments('hw_enable_alice_show_interactivity')
class TestNormalFlowWithChooseNews(TestNormalFlow):
    def test_normal_flow(self, alice):
        _test_normal_flow(alice, SEQ_CHOOSE_NEWS)

    def test_normal_flow_yes(self, alice):
        _test_normal_flow(alice, SEQ_CHOOSE_NEWS_YES)


@pytest.mark.experiments('hw_enable_alice_show_interactivity')
class TestNavigateWithChooseNews(TestNavigate):
    def test_navigate(self, alice):
        _test_navigate(alice, SEQ_CHOOSE_NEWS)

    def test_navigate_yes(self, alice):
        _test_navigate(alice, SEQ_CHOOSE_NEWS_YES)


EMOTION = 'negative'


def _assert_forced_emotion(reset_add):
    for effect in reset_add.Effects:
        assert effect.Options.ForcedEmotion == EMOTION


@pytest.mark.experiments('hw_alice_show_forced_emotion=' + EMOTION)
class TestForcedEmotion(TestBase):
    def test_forced_emotion(self, alice):
        r = _assert_show_start(alice(voice('включи утреннее шоу')), TAliceShowState.EShowType.Morning, 'morning')
        _assert_forced_emotion(r.run_response.ResponseBody.StackEngine.Actions[1].ResetAdd)
        r = alice(_get_callback(r))
        _assert_forced_emotion(r.run_response.ResponseBody.StackEngine.Actions[0].ResetAdd)


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.experiments('hw_disable_alice_show_without_music')
class TestNoPlus(TestBaseNoAuthMixedMemento):
    def test_reject(self, alice):
        r = alice(voice('включи утреннее шоу'))
        assert r.scenario_stages() == {'run'}
        response_body = r.run_response.ResponseBody
        assert 'подписк' in response_body.Layout.OutputSpeech
        assert not r.run_response.ResponseBody.HasField('StackEngine')


@pytest.mark.oauth(auth.Yandex)
class TestShowWithoutMusic(TestBaseNoAuthMixedMemento):
    SEQ_WITHOUT_MUSIC = (
        (GET_NEXT_SHOW_BLOCK_CALLBACK,  'GetTimeSemanticFrame',     2, 0, 0, 0, 1),
        (GET_NEXT_SHOW_BLOCK_CALLBACK,  'WeatherSemanticFrame',     3, 0, 0, 0, 2),
        (CONTINUE_PART_CALLBACK,        'NewsSemanticFrame',        4, 1, 0, 1, 2, {'news_source': '35376ef1-ria-novosti'}),
        (CONTINUE_PART_CALLBACK,        'NewsSemanticFrame',        4, 2, 0, 2, 2, {'news_source': '35376ef1-ria-novosti'}),
        (GET_NEXT_SHOW_BLOCK_CALLBACK,  'NewsSemanticFrame',        4, 0, 0, 3, 2, {'news_source': '35376ef1-ria-novosti'}),
        (GET_NEXT_SHOW_BLOCK_CALLBACK,  None,     5, 0, 0, 3, 2, {'no_content': True}),
        (None,                          'RadioPlaySemanticFrame',   6, 0, 0, 3, 2),
    )

    def test_normal_flow(self, alice):
        _test_normal_flow(alice, self.SEQ_WITHOUT_MUSIC)

    def test_navigate(self, alice):
        _test_navigate(alice, self.SEQ_WITHOUT_MUSIC)


@pytest.mark.oauth(auth.YandexPlus)
class TestShowWithoutNews(TestBaseNoAuth):
    SEQ_WITHOUT_NEWS = (
        (GET_NEXT_SHOW_BLOCK_CALLBACK,  'GetTimeSemanticFrame',     2, 0, 0, 0, 1),
        (GET_NEXT_SHOW_BLOCK_CALLBACK,  'WeatherSemanticFrame',     3, 0, 0, 0, 2),
        (None,                          'HardcodedMorningShowSemanticFrame', 4, 0, 0, 0, 2),
    )

    @pytest.mark.memento(DEFAULT_MEMENTO)
    def test_normal_flow_default_memento(self, alice):
        _test_normal_flow(alice, self.SEQ_WITHOUT_NEWS)

    @pytest.mark.memento(DEFAULT_MEMENTO)
    def test_navigate_default_memento(self, alice):
        _test_navigate(alice, self.SEQ_WITHOUT_NEWS)

    @pytest.mark.memento(RUBRIC_MEMENTO)
    def test_normal_flow_rubric_memento(self, alice):
        _test_normal_flow(alice, self.SEQ_WITHOUT_NEWS)

    @pytest.mark.memento(RUBRIC_MEMENTO)
    def test_navigate_rubric_memento(self, alice):
        _test_navigate(alice, self.SEQ_WITHOUT_NEWS)

    @pytest.mark.memento(MIXED_MEMENTO)
    @pytest.mark.experiments('hw_disable_news')
    def test_normal_flow_disabled_news(self, alice):
        _test_normal_flow(alice, self.SEQ_WITHOUT_NEWS)

    @pytest.mark.memento(MIXED_MEMENTO)
    @pytest.mark.experiments('hw_disable_news')
    def test_navigate_disabled_news(self, alice):
        _test_navigate(alice, self.SEQ_WITHOUT_NEWS)


def _get_user_config_from_memento_directive(r, key, type):
    memento = _find_server_directive(r.run_response.ResponseBody, 'MementoChangeUserObjectsDirective')
    for c in memento.UserObjects.UserConfigs:
        if c.Key == key:
            v = type()
            c.Value.Unpack(v)
            return v
    return None


def _get_reset_frame(r, type):
    for a in r.run_response.ResponseBody.StackEngine.Actions:
        if a.HasField('ResetAdd'):
            reset_add = a.ResetAdd
            assert len(reset_add.Effects) > 0
            frame = reset_add.Effects[0].ParsedUtterance.TypedSemanticFrame
            assert frame.HasField(type)
            return getattr(frame, type)
    assert False, type + ' not found'


class TestSlots(TestBase):
    NEWS_SOURCE = "86db974e-biznes-fm"
    RUBRIC = "main"
    PODCAST = "pop_science"
    SEQUENCE_RADIO_NEWS = (
        (GET_NEXT_SHOW_BLOCK_CALLBACK,  None,                       2, 0, 0, 0, 1, {'no_content': True}),
        (GET_NEXT_SHOW_BLOCK_CALLBACK,  'GetTimeSemanticFrame',     3, 0, 0, 0, 1),
        (GET_NEXT_SHOW_BLOCK_CALLBACK,  'WeatherSemanticFrame',     4, 0, 0, 0, 2),
        (CONTINUE_PART_CALLBACK,        'NewsSemanticFrame',        5, 1, 0, 1, 3, {'news_source': NEWS_SOURCE, 'news_rubric': RUBRIC}),
        (GET_NEXT_SHOW_BLOCK_CALLBACK,  None,                       5, 0, 0, 1, 3, {'no_content': True}),
        (GET_NEXT_SHOW_BLOCK_CALLBACK,  None,                       6, 0, 0, 1, 3, {'no_content': True}),  # convert
        (GET_NEXT_SHOW_BLOCK_CALLBACK,  'MusicPlaySemanticFrame',   7, 0, 1, 1, 4),  # joke
        (None,                          'HardcodedMorningShowSemanticFrame', 8, 0, 1, 1, 4),
    )

    def test_slots(self, alice):
        payload = {
            'typed_semantic_frame': {
                'alice_show_activate_semantic_frame': {
                    'news_provider': {
                        'news_provider_value': {
                            "news_source": self.NEWS_SOURCE,
                            "rubric": self.RUBRIC
                        }
                    },
                    'topic': {
                        'topic_value': {
                            "podcast": self.PODCAST
                        }
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'alice_show'
            }
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        for seq in self.SEQUENCE_RADIO_NEWS:
            r = _assert_show_action(alice(_get_callback(r)), 'alice_show.continue', seq)
        frame = _get_reset_frame(r, 'HardcodedMorningShowSemanticFrame')
        assert frame.NewsProvider.SerializedData == f'{{"news_source":"{self.NEWS_SOURCE}","rubric":"{self.RUBRIC}"}}'
        assert frame.Topic.SerializedData == f'{{"podcast":"{self.PODCAST}"}}'
