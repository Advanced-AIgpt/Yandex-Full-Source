import re

import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from .test_general_conversation import (
    get_disscussion_info_entity_key, get_source, get_original_intent, get_proactivity_action_name, get_gc_intent
)


class MovieAkinatorIntents(object):
    ChooseContent = 'alice.movie_akinator.choose_content'
    ChooseSimilarContent = 'alice.movie_akinator.choose_similar_content'
    Recommend = 'alice.movie_akinator.recommend'
    Reset = 'alice.movie_akinator.do_reset'
    ShowDescription = 'alice.movie_akinator.show_description'


GC_FORCE_INTENTS = [
    'alice.general_conversation.proactivity.alice_do',
    'alice.general_conversation.proactivity.bored',
    'alice.general_conversation.lets_discuss_specific_movie',
    'alice.movie_akinator.recommend'
]

EXPERIMENTS = [
    'hw_gc_proactivity',
    'hw_gc_force_proactivity_soft',
    'hw_gc_frame_proactivity',
    'hw_gc_proactivity_timeout=300000',
    'hw_gc_lets_discuss_movie_frames',
    'hw_gc_proactivity_forbidden_prev_scenario_timeout=180000',
    'hw_gc_proactivity_forbidden_prev2_scenario_timeout=240000',
    'hw_gc_proactivity_forbidden_dialog_turn_count_less=0',
    'hw_gc_entity_index',
    'hw_gc_force_entity_soft_rng',
    'hw_gc_entity_discussion_question_suggest',
    'hw_gc_reply_EntityRankerModelName=catboost_movie_interest',
    'hw_gc_proactivity_movie_akinator',
    'hw_gcp_proactivity_movie_akinator',
    'hw_gc_proactivity_force_soft=alice.movie_akinator.recommend',
    'hw_gcp_proactivity_force_soft=alice.movie_akinator.recommend',
    'hw_gc_movie_akinator',
    'mm_postclassifier_gc_force_intents={}'.format(';'.join(GC_FORCE_INTENTS))
]


