import itertools
import logging
import re

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import callback, voice, server_action, Biometry
from conftest import get_scenario_state


logger = logging.getLogger(__name__)

SCENARIO_NAME = 'HollywoodMusic'

EXPERIMENTS = [
    'bg_fresh_granet',
    'bg_alice_music_like',
    'bg_alice_music_onboarding',
    'bg_alice_music_onboarding_artists',
    'bg_alice_music_onboarding_genres',
    'bg_alice_music_onboarding_tracks',
    'hw_music_onboarding',
    'hw_music_complex_like',
    'hw_music_thin_client',
]

TRACKS_GAME_LIKE_PHRASES = [
    'Ура! Я старалась! Послушайте теперь этот.',
    'Отлично! Послушайте ещё вот этот.',
    'Очень рада! А как насчет этого?',
]

TRACKS_GAME_DISLIKE_PHRASES = re.compile(
    r'(Поняла\.|Запомнила!|Очень жаль\.) '
    r'(Послушайте теперь этот\.|Попробуйте теперь вот этот\.|А как насчет этого\?)'
)

TRACKS_GAME_DISLIKE_PROPOSAL_PHRASES = re.compile(
    r'(Поняла\.|Запомнила!|Очень жаль\.) '
    r'(Кажется, наши вкусы сегодня не совпадают\. Что вы хотите послушать'
    r'|Похоже, вам не нравится то, что я ставлю\. Что прикажете'
    r'|Что вам включить, чтобы вам понравилось'
    r'|Что вам включить, чтобы вас порадовать'
    r'|А давайте послушаем то, что вам нравится\. Что вам включить)\?'
)

TRACKS_MIDROLL_DIRECTIVE_ID_REG = re.compile(r'music_onboarding_midroll_\d+_\d+_[\d\w]+')

ADD_SCHEDULE_ACTION_DIRECTIVE_NAME = 'AddScheduleActionDirective'


def _get_callback(r):
    callback_pb = None
    run_response_body = r.run_response.ResponseBody
    commit_candidate_body = r.run_response.CommitCandidate.ResponseBody
    for action in itertools.chain(
        run_response_body.StackEngine.Actions,
        commit_candidate_body.StackEngine.Actions,
    ):
        if action.ResetAdd.Effects and action.ResetAdd.Effects[0].Callback:
            callback_pb = action.ResetAdd.Effects[0].Callback
            break
    assert callback_pb, 'No callback found'
    return callback(callback_pb.Name, {
        '@scenario_name': SCENARIO_NAME  # TODO(vitvlkv) Avoid this hack, HOLLYWOOD-269
    })


def _assert_irrelevant_response(r):
    assert r.run_response.Features.IsIrrelevant


def _try_find_server_directive(response_body, directive_type):
    for d in response_body.ServerDirectives:
        if d.HasField(directive_type):
            return getattr(d, directive_type)
    return None


def _check_tracks_midroll_directive(response, is_first_midroll):
    tracks_midroll_directive = _try_find_server_directive(response.ResponseBody, ADD_SCHEDULE_ACTION_DIRECTIVE_NAME)
    assert tracks_midroll_directive

    schedule_action = tracks_midroll_directive.ScheduleAction
    assert schedule_action
    assert TRACKS_MIDROLL_DIRECTIVE_ID_REG.match(schedule_action.Id)
    assert schedule_action.Puid
    assert schedule_action.DeviceId

    state = get_scenario_state(response)
    history = state.Queue.History
    assert len(history) > 0

    start_at_timestamp_ms = schedule_action.StartPolicy.StartAtTimestampMs
    current_timestamp_ms = history[-1].UrlInfo.UrlTime
    actual_midroll_delay = round((start_at_timestamp_ms - current_timestamp_ms) / 1000)

    if is_first_midroll:
        current_track_duration_ms = history[-1].DurationMs
        expected_midroll_delay = round(current_track_duration_ms / 1000 * 0.3)
        expected_midroll_delay = max(min(expected_midroll_delay, 90), 30)
    else:
        expected_midroll_delay = 90
        # Timestamps in response behave awkwardly after processing the first midroll
        actual_midroll_delay -= 2

    assert expected_midroll_delay == actual_midroll_delay


