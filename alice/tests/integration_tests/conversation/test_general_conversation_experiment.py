import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


def get_proactivity_action_name(response):
    return response.scenario_analytics_info.object('gc_response_info').get('proactivity_info', dict()).get('action_name')


def get_proactivity_entity_key(response):
    return response.scenario_analytics_info.object('gc_response_info').get('proactivity_info', dict()).get('entity_key')


def get_disscussion_info_entity_key(response):
    return response.scenario_analytics_info.object('gc_response_info').get('discussion_info', dict()).get('entity_key')


def get_source(response):
    return response.scenario_analytics_info.object('gc_response_info').get('source')


def get_gc_intent(response):
    return response.scenario_analytics_info.object('gc_response_info').get('gc_intent')


def get_original_intent(response):
    return response.scenario_analytics_info.object('gc_response_info').get('original_intent')


PROACTIVITY_FLAGS = ['hw_gc_proactivity', 'hw_gc_frame_proactivity', 'hw_gc_reply_ProactivityBoost=10000000', 'hw_gc_force_proactivity_soft',
                     'hw_gc_proactivity_timeout=300000', 'hw_gc_movie_open_suggest_prob=1.0']
ENTITY_INDEX_FLAGS = ['hw_gc_entity_index', 'hw_gc_reply_EntityBoost=15000000', 'hw_gc_force_entity_soft_rng',
                      'hw_gc_proactivity_movie_discuss_entity_boost=15000000', 'hw_gc_proactivity_movie_discuss_specific_wacthed_entity_boost=15000000',
                      'hw_gc_proactivity_movie_discuss_specific_not_wacthed_entity_boost=15000000']
ENTITY_SEARCH_FLAGS = ['hw_gc_proactivity_entity_search', 'hw_gc_proactivity_entity_search_exp=last_viewed_films']
LETS_DISCUSS_FLAGS = ['hw_gc_lets_discuss_movie_frames']
BASE_FLAGS = PROACTIVITY_FLAGS + LETS_DISCUSS_FLAGS + \
    ['hw_gc_disable_movie_discussions_by_default', 'hw_gc_proactivity_forbidden_dialog_turn_count_less=0'] + \
    ['mm_postclassifier_gc_force_intents=alice.general_conversation.proactivity.alice_do;alice.general_conversation.proactivity.bored;' +
     'alice.general_conversation.lets_discuss_specific_movie;alice.general_conversation.lets_discuss_some_movie']
OO_INDEX_SUGGESTS_FLAGS = BASE_FLAGS + ENTITY_INDEX_FLAGS + ENTITY_SEARCH_FLAGS + ['hw_gc_entity_discussion_question_suggest', 'hw_gc_proactivity_movie_discuss_suggests']
OO_INDEX_SUGGESTS_RERANKER_FLAGS = OO_INDEX_SUGGESTS_FLAGS + ['hw_gc_reply_EntityRankerModelName=catboost_movie_interest']