@pytest.mark.parametrize('surface', [surface.searchapp])
@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.skip(reason='This scenario is abandoned')
class TestMovieAkinator(object):

    owners = ('dan-anastasev',)

    genres = [
        ('мультики', ('cartoon', None)),
        ('фильмы', ('movie', None)),
        ('комедии', ('movie', 'comedy')),
        ('фантастику', ('movie', 'science_fiction')),
        ('боевики', ('movie', 'action')),
        ('семейное кино', ('movie', 'family')),
        ('триллеры', ('movie', 'thriller')),
        ('драматические сериалы', ('tv_show', 'drama')),
    ]

    recommendation_commands = ['посоветуй', 'порекомендуй']
    reset_commands = ['начать сначала', 'давай другие', 'хочу что-нибудь другое', 'оба не нравятся', 'я такое не люблю']

    start_response_prefix = (
        'Я сгруппировала для вас фильмы по похожести. Просто выбирайте группу, '
        'в которой есть интересные вам фильмы, пока не заметите тот, что захотите посмотреть. '
    )

    choose_poster_cloud_response_template = (
        '{}(Какие фильмы больше похожи на то, что вам хотелось бы увидеть?|'
        'Какая группа фильмов ближе к тому, что вам хотелось бы посмотреть?)'
    )

    choose_poster_cloud_on_start_response = choose_poster_cloud_response_template.format(start_response_prefix)
    choose_poster_cloud_response = choose_poster_cloud_response_template.format('')

    choose_poster_response = '(Я могу предложить следующие фильмы.|Какой фильм хотите посмотреть?)'
    show_description_response = '(?:О чем|Покажи описание) «(.*)»'

    analytics_info_poster_cloud_state = 'poster_cloud_state'
    analytics_info_poster_gallery_state = 'poster_gallery_state'

    @staticmethod
    def _get_recognized_frame(response):
        return response.scenario_analytics_info.object('gc_response_info').get('recognized_frame')

    @staticmethod
    def _get_intent_name(response):
        return response.scenario_analytics_info.object('gc_response_info').get('intent_name')

    @staticmethod
    def _get_analytics_info(response):
        return response.scenario_analytics_info.object('gc_response_info').get('movie_akinator_info', {})

    @classmethod
    def _check_genre_in_analytics_info(cls, analytics_info, expected_genre):
        assert analytics_info['content_type'] == expected_genre[0]
        assert analytics_info.get('genre') == expected_genre[1]

    @classmethod
    def _check_poster_cloud_response(cls, response, text):
        assert re.match(cls.choose_poster_cloud_response_template.format('.*'), text)

        posters = response.div_card.container
        assert len(posters) == 2
        for poster in posters:
            assert poster.image_url
            assert poster.action_url

        assert cls._get_analytics_info(response).get(cls.analytics_info_poster_cloud_state)

    @classmethod
    def _get_sample_show_description_suggest_text(cls, response):
        for frame_action_name, frame_action_value in response.scenario_analytics_info.frame_actions:
            if frame_action_name.startswith('show_description_'):
                return frame_action_value.directives.list[0].type_text_silent_directive.text

    @classmethod
    def _check_poster_gallery_suggests(cls, response):
        assert response.suggest('Начать сначала')
        assert cls._get_sample_show_description_suggest_text(response)

    @classmethod
    def _check_poster_gallery_response(cls, response, text):
        assert re.match(cls.choose_poster_response, text)
        cls._check_poster_gallery_suggests(response)

        poster_gallery = response.div_card.gallery
        assert len(poster_gallery) > 0
        for poster_info in poster_gallery:
            assert poster_info.action_url
            assert len(poster_info) == 1
            assert poster_info.first.image_url

        assert cls._get_analytics_info(response).get(cls.analytics_info_poster_gallery_state)

    @pytest.mark.parametrize('recommendation_command', recommendation_commands)
    @pytest.mark.parametrize('input_genre, expected_genre', genres)
    def test_activations(self, alice, recommendation_command, input_genre, expected_genre):
        response = alice(f'{recommendation_command} {input_genre}')
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend

        assert re.match(self.choose_poster_cloud_on_start_response, response.text)
        self._check_poster_cloud_response(response, response.text)

        self._check_genre_in_analytics_info(self._get_analytics_info(response), expected_genre)

    @pytest.mark.parametrize('command', ['хочу', 'давай'])
    @pytest.mark.parametrize('input_genre, expected_genre', genres)
    def test_genre_filters(self, alice, command, input_genre, expected_genre):
        response = alice('посоветуй фильм')
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend
        assert re.match(self.choose_poster_cloud_on_start_response, response.text)

        response = alice(f'{command} {input_genre}')
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.ChooseContent
        assert re.match(self.choose_poster_cloud_response, response.text)
        self._check_poster_cloud_response(response, response.text)

        self._check_genre_in_analytics_info(self._get_analytics_info(response), expected_genre)

    def test_invalid_genre(self, alice):
        response = alice('посоветуй эротический фильм')
        assert response.scenario != scenario.GeneralConversation

    @pytest.mark.parametrize('navigation_command, next_node_key', [
        ('левые', 'LeftNodeId'),
        ('правые', 'RightNodeId'),
        (0, 'LeftNodeId'),
        (1, 'RightNodeId'),
    ])
    def test_descend_to_posters(self, alice, navigation_command, next_node_key):
        expected_genre = ('movie', 'horror')

        response = alice('посоветуй ужастики')
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend
        assert re.match(self.choose_poster_cloud_on_start_response, response.text)

        self._check_poster_cloud_response(response, response.text)

        analytics_info = self._get_analytics_info(response)
        self._check_genre_in_analytics_info(analytics_info, expected_genre)

        expected_next_node = analytics_info[self.analytics_info_poster_cloud_state][next_node_key]

        is_choose_poster = False
        for _ in range(10):
            if isinstance(navigation_command, int):
                response = alice.click(response.div_card.container[navigation_command])
            else:
                response = alice(navigation_command)
            assert response.scenario == scenario.GeneralConversation
            assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
            assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend

            analytics_info = self._get_analytics_info(response)
            self._check_genre_in_analytics_info(analytics_info, expected_genre)
            assert analytics_info['node_id'] == expected_next_node

            is_choose_poster_cloud = re.match(self.choose_poster_cloud_response, response.text)
            is_choose_poster = re.match(self.choose_poster_response, response.text)

            assert is_choose_poster or is_choose_poster_cloud

            if is_choose_poster_cloud:
                self._check_poster_cloud_response(response, response.text)
                expected_next_node = analytics_info[self.analytics_info_poster_cloud_state][next_node_key]
            else:
                self._check_poster_gallery_response(response, response.text)
                break

        assert is_choose_poster

        response = alice.click(response.div_card.gallery[0])
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.ShowDescription
        assert response.suggest('Начать сначала')

        analytics_info = self._get_analytics_info(response)
        show_description_state = analytics_info.get('show_description_state')
        assert show_description_state
        assert analytics_info['node_id'] == expected_next_node

        open_on_kinopoisk_action = 'Открой на кинопоиске'
        assert response.suggest(open_on_kinopoisk_action)

        response = alice(open_on_kinopoisk_action)
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri.startswith('http://www.kinopoisk.ru/film')

    @pytest.mark.parametrize('open_faster_command, next_node_movies_key, next_node_key', [
        ('покажи галерею из левых', 'ShownLeftMovieIds', 'LeftNodeId'),
        ('покажи галерею из правых', 'ShownRightMovieIds', 'RightNodeId'),
    ])
    def test_open_gallery_faster(self, alice, open_faster_command, next_node_movies_key, next_node_key):
        response = alice('порекомендуй комедийный фильм')
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend
        assert re.match(self.choose_poster_cloud_on_start_response, response.text)
        self._check_poster_cloud_response(response, response.text)

        analytics_info = self._get_analytics_info(response)
        expected_movies = analytics_info[self.analytics_info_poster_cloud_state][next_node_movies_key]
        next_node_id = analytics_info[self.analytics_info_poster_cloud_state][next_node_key]

        response = alice(open_faster_command)
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend
        self._check_poster_gallery_response(response, response.text)

        analytics_info = self._get_analytics_info(response)
        node_id = analytics_info['node_id']
        assert node_id == next_node_id

        gallery_state = analytics_info[self.analytics_info_poster_gallery_state]
        shown_movies = gallery_state.get('ShownMovieIds', [])

        min_common_length = min(len(expected_movies), len(shown_movies))
        assert min_common_length > 0
        assert expected_movies[:min_common_length] == shown_movies[:min_common_length]

    @pytest.mark.parametrize('reset_command', reset_commands)
    def test_reset(self, alice, reset_command):
        response = alice('порекомендуй фильмы')
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend
        assert re.match(self.choose_poster_cloud_on_start_response, response.text)
        self._check_poster_cloud_response(response, response.text)

        response = alice(reset_command)
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Reset
        assert re.match(self.choose_poster_cloud_response, response.text)
        self._check_poster_cloud_response(response, response.text)

        response = alice('левые')
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend

        response = alice(reset_command)
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Reset
        self._check_poster_cloud_response(response, response.text)

    @pytest.mark.parametrize('movie_name, movie_id', [
        ('Интерстеллар', 258687),
        ('Побег из Шоушенка', 326),
        ('Карты, деньги, два ствола', 522)
    ])
    @pytest.mark.parametrize('recommendation_command', recommendation_commands)
    def test_choose_by_movie_name(self, alice, recommendation_command, movie_name, movie_id):
        response = alice('посоветуй хороший фильм')
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend
        assert re.match(self.choose_poster_cloud_on_start_response, response.text)
        self._check_poster_cloud_response(response, response.text)

        response = alice(f'{recommendation_command} фильмы, похожие на {movie_name}')
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.ChooseSimilarContent
        assert int(self._get_analytics_info(response)['show_similar_movie_id']) == movie_id
        self._check_poster_cloud_response(response, response.text)

        choose_poster_cloud_response = self.choose_poster_cloud_response_template.format(
            f'Мне кажется, эти фильмы достаточно похожи на «{movie_name}». '
        ) + '.*'
        assert re.match(choose_poster_cloud_response, response.text)
        self._check_poster_cloud_response(response, response.text)

    def test_choose_unknown_movie_name(self, alice):
        alice('посоветуй хороший фильм')

        response = alice('посоветуй фильмы в стиле бла-бла-бла')
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert response.text == 'Извините, я не смогла узнать фильм с таким названием.'
        assert not response.div_card

    @pytest.mark.experiments('hw_gc_reply_EntityBoost=15')
    @pytest.mark.parametrize('show_description', [False, True])
    @pytest.mark.parametrize('click_suggest', [False, True])
    @pytest.mark.parametrize('init_requests, discuss_request', [
        (['посоветуй фильм'], 'давай обсудим фильм {}'),
        (['посоветуй фильм'], 'как тебе сюжет фильма {}'),
        (['посоветуй фильм', 'покажи галерею из левых'], 'как тебе сюжет фильма {}'),
    ])
    def test_lets_discuss_some_movie(self, alice, show_description, click_suggest, init_requests, discuss_request):
        movie_name = 'Начало'

        for request in init_requests:
            alice(request)

        discuss_response = None
        if show_description:
            response = alice(f'о чем фильм {movie_name}')
            assert response.scenario == scenario.GeneralConversation
            assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
            assert self._get_recognized_frame(response) == MovieAkinatorIntents.ShowDescription

            for suggest in response.suggests:
                if movie_name in suggest.title:
                    if click_suggest:
                        discuss_response = alice.click(suggest)
                    else:
                        discuss_response = alice(suggest.title)
                    break

        if discuss_response is None:
            discuss_response = alice(discuss_request.format(movie_name))

        assert discuss_response.scenario == scenario.GeneralConversation
        movie_discuss_intents = {'alice.general_conversation.akinator_movie_discuss',
                                 'alice.general_conversation.lets_discuss_specific_movie'}
        assert self._get_recognized_frame(discuss_response) in movie_discuss_intents
        assert get_disscussion_info_entity_key(discuss_response) == 'movie:447301'
        assert get_source(discuss_response) == 'movie_specific'

        response = alice('как тебе сюжет этого фильма?')
        assert response.scenario == scenario.GeneralConversation
        assert get_disscussion_info_entity_key(response) == 'movie:447301'
        assert get_source(response) == 'movie_specific'

    @pytest.mark.parametrize('is_modal', [False, True])
    def test_gc_frame_proactivity(self, alice, is_modal):
        if is_modal:
            response = alice('давай поболтаем')
            assert response.scenario == scenario.GeneralConversation
            assert response.intent == intent.ExternalSkillGc
            assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response) == 'alice.general_conversation.proactivity.bored'
        assert get_proactivity_action_name(response) == 'alice.movie_akinator.recommend'

        response = alice('давай')
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend

        response = alice('левые')
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend

    @pytest.mark.experiments('hw_gc_reply_ProactivityBoost=10000000', 'hw_gc_reply_EntityBoost=10000000')
    @pytest.mark.parametrize('is_modal', [False, True])
    def test_gc_index_proactivity(self, alice, is_modal):
        if is_modal:
            response = alice('давай поболтаем')
            assert response.scenario == scenario.GeneralConversation
            assert response.intent == intent.ExternalSkillGc
            assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert get_proactivity_action_name(response) == 'alice.movie_akinator.recommend'
        assert get_source(response) == 'proactivity'

        response = alice('давай')
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend

        response = alice('левые')
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend

    @pytest.mark.parametrize('is_positive', [False, True])
    def test_feedback_on_gallery(self, alice, is_positive):
        alice('посоветуй фильм')

        response = alice('покажи галерею из левых')
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend
        assert response.cards[-1].text.endswith('?')
        self._check_poster_gallery_response(response, response.text)

        assert response.suggest('👍')
        assert response.suggest('👎')

        if is_positive:
            response = alice.click(response.suggest('👍'))
            assert response.scenario == scenario.GeneralConversation
            assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
            assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend

            assert response.text.endswith('!')
            assert not response.suggest('👍')
            assert not response.suggest('👎')
            self._check_poster_gallery_suggests(response)
            return

        response = alice.click(response.suggest('👎'))
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Reset

        assert response.cards[0].text.endswith('?')
        assert not response.suggest('👍')
        assert not response.suggest('👎')

        self._check_poster_cloud_response(response, response.cards[1].text)

    @pytest.mark.parametrize('is_positive', [False, True])
    def test_feedback_on_description(self, alice, is_positive):
        alice('посоветуй фильм')

        response = alice('покажи галерею из левых')
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend
        assert response.cards[-1].text.endswith('?')
        self._check_poster_gallery_response(response, response.text)

        assert response.suggest('👍')
        assert response.suggest('👎')

        response = alice.click(response.div_card.gallery[0])
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.ShowDescription

        assert response.cards[-1].text.endswith('?')
        assert response.suggest('👍')
        assert response.suggest('👎')
        assert response.suggest('Открой на кинопоиске')

        if is_positive:
            response = alice.click(response.suggest('👍'))
            assert response.scenario == scenario.GeneralConversation
            assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
            assert self._get_recognized_frame(response) == MovieAkinatorIntents.ShowDescription

            assert response.text.endswith('!')
            assert not response.suggest('👍')
            assert not response.suggest('👎')
            assert response.suggest('Открой на кинопоиске')
            return

        response = alice.click(response.suggest('👎'))
        assert response.scenario == scenario.GeneralConversation
        assert self._get_intent_name(response) == intent.GeneralConversationMovieAkinator
        assert self._get_recognized_frame(response) == MovieAkinatorIntents.Recommend

        assert response.cards[0].text.endswith('?')
        assert not response.suggest('👍')
        assert not response.suggest('👎')
        self._check_poster_gallery_response(response, response.cards[1].text)


@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.skip(reason='This scenario is abandoned')
class TestInvalidSurface(object):

    owners = ('dan-anastasev',)

    def test_invalid_surface(self, alice):
        response = alice('посоветуй фильм')
        gc_response_info = response.scenario_analytics_info.object('gc_response_info') or {}
        assert gc_response_info.get('intent_name') != intent.GeneralConversationMovieAkinator