def make_tracks_reask_action(track_index: int):
    track_reask_payload = {
        'typed_semantic_frame': {
            'music_onboarding_tracks_reask_semantic_frame': {
                'track_index': {
                    'num_value': track_index
                }
            }
        },
        'analytics': {
            'origin': 'Scenario',
            'purpose': 'music_onboarding'
        }
    }
    return server_action(name='@@mm_semantic_frame', payload=track_reask_payload)


@pytest.mark.scenario(name=SCENARIO_NAME, handle='music')
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments(*EXPERIMENTS)
class _TestsOnboardingBase:
    def _test(self, r, stages=None, in_onboarding=False, in_master_onboarding=False, directives_count=0, scenario='music',
              intent='alice.music.complex_like_dislike', speech='', play=False, pause=False, different_track=True, last_track=None,
              radio='user:onyourwave', check_no_unexpected_start_callback=False, readable_analytics='', expects_request=None):
        if stages is None:
            stages = r.scenario_stages()
        else:
            assert r.scenario_stages() == stages
        if 'commit' in stages:
            rr = r.run_response.CommitCandidate
        elif 'apply' in stages:
            rr = r.apply_response
        elif 'continue' in stages:
            rr = r.continue_response
        else:
            rr = r.run_response
        response = rr.ResponseBody
        state = get_scenario_state(rr)

        layout = response.Layout
        if isinstance(speech, re.Pattern):
            assert speech.fullmatch(layout.OutputSpeech)
        elif isinstance(speech, list):
            assert layout.OutputSpeech in speech
        elif speech != '':
            assert layout.OutputSpeech == speech
        assert directives_count is None or len(layout.Directives) == directives_count
        if play:
            assert directives_count > 0
            audio_play = None
            if pause:
                assert layout.Directives[0].HasField('TtsPlayPlaceholderDirective')
                assert layout.Directives[1].HasField('ListenDirective')
                assert layout.Directives[2].HasField('AudioPlayDirective')
                audio_play = layout.Directives[2].AudioPlayDirective
                assert audio_play.SetPause
            else:
                assert layout.Directives[0].HasField('AudioPlayDirective')
                audio_play = layout.Directives[0].AudioPlayDirective
                assert not audio_play.SetPause
            assert audio_play.Name == 'music'
            assert audio_play.Stream.Id
            if last_track is not None:
                if different_track and len(last_track):
                    assert audio_play.Stream.Id != last_track[0]
                last_track.clear()
                last_track.append(audio_play.Stream.Id)
            if radio:
                assert audio_play.HasField('AudioPlayMetadata')
                assert audio_play.AudioPlayMetadata.GlagolMetadata.MusicMetadata.Id == radio
            if check_no_unexpected_start_callback:
                assert audio_play.Callbacks.HasField('OnPlayStartedCallback')
                on_started = r.apply_response_pyobj['response']['layout']['directives'][2 if pause else 0]['audio_play_directive']['callbacks']['on_started']
                for event in on_started['payload']['events']:
                    assert 'radioFeedbackEvent' not in event or event['radioFeedbackEvent']['type'] not in ['Skip', 'Dislike']

        if in_onboarding:
            assert state.OnboardingState.InOnboarding
            if in_master_onboarding:
                assert state.OnboardingState.InMasterOnboarding
        else:
            assert not state.HasField('OnboardingState')
        assert response.ExpectsRequest == (in_onboarding if expects_request is None else expects_request)

        analytics = response.AnalyticsInfo
        assert intent is None or analytics.Intent == intent
        assert scenario is None or analytics.ProductScenarioName == scenario
        if readable_analytics:
            assert analytics.Actions[0].HumanReadable == readable_analytics

    def _test_not_onboarding(self, r):
        self._test(r, in_onboarding=False, scenario=None, intent=None, directives_count=None)