@pytest.mark.parametrize('surface', [surface.station])
class TestExperimentBase(object):

    owners = ('nzinov',)

    experiments = BASE_FLAGS

    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss')
    def test_index_movie_discuss(self, alice):
        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'proactivity'

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) != 'proactivity'

    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss_specific')
    def test_index_movie_discuss_specific(self, alice):
        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'proactivity'
        entity_key = get_disscussion_info_entity_key(response)
        assert entity_key.startswith('movie:')

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) != 'proactivity'
        assert get_disscussion_info_entity_key(response) == entity_key

    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss')
    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    def test_index_modal_movie_discuss(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'proactivity'

        response = alice('песнь моря')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_disscussion_info_entity_key(response) == 'movie:714248'

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) != 'proactivity'
        assert get_disscussion_info_entity_key(response) == 'movie:714248'

    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss_specific')
    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    def test_index_modal_movie_discuss_specific(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'proactivity'
        entity_key = get_disscussion_info_entity_key(response)
        assert entity_key.startswith('movie:')

        response = alice('норм фильм')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_disscussion_info_entity_key(response) == entity_key

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) != 'proactivity'
        assert get_disscussion_info_entity_key(response) == entity_key

    @pytest.mark.experiments('hw_gcp_proactivity_movie_discuss')
    def test_frame_movie_discuss(self, alice):
        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response) == 'alice.general_conversation.proactivity.bored'
        assert get_proactivity_action_name(response) == 'alice.movie_discuss'

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response) != 'alice.general_conversation.proactivity.bored'

    @pytest.mark.experiments('hw_gcp_proactivity_movie_discuss_specific')
    def test_frame_movie_discuss_specific(self, alice):
        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response) == 'alice.general_conversation.proactivity.bored'
        assert get_proactivity_action_name(response) == 'alice.movie_discuss_specific'
        entity_key = get_disscussion_info_entity_key(response)
        assert entity_key.startswith('movie:')

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response) != 'alice.general_conversation.proactivity.bored'

    @pytest.mark.experiments('hw_gcp_proactivity_movie_discuss')
    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    def test_frame_modal_movie_discuss(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_proactivity_action_name(response) == 'alice.movie_discuss'

        response = alice('песнь моря')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_disscussion_info_entity_key(response) == 'movie:714248'

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_proactivity_action_name(response) is None

    @pytest.mark.experiments('hw_gcp_proactivity_movie_discuss_specific')
    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    def test_frame_modal_movie_discuss_specific(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_proactivity_action_name(response) == 'alice.movie_discuss_specific'
        entity_key = get_disscussion_info_entity_key(response)
        assert entity_key.startswith('movie:')

        response = alice('норм фильм')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_disscussion_info_entity_key(response) == entity_key

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_proactivity_action_name(response) is None
        assert get_disscussion_info_entity_key(response) == entity_key

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-7552')
    def test_lets_discuss_movie_general(self, alice):
        entity_key_matrix = 'movie:301'
        response = alice('давай обсудим фильм матрица')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response).endswith(intent.LetsDiscussSpecificMovie)
        assert get_disscussion_info_entity_key(response) == entity_key_matrix

        response = alice('мне он понравился')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_disscussion_info_entity_key(response) == entity_key_matrix

        response = alice('о чем он')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_disscussion_info_entity_key(response) == entity_key_matrix

        response = alice('давай обсудим другой фильм')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response) == intent.LetsDiscussSomeMovie
        entity_key_some_movie = get_disscussion_info_entity_key(response)
        assert entity_key_some_movie is not None
        assert entity_key_some_movie != entity_key_matrix

        response = alice('давай обсудим мультфильм')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response) == intent.LetsDiscussSomeMovie
        entity_key_some_mult = get_disscussion_info_entity_key(response)
        assert entity_key_some_mult is not None
        assert entity_key_some_mult != entity_key_matrix
        assert entity_key_some_mult != entity_key_some_movie

    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss', 'hw_gc_movie_open_suggest_prob=1.0')
    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('play_request', [
        'включи его',
        'включи этот фильм',
        'включи фильм Джентльмены',
    ])
    def test_movie_discuss_movie_open(self, alice, play_request):
        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'proactivity'

        response = alice('Джентльмены')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == 'movie:1143242'
        assert 'Включи фильм «Джентльмены»' in (response.suggests[0]['title'], response.suggests[1]['title'])

        response = alice(play_request)
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.name == 'Джентльмены'


class TestExperimentOO_Index_Suggests(TestExperimentBase):

    owners = ('nzinov',)

    experiments = OO_INDEX_SUGGESTS_FLAGS

    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss')
    def test_movie_discuss_question_suggest(self, alice):
        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'proactivity'

        response = alice('песнь моря')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'movie_specific'
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == 'movie:714248'
        question_suggest = response.suggests[0]['title']
        assert question_suggest.endswith('?')

        for _ in range(5):
            response = alice(question_suggest)
            assert response.scenario == scenario.GeneralConversation
            assert response.intent == intent.GeneralConversation
            assert get_source(response) == 'movie_specific'
            assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == 'movie:714248'
            question_suggest = response.suggests[0]['title']
            assert question_suggest.endswith('?')

    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss')
    @pytest.mark.parametrize('suggest_num', [0, 1, 2])
    def test_movie_discuss_movie_suggest(self, alice, suggest_num):
        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'proactivity'

        response = alice(response.suggests[suggest_num]['title'])
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) is not None
        question_suggest = response.suggests[0]['title']
        assert question_suggest.endswith('?')


class TestExperimentOO_Index_Suggests_Reranker(TestExperimentOO_Index_Suggests):

    owners = ('nzinov',)

    experiments = OO_INDEX_SUGGESTS_RERANKER_FLAGS