class TestsOnboardingArtists(_TestsOnboardingBase):
    def _test_guess_artist(self, alice):
        self._test(alice(voice('угадай исполнителей которые мне нравятся')), {'run'}, in_onboarding=True,
                   directives_count=2,
                   intent='alice.music_onboarding.artists',
                   speech='Кто ваш любимый исполнитель?')

    @pytest.mark.oauth(auth.Yandex)
    @pytest.mark.parametrize('command', [
        pytest.param('угадай исполнителей которые мне нравятся', id='guess'),
    ])
    def test_no_subscription(self, alice, command):
        self._test(alice(voice(command)), {'run'},
                   intent='alice.music_onboarding.artists',
                   speech='Чтобы настраивать рекомендации музыки, вам нужна подписка на Музыку.')

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('subcommand, output_speech, reach_commit, reask', [
        pytest.param('Radiohead', 'Radiohead, отличный выбор!', True, False, id='artist'),
        pytest.param('мне нравится Radiohead', 'Radiohead, отличный выбор!', True, False, id='like_artist'),
        pytest.param('мне не нравится Radiohead', 'Хорошо, поставила дизлайк. А какой исполнитель вам нравится?', True, True, id='dislike_artist'),
        pytest.param('песня Gold Guns Girls', 'Metric, отличный выбор!', True, False, id='artist_from_song'),
        pytest.param('мне нравится песня Gold Guns Girls', 'Metric, отличный выбор!', True, False, id='like_artist_from_song'),
        pytest.param('рок', 'Извините, я не смогла разобрать исполнителя. Можете повторить?', False, True, id='genre'),
        pytest.param('мне нравится рок', 'Извините, я не смогла разобрать исполнителя. Можете повторить?', False, True, id='like_genre'),
    ])
    def test_normal(self, alice, subcommand, output_speech, reach_commit, reask):
        self._test_guess_artist(alice)

        self._test(alice(voice(subcommand)), {'run', 'commit'} if reach_commit else {'run'}, in_onboarding=reask,
                   directives_count=2 if reask else 0,
                   speech=output_speech)

        if reask:
            self._test(alice(voice('Radiohead')), {'run', 'commit'},
                       speech='Radiohead, отличный выбор!')

        self._test_not_onboarding(alice(voice('Radiohead')))

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.experiments('hw_music_onboarding_no_artist_from_track')
    def test_no_artist_from_track(self, alice):
        self._test_guess_artist(alice)

        self._test(alice(voice('песня Gold Guns Girls')), {'run'}, in_onboarding=True,
                   directives_count=2,
                   speech='Извините, я не смогла найти такого исполнителя. Кто ещё вам нравится?')

        # We only reask once
        self._test(alice(voice('песня Gold Guns Girls')), {'run'},
                   speech='Извините, я не смогла найти такого исполнителя.')

        self._test_not_onboarding(alice(voice('песня Gold Guns Girls')))


class TestsOnboardingGenres(_TestsOnboardingBase):
    @pytest.mark.oauth(auth.Yandex)
    @pytest.mark.parametrize('command', [
        pytest.param('угадай жанры которые мне нравятся', id='guess'),
        pytest.param('запомни мои любимые жанры', id='remember'),
    ])
    def test_no_subscription(self, alice, command):
        self._test(alice(voice(command)), {'run'},
                   intent='alice.music_onboarding.genres',
                   speech='Чтобы настраивать рекомендации музыки, вам нужна подписка на Музыку.')

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('subcommand, output_speech, reask', [
        pytest.param('рок', 'Рок, мне нравится!', False, id='genre'),
        pytest.param('мне нравится рок', 'Рок, мне нравится!', False, id='like_genre'),
        pytest.param('Radiohead', 'Не могу найти такой жанр. Можете повторить?', True, id='artist'),
        pytest.param('мне нравится Radiohead', 'Не могу найти такой жанр. Можете повторить?', True, id='like_artist'),
    ])
    def test_normal(self, alice, subcommand, output_speech, reask):
        self._test(alice(voice('угадай жанры которые мне нравятся')), {'run'}, in_onboarding=True,
                   directives_count=2,
                   intent='alice.music_onboarding.genres',
                   speech='Музыку какого жанра вы слушаете чаще всего?')

        self._test(alice(voice(subcommand)), {'run', 'commit'} if not reask else {'run'}, in_onboarding=reask,
                   directives_count=2 if reask else 0,
                   speech=output_speech)

        if reask:
            self._test(alice(voice('рок')), {'run', 'commit'}, speech='Рок, мне нравится!')

        self._test_not_onboarding(alice(voice('рок')))


class TestsOnboardingTracks(_TestsOnboardingBase):
    @pytest.mark.oauth(auth.Yandex)
    @pytest.mark.parametrize('command', [
        pytest.param('угадай песни которые мне нравятся', id='guess'),
        # TODO(jan-fazli): Make it work someday
        # pytest.param('запомни мои любимые песни', id='remember'),
    ])
    def test_no_subscription(self, alice, command):
        self._test(alice(voice(command)), {'run'},
                   intent='alice.music_onboarding.tracks',
                   speech='Чтобы настраивать рекомендации музыки, вам нужна подписка на Музыку.')

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.experiments('hw_music_onboarding_tracks_count=4')
    def test_normal(self, alice):
        last_track = []  # An array cause we want it to be mutable

        self._test(alice(voice('угадай песни которые мне нравятся')), {'run', 'continue'},
                   in_onboarding=True, directives_count=1, play=True, last_track=last_track,
                   intent='alice.music_onboarding.tracks',
                   speech='Хорошо! Включаю первый трек, а вы говорите, нравится или нет.')

        self._test(alice(voice('лайк')), {'run', 'apply'},
                   in_onboarding=True, directives_count=1, play=True, last_track=last_track,
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_like', scenario='player_commands',
                   speech=TRACKS_GAME_LIKE_PHRASES)

        self._test(alice(voice('дизлайк')), {'run', 'apply'},
                   in_onboarding=True, directives_count=1, play=True, last_track=last_track,
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_dislike', scenario='player_commands',
                   speech=TRACKS_GAME_DISLIKE_PHRASES)

        self._test(alice(voice('лайк')), {'run', 'apply'},
                   in_onboarding=True, directives_count=1, play=True, last_track=last_track,
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_like', scenario='player_commands',
                   speech='Хорошо, попробуем последний.')

        self._test(alice(voice('лайк')), {'run', 'apply'},
                   directives_count=3, play=True, pause=True, last_track=last_track,
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_like', scenario='player_commands',
                   speech='Окей, запомнила. Теперь когда вы будете просить музыку я буду основываться на ваших предпочтениях. Давайте послушаем?')

        self._test(alice(voice('нет')), {'run'},
                   intent='alice.music_onboarding.tracks',
                   speech='Хорошо. Вы можете в любое время сказать мне "Алиса, включи музыку".')

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.experiments('hw_music_onboarding_tracks_count=1')
    def test_normal_with_play_after(self, alice):
        self._test(alice(voice('угадай песни которые мне нравятся')), {'run', 'continue'},
                   in_onboarding=True, directives_count=1, play=True,
                   intent='alice.music_onboarding.tracks',
                   speech='Хорошо! Включаю первый трек, а вы говорите, нравится или нет.')

        self._test(alice(voice('лайк')), {'run', 'apply'},
                   directives_count=3, play=True, pause=True,
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_like', scenario='player_commands',
                   speech='Окей, запомнила. Теперь когда вы будете просить музыку я буду основываться на ваших предпочтениях. Давайте послушаем?')

        self._test(alice(voice('давай')), {'run', 'continue'}, directives_count=1, play=True,
                   intent='personal_assistant.scenarios.player_continue', scenario='player_commands',
                   speech='')

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.experiments('hw_music_onboarding_tracks_reask', 'hw_music_onboarding_tracks_count=2')
    def test_reask(self, alice):
        last_track = []  # An array cause we want it to be mutable

        r = alice(voice('угадай песни которые мне нравятся'))
        self._test(r, {'run', 'continue'},
                   in_onboarding=True, directives_count=1, play=True, last_track=last_track,
                   intent='alice.music_onboarding.tracks',
                   speech='Хорошо! Включаю первый трек, а вы говорите, нравится или нет.')
        _check_tracks_midroll_directive(r.continue_response, True)

        for i in range(2):
            r = alice(make_tracks_reask_action(0))
            assert r.scenario_stages() == {'run'}

            response_body = r.run_response.ResponseBody
            assert response_body.Layout.OutputSpeech == 'Мне только спросить. Вам нравится эта песня? Скажите лайк.'

            if i == 0:
                _check_tracks_midroll_directive(r.run_response, False)
            else:
                assert not _try_find_server_directive(response_body, ADD_SCHEDULE_ACTION_DIRECTIVE_NAME)

        r = alice(voice('лайк'))
        self._test(r, {'run', 'apply'},
                   in_onboarding=True, directives_count=1, play=True, last_track=last_track,
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_like', scenario='player_commands',
                   speech='Хорошо, попробуем последний.')
        _check_tracks_midroll_directive(r.apply_response, True)

        for i in range(2):
            r = alice(make_tracks_reask_action(1))
            assert r.scenario_stages() == {'run'}

            response_body = r.run_response.ResponseBody
            assert response_body.Layout.OutputSpeech == 'Мне только спросить. Вам нравится эта песня? Скажите лайк.'

            if i == 0:
                _check_tracks_midroll_directive(r.run_response, False)
            else:
                assert not _try_find_server_directive(response_body, ADD_SCHEDULE_ACTION_DIRECTIVE_NAME)

        r = alice(voice('лайк'))
        self._test(r, {'run', 'apply'},
                   directives_count=3, play=True, pause=True, last_track=last_track,
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_like', scenario='player_commands',
                   speech='Окей, запомнила. Теперь когда вы будете просить музыку я буду основываться на ваших предпочтениях. Давайте послушаем?')
        assert not _try_find_server_directive(r.apply_response.ResponseBody, ADD_SCHEDULE_ACTION_DIRECTIVE_NAME)

        r = alice(make_tracks_reask_action(2))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech == ''
        assert not _try_find_server_directive(r.run_response.ResponseBody, ADD_SCHEDULE_ACTION_DIRECTIVE_NAME)

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.experiments('hw_music_onboarding_tracks_reask')
    def test_ignore_invalid_midroll(self, alice):
        r = alice(voice('угадай песни которые мне нравятся'))
        r = alice(make_tracks_reask_action(1))
        _assert_irrelevant_response(r)
        assert not _try_find_server_directive(r.run_response.ResponseBody, ADD_SCHEDULE_ACTION_DIRECTIVE_NAME)
        assert r.run_response.ResponseBody.HasField('State')

        r = alice(voice('лайк'))
        r = alice(make_tracks_reask_action(0))
        _assert_irrelevant_response(r)
        assert not _try_find_server_directive(r.run_response.ResponseBody, ADD_SCHEDULE_ACTION_DIRECTIVE_NAME)
        assert r.run_response.ResponseBody.HasField('State')

    @pytest.mark.oauth(auth.YandexPlus)
    def test_unknown_user(self, alice):
        incognito_biometry = Biometry(is_known=False, known_user_id='1083955728')

        r = alice(voice('угадай песни которые мне нравятся', biometry=incognito_biometry))
        self._test(r, {'run', 'continue'},
                   in_onboarding=True, directives_count=1, play=True,
                   intent='alice.music_onboarding.tracks',
                   speech='Хорошо! Включаю первый трек, а вы говорите, нравится или нет.')

        r = alice(voice('лайк', biometry=incognito_biometry))
        self._test(r, {'run', 'apply'},
                   in_onboarding=True, directives_count=1, play=True,
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_like', scenario='player_commands',
                   speech=TRACKS_GAME_LIKE_PHRASES)


class TestsOutOfOnboardingLikeDislike(_TestsOnboardingBase):
    @pytest.mark.oauth(auth.Yandex)
    def test_no_subscription(self, alice):
        self._test(alice(voice('мне нравится Radiohead')), {'run'},
                   speech='Чтобы ставить лайки и дизлайки музыки, вам нужна подписка на Музыку.')

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('command, output_speech, analytics', [
        pytest.param('мне нравится Radiohead', 'Radiohead, отличный выбор!', 'Ставится лайк исполнителю Radiohead', id='like_artist'),
        pytest.param('мне не нравится Radiohead', 'Хорошо, поставила дизлайк исполнителю.', 'Ставится дизлайк исполнителю Radiohead', id='dislike_artist'),
        pytest.param('мне нравится песня Gold Guns Girls', 'Хорошо, поставила лайк треку!', 'Ставится лайк треку Gold Guns Girls', id='like_song'),
        pytest.param('мне не нравится песня Gold Guns Girls', 'Хорошо, поставила дизлайк треку.', 'Ставится дизлайк треку Gold Guns Girls', id='dislike_song'),
        pytest.param('мне нравится альбом OK Computer', 'Хорошо, поставила лайк альбому!', 'Ставится лайк альбому OK Computer', id='like_album'),
        pytest.param('мне нравится рок', 'Рок, мне нравится!', 'Ставится лайк жанру rock', id='like_genre'),
    ])
    def test_normal(self, alice, command, output_speech, analytics):
        self._test(alice(voice(command)), {'run', 'commit'},
                   speech=output_speech, readable_analytics=analytics)

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('command', [
        pytest.param('мне не нравится альбом OK Computer', id='dislike_album'),
        pytest.param('мне не нравится рок', id='dislike_genre'),
    ])
    def test_unsupported(self, alice, command):
        self._test(alice(voice(command)), {'run'},
                   speech='Извините, я пока умею ставить дизлайки только песням и исполнителям.')

    # Should just go to music
    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('command', [
        pytest.param('Radiohead', id='artist'),
        pytest.param('песня Gold Guns Girls', id='song'),
        pytest.param('рок', id='genre'),
    ])
    def test_no_like_dislike(self, alice, command):
        self._test_not_onboarding(alice(voice(command)))


class TestsMasterOnboarding(_TestsOnboardingBase):
    @pytest.mark.oauth(auth.Yandex)
    def test_no_subscription(self, alice):
        self._test(alice(voice('настрой рекомендацию песен алиса')), {'run'},
                   intent='alice.music_onboarding',
                   speech='Чтобы настраивать рекомендации музыки, вам нужна подписка на Музыку.')

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('command, output_speech', [
        pytest.param('мне нравится Radiohead',
                     'Radiohead, отличный выбор! А музыку какого жанра вы слушаете чаще всего?',
                     id='like_artist'),
        pytest.param('мне не нравится Radiohead',
                     'Хорошо, поставила дизлайк исполнителю. А музыку какого жанра вы слушаете чаще всего?',
                     id='dislike_artist'),
    ])
    def test_normal(self, alice, command, output_speech):
        self._test(alice(voice('настрой музыкальные рекомендации')), {'run'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=2,
                   intent='alice.music_onboarding',
                   speech='Для начала давайте узнаем друг друга лучше. Кто ваш любимый исполнитель?')

        self._test(alice(voice('джаз')), {'run'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=2,
                   speech='Извините, я не смогла разобрать исполнителя. Можете повторить?')

        self._test(alice(voice(command)), {'run', 'commit'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=2,
                   speech=output_speech)

        self._test(alice(voice('Opeth')), {'run'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=2,
                   speech='Не могу найти такой жанр. Можете повторить?')

        self._test(alice(voice('рок')), {'run', 'commit'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=2,
                   speech='Рок, мне нравится! Давайте поиграем? Я вам включу несколько песен, а вы скажете, нравятся они вам или нет.')

        self._test(alice(voice('нет')), {'run'},
                   intent='alice.music_onboarding.tracks',
                   speech='Хорошо.')

    @pytest.mark.oauth(auth.YandexPlus)
    def test_dont_know(self, alice):
        self._test(alice(voice('настрой музыкальные рекомендации')), {'run'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=2,
                   intent='alice.music_onboarding',
                   speech='Для начала давайте узнаем друг друга лучше. Кто ваш любимый исполнитель?')

        self._test(alice(voice('не знаю')), {'run'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=2,
                   intent='alice.music_onboarding.dont_know',
                   speech='Давайте попробуем еще раз: кто ваш любимый исполнитель?')

        self._test(alice(voice('мне нравится Radiohead')), {'run', 'commit'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=2,
                   speech='Radiohead, отличный выбор! А музыку какого жанра вы слушаете чаще всего?')

        self._test(alice(voice('не знаю')), {'run'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=2,
                   intent='alice.music_onboarding.dont_know',
                   speech='Давайте попробуем еще раз: музыку какого жанра вы слушаете чаще всего?')

        self._test(alice(voice('не знаю')), {'run'},
                   intent='alice.music_onboarding.dont_know',
                   speech='Хорошо, когда определитесь, скажите: Алиса, настрой мои музыкальные предпочтения.')

        self._test_not_onboarding(alice(voice('джаз')))

    @pytest.mark.oauth(auth.YandexPlus)
    def test_silence(self, alice):
        r = alice(voice('настрой музыкальные рекомендации'))
        self._test(r, {'run'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=2,
                   intent='alice.music_onboarding',
                   speech='Для начала давайте узнаем друг друга лучше. Кто ваш любимый исполнитель?')

        r = alice(_get_callback(r))
        self._test(r, {'run'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=2,
                   intent='alice.music_onboarding.silence',
                   speech='Давайте попробуем еще раз: кто ваш любимый исполнитель?')

        r = alice(voice('мне нравится Radiohead'))
        self._test(r, {'run', 'commit'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=2,
                   speech='Radiohead, отличный выбор! А музыку какого жанра вы слушаете чаще всего?')

        r = alice(_get_callback(r))
        self._test(r, {'run'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=2,
                   intent='alice.music_onboarding.silence',
                   speech='Давайте попробуем еще раз: музыку какого жанра вы слушаете чаще всего?')

        r = alice(_get_callback(r))
        self._test(r, {'run'},
                   intent='alice.music_onboarding.silence',
                   speech='Хорошо, когда определитесь, скажите: Алиса, настрой мои музыкальные предпочтения.')

        r = alice(voice('джаз'))
        self._test_not_onboarding(r)

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.experiments(
        'hw_music_onboarding_genre_radio',
        'bg_fresh_granet_form=alice.proactivity.force_exit',
    )
    @pytest.mark.parametrize('session_len', list(range(1, 7, 1)) + [None])
    def test_force_exit(self, alice, session_len):
        self._test(alice(voice('настрой музыкальные рекомендации')), {'run'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=2,
                   intent='alice.music_onboarding',
                   speech='Для начала давайте узнаем друг друга лучше. Кто ваш любимый исполнитель?')

        force_exit_command = 'погода'
        next_command = 'мне нравится Radiohead'
        if session_len == 1:
            _assert_irrelevant_response(alice(voice(force_exit_command)))
            self._test_not_onboarding(alice(voice(next_command)))
            return

        self._test(alice(voice(next_command)), {'run', 'commit'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=2,
                   speech='Radiohead, отличный выбор! А музыку какого жанра вы слушаете чаще всего?')

        next_command = 'рок'
        if session_len == 2:
            _assert_irrelevant_response(alice(voice(force_exit_command)))
            self._test_not_onboarding(alice(voice(next_command)))
            return

        self._test(alice(voice(next_command)), {'run', 'commit'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=2,
                   speech='Рок, мне нравится! Давайте поиграем? Я вам включу несколько песен, а вы скажете, нравятся они вам или нет.')

        next_command = 'давай'
        if session_len == 3:
            _assert_irrelevant_response(alice(voice(force_exit_command)))
            self._test_not_onboarding(alice(voice(next_command)))
            return

        last_track = []  # An array cause we want it to be mutable

        self._test(alice(voice(next_command)), {'run', 'continue'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=1, play=True, last_track=last_track, radio='genre:rock',
                   intent='alice.music_onboarding.tracks',
                   speech='Хорошо! Включаю первый трек.')

        next_command = 'лайк'
        if session_len == 4:
            _assert_irrelevant_response(alice(voice(force_exit_command)))
            self._test_not_onboarding(alice(voice(next_command)))
            return

        self._test(alice(voice(next_command)), {'run', 'apply'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=1, play=True, last_track=last_track, radio='genre:rock',
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_like', scenario='player_commands',
                   speech=TRACKS_GAME_LIKE_PHRASES)

        next_command = 'лайк'
        if session_len == 5:
            _assert_irrelevant_response(alice(voice(force_exit_command)))
            self._test_not_onboarding(alice(voice(next_command)))
            return

        self._test(alice(voice(next_command)), {'run', 'apply'}, in_onboarding=True, in_master_onboarding=True,
                   directives_count=1, play=True, last_track=last_track, radio='genre:rock',
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_like', scenario='player_commands',
                   speech='Хорошо, попробуем последний.')

        next_command = 'лайк'
        if session_len == 6:
            _assert_irrelevant_response(alice(voice(force_exit_command)))
            self._test_not_onboarding(alice(voice(next_command)))
            return

        self._test(alice(voice(next_command)), {'run', 'apply'},
                   directives_count=3, play=True, pause=True, last_track=last_track, radio='genre:rock',
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_like', scenario='player_commands',
                   speech='Окей, запомнила. Теперь когда вы будете просить музыку я буду основываться на ваших предпочтениях. Давайте послушаем?')

        self._test(alice(voice('давай')), {'run', 'continue'}, directives_count=1, play=True, radio='genre:rock',
                   intent='personal_assistant.scenarios.player_continue', scenario='player_commands',
                   speech='')

        self._test_not_onboarding(alice(voice('джаз')))


class TestsOnboardingTracksRepeatedSkip(_TestsOnboardingBase):
    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.experiments(
        'hw_music_onboarding_tracks_count=3',
        'hw_music_onboarding_repeated_skip_threshold=3',
    )
    def test_normal(self, alice):
        last_track = []  # An array cause we want it to be mutable

        self._test(alice(voice('угадай песни которые мне нравятся')), {'run', 'continue'},
                   in_onboarding=True, directives_count=1, play=True, last_track=last_track,
                   intent='alice.music_onboarding.tracks',
                   speech='Хорошо! Включаю первый трек, а вы говорите, нравится или нет.')

        self._test(alice(voice('дизлайк')), {'run', 'apply'},
                   in_onboarding=True, directives_count=1, play=True, last_track=last_track,
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_dislike', scenario='player_commands',
                   speech=TRACKS_GAME_DISLIKE_PHRASES)

        self._test(alice(voice('дизлайк')), {'run', 'apply'},
                   in_onboarding=True, directives_count=1, play=True, last_track=last_track,
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_dislike', scenario='player_commands',
                   speech='Хорошо, попробуем последний.')

        self._test(alice(voice('дизлайк')), {'run', 'apply'},
                   in_onboarding=True, directives_count=3, play=False, last_track=last_track,
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_dislike', scenario='player_commands',
                   speech=TRACKS_GAME_DISLIKE_PROPOSAL_PHRASES)

        self._test(alice(voice('рок')), {'run', 'continue'},
                   in_onboarding=True, directives_count=1, play=True, last_track=last_track,
                   intent='personal_assistant.scenarios.music_play', scenario='music',
                   radio='genre:rock', expects_request=False,
                   speech='Включаю')

        self._test(alice(voice('лайк')), {'run', 'apply'},
                   in_onboarding=True, directives_count=1, play=True, last_track=last_track,
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_like', scenario='player_commands',
                   radio='genre:rock',
                   speech=TRACKS_GAME_LIKE_PHRASES)

        self._test(alice(voice('дизлайк')), {'run', 'apply'},
                   in_onboarding=True, directives_count=1, play=True, last_track=last_track,
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_dislike', scenario='player_commands',
                   radio='genre:rock',
                   speech='Хорошо, попробуем последний.')

        self._test(alice(voice('лайк')), {'run', 'apply'},
                   directives_count=3, play=True, pause=True, last_track=last_track,
                   check_no_unexpected_start_callback=True,
                   intent='personal_assistant.scenarios.player_like', scenario='player_commands',
                   radio='genre:rock',
                   speech='Окей, запомнила. Теперь когда вы будете просить музыку я буду основываться на ваших предпочтениях. Давайте послушаем?')

        self._test(alice(voice('нет')), {'run'},
                   intent='alice.music_onboarding.tracks',
                   speech='Хорошо. Вы можете в любое время сказать мне "Алиса, включи музыку".')
