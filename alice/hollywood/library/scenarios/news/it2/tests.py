import logging
import re

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice, server_action


logger = logging.getLogger(__name__)


SCENARIO_NAME = 'News'
SCENARIO_HANDLE = 'news'


def make_news_memento_config(source: str, rubric: str, is_onboarded=False) -> dict:
    return {
        'UserConfigs': {
            'NewConfig': {
                'NewsConfig': [
                    {
                        'NewsProvider': {
                            'Rubric': rubric,
                            'NewsSource': source,
                        }
                    }
                ],
                'IsOnboarded': is_onboarded,
            }
        }
    }


MEMENTO_CONFIG_NEWS_CULTURE_RUBRIC = make_news_memento_config('6e24a5bb-yandeks-novost', 'culture')
MEMENTO_CONFIG_NEWS_DEFAULT = make_news_memento_config('6e24a5bb-yandeks-novost', 'index')
MEMENTO_CONFIG_NEWS_DEFAULT_ONBOARDED = make_news_memento_config('6e24a5bb-yandeks-novost', 'index', is_onboarded=True)
MEMENTO_CONFIG_NEWS_RADIO_PROVIDER = make_news_memento_config('86db974e-biznes-fm', 'main')
MEMENTO_CONFIG_NEWS_AIF_SOURCE = make_news_memento_config('b83e90ec-argumenty-i-fakty', 'main')
MEMENTO_CONFIG_NEWS_RIA_SOURCE = make_news_memento_config('35376ef1-ria-novosti', 'main')
MEMENTO_CONFIG_NEWS_NPLUS1_SOURCE = make_news_memento_config('c16d4bd9-n-1', 'main')
MEMENTO_CONFIG_NEWS_NPLUS1_SOURCE_ONBOARDED = make_news_memento_config('c16d4bd9-n-1', 'main', is_onboarded=True)
MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC = make_news_memento_config('6e24a5bb-yandeks-novost', 'science')
MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC_ONBOARDED = make_news_memento_config('6e24a5bb-yandeks-novost', 'science', is_onboarded=True)


DEFAULT_INTRO_RE = (
    r'(Вот последние новости\.'
    r'|В эфире новости\.'
    r'|Всегда мечтала стать ведущей, вот последние новости\.'
    r'|Новости сами себя не прочтут\. А, нет\. Прочтут\.'
    r'|Вот последние новости к этому часу\. Надеюсь, хороших будет больше\.)'
)
SERP_INTRO_RE = (
    r'(Вот все, что есть по этой теме\.'
    r'|Нашла кое-что похожее, слушайте\.'
    r'|Надеюсь, это то, что вы искали, слушайте\.)'
)
SMI_INTRO_TEMPLATE = (
    r'(Читаю последние новости с сайта'
    r'|Конечно. Читаю новости с сайта'
    r'|Сейчас все узнаем. Читаю новости с сайта'
    r'|Без проблем. Читаю новости с сайта'
    r'|Как скажете. Включаю новости с сайта)'
) + ' {}.'
ONBOARDING_PART_INTRO_TEMPLATE = (
    r'(Читаю новости с сайта {0}\. Все как заказывали в настройках\.'
    r'|Читаю из источника, который вы выбрали\.'
    r'|Читаю из источника, который указан в настройках\.'
    r'|Читаю новости, которые вы всегда слушаете\.'
    r'|Читаю из того источника, который вам нравится, как я поняла\.)'
)
ONBOARDING_FULL_INTRO_TEMPLATE = (
    r'(Источников новостей много, включаю случайный - сегодня это {0}\. Если он вам не подойдет, скажите мне: Алиса, настрой новости\.'
    r'|Ок, зачитаю новости из первого попавшегося источника, в этот раз это {0}\. Но вы всегда можете выбрать любимые СМИ в настройках в приложении Яндекса\.'
    r'|Ок, вот новости из случайного источника - {0}\. Но вы всегда можете выбрать источник по умолчанию, просто скажите мне: Алиса, настрой новости\.'
    r'|Вы пока не указали в настройках, какие СМИ вам нравятся, поэтому я выбрала наугад. Вот последние новости с сайта {0}\.'
    r'|Включаю, в этот раз попались новости из источника {0}, потому что вы до сих пор не выбрали свои любимые СМИ\. Чтобы сделать это, скажите: Алиса, настрой новости\.)'
)
RUBRIC_API_NLG = (
    'Чтобы слушать новости, попросите меня рассказать новости из конкретного источника или выберите его в настройках. '
    'Для этого скажите мне в приложении Яндекс: Алиса, настрой новости.'
)
NO_NEWS_NLG = 'К сожалению, я не смогла найти новостей по данному запросу.'


def _try_find_server_directive(response_body, directive_type):
    for d in response_body.ServerDirectives:
        if d.HasField(directive_type):
            return getattr(d, directive_type)
    return None


@pytest.fixture(scope='module')
def enabled_scenarios():
    return [SCENARIO_HANDLE]


@pytest.mark.scenario(name=SCENARIO_NAME, handle=SCENARIO_HANDLE)
class _TestBase:
    pass


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.smart_display])
class _SmartDisplayTestBase(_TestBase):
    pass


class TestGetNews(_SmartDisplayTestBase):

    def test_get_news_topic_sport(self, alice):
        r = alice(voice('новости спорта'))
        assert r.scenario_stages() == {'run'}
        response_body = r.run_response.ResponseBody
        directives = response_body.Layout.Directives
        assert len(directives) == 2

        assert directives[0].HasField('ShowViewDirective')
        directive = directives[0].ShowViewDirective
        assert directive.HasField('Div2Card')

        assert directives[1].HasField('TtsPlayPlaceholderDirective')

        return str(r)

    @pytest.mark.xfail(reason='Ongoing emergency News fixes, no time to support non-prod surface')
    @pytest.mark.parametrize('dummy', [
        pytest.param(1, id='default'),
        pytest.param(2, id='radio_provider', marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_RADIO_PROVIDER))
    ])
    def test_get_news_teasers(self, alice, dummy):
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'centaur_collect_cards': {}
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'teasers'
            }
        }))

        assert r.scenario_stages() == {'run'}

        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 5

        card_ids = set()
        for directive in directives:
            assert directive.AddCardDirective.HasField('Div2Card')
            card_id = directive.AddCardDirective.CardId
            assert len(card_id) > 0
            assert card_id not in card_ids
            card_ids.add(card_id)

        return str(r)

    @pytest.mark.xfail(reason='Ongoing emergency News fixes, no time to support non-prod surface')
    @pytest.mark.parametrize('dummy', [
        pytest.param(1, id='default'),
        pytest.param(2, id='radio_provider', marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_RADIO_PROVIDER))
    ])
    def test_get_news_main_screen(self, alice, dummy):
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'centaur_collect_main_screen': {}
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'main_screen'
            }
        }))

        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.HasField('ScenarioData')
        scenario_data = r.run_response.ResponseBody.ScenarioData
        assert scenario_data.HasField('NewsMainScreenData')

        return str(r)

    @pytest.mark.xfail(reason='Ongoing emergency News fixes, no time to support non-prod surface')
    @pytest.mark.parametrize('dummy', [
        pytest.param(1, id='default'),
        pytest.param(2, id='radio_provider', marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_RADIO_PROVIDER))
    ])
    def test_get_news_main_screen_widget(self, alice, dummy):
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'centaur_collect_widget_gallery_semantic_frame': {}
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'main_screen'
            }
        }))

        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.HasField('ScenarioData')
        scenario_data = r.run_response.ResponseBody.ScenarioData
        assert scenario_data.HasField('NewsMainScreenData')

        return str(r)


@pytest.mark.oauth(auth.Yandex)
class TestNewsSettings(_TestBase):

    @pytest.mark.parametrize('surface', [surface.legatus])
    def test_news_settings_legatus(self, alice):
        r = alice(voice('настрой новости'))
        assert r.scenario_stages() == {'run'}
        response_body = r.run_response.ResponseBody
        assert not response_body.Layout.Directives
        assert response_body.Layout.OutputSpeech in [
            'Отправила уведомление, продолжим в приложении Яндекс, откройте его.',
            'Отправила пуш на телефон. Чтобы его прочесть, зайдите в приложение Яндекс.',
        ]

        return str(r)


@pytest.mark.parametrize('surface', [surface.station])
class TestPlayerPause(_TestBase):

    @pytest.mark.parametrize('dummy', [
        pytest.param(1, id='bluetooth', marks=pytest.mark.device_state(
            bluetooth={
                'currently_playing': {
                    'track_id': '24417',
                },
            },
        )),
        pytest.param(2, id='radio', marks=pytest.mark.device_state(
            radio={
                'currently_playing': {
                    'track_id': '24417',
                },
            },
        )),
        pytest.param(3, id='music', marks=pytest.mark.device_state(
            music={
                'currently_playing': {
                    'track_id': '24417',
                },
            },
        )),
        pytest.param(4, id='audio', marks=pytest.mark.device_state(
            audio_player={
                'player_state': 'Playing',
                'scenario_meta': {
                    'owner': 'music',
                    'what_is_playing_answer': 'Rammstein, "Du Riechst So Gut"',
                },
            },
        )),
    ])
    def test(self, alice, dummy):
        assert dummy  # to prevent 'unused' error

        r = alice(voice('расскажи новости'))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert any(d.HasField('PlayerPauseDirective') for d in directives)


@pytest.mark.parametrize('surface', [surface.station])
class TestNoRubrics(_TestBase):
    COMMAND_NEWS = 'новости'
    COMMAND_NEWS_MAIN = 'главные новости'
    COMMAND_NEWS_CULTURE = 'новости культуры'
    COMMAND_NEWS_SCIENCE = 'новости науки'
    COMMAND_NEWS_RBK = 'новости РБК'
    COMMAND_NEWS_SERP = 'новости про взрыв на макаронной фабрике'

    NEWS_MAIN_RE = SMI_INTRO_TEMPLATE.format('(РБК|Газета ru|Коммерсантъ|Лента ru|РИА Новости)')
    NEWS_SCIENCE_RE = SMI_INTRO_TEMPLATE.format('N plus 1')
    NEWS_AIF_RE = SMI_INTRO_TEMPLATE.format('Аргументы и Факты')
    NEWS_RBK_RE = SMI_INTRO_TEMPLATE.format('РБК')
    NEWS_RIA_RE = SMI_INTRO_TEMPLATE.format('РИА Новости')
    NEWS_NPLUS1_RE = SMI_INTRO_TEMPLATE.format('N plus 1')

    NEWS_MAIN_ONBOARDING_FULL_RE = ONBOARDING_FULL_INTRO_TEMPLATE.format('(РБК|Газета ru|Коммерсантъ|Лента ru|РИА Новости)')
    NEWS_SCIENCE_ONBOARDING_FULL_RE = ONBOARDING_FULL_INTRO_TEMPLATE.format('N plus 1')
    NEWS_NPLUS1_ONBOARDING_PART_RE = ONBOARDING_PART_INTRO_TEMPLATE.format('N plus 1')

    # Checking the behavior defined by this scheme: https://wiki.yandex-team.ru/users/andreim/alice/scenarios/news/table_v2/
    @pytest.mark.parametrize(
        'command, response_re',
        [
            pytest.param(
                COMMAND_NEWS,
                RUBRIC_API_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT),
                id='news_default_memento',
            ),
            pytest.param(
                COMMAND_NEWS_MAIN,
                RUBRIC_API_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT),
                id='main_news_default_memento',
            ),
            pytest.param(
                COMMAND_NEWS_CULTURE,
                RUBRIC_API_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT),
                id='culture_news_default_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SCIENCE,
                RUBRIC_API_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT),
                id='science_news_default_memento',
            ),
            pytest.param(
                COMMAND_NEWS_RBK,
                NEWS_RBK_RE,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT),
                id='rbk_news_default_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SERP,
                NO_NEWS_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT),
                id='serp_news_default_memento',
            ),

            pytest.param(
                COMMAND_NEWS,
                RUBRIC_API_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC),
                id='news_science_memento',
            ),
            pytest.param(
                COMMAND_NEWS_MAIN,
                RUBRIC_API_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC),
                id='main_news_science_memento',
            ),
            pytest.param(
                COMMAND_NEWS_CULTURE,
                RUBRIC_API_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC),
                id='culture_news_science_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SCIENCE,
                RUBRIC_API_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC),
                id='science_news_science_memento',
            ),
            pytest.param(
                COMMAND_NEWS_RBK,
                NEWS_RBK_RE,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC),
                id='rbk_news_science_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SERP,
                NO_NEWS_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC),
                id='serp_news_science_memento',
            ),

            pytest.param(
                COMMAND_NEWS,
                RUBRIC_API_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_CULTURE_RUBRIC),
                id='news_culture_memento',
            ),
            pytest.param(
                COMMAND_NEWS_MAIN,
                RUBRIC_API_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_CULTURE_RUBRIC),
                id='main_news_culture_memento',
            ),
            pytest.param(
                COMMAND_NEWS_CULTURE,
                RUBRIC_API_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_CULTURE_RUBRIC),
                id='culture_news_culture_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SCIENCE,
                RUBRIC_API_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_CULTURE_RUBRIC),
                id='science_news_culture_memento',
            ),
            pytest.param(
                COMMAND_NEWS_RBK,
                NEWS_RBK_RE,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_CULTURE_RUBRIC),
                id='rbk_news_culture_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SERP,
                NO_NEWS_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_CULTURE_RUBRIC),
                id='serp_news_culture_memento',
            ),

            pytest.param(
                COMMAND_NEWS,
                NEWS_AIF_RE,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_AIF_SOURCE),
                id='news_aif_memento',
            ),
            pytest.param(
                COMMAND_NEWS_MAIN,
                RUBRIC_API_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_AIF_SOURCE),
                id='main_news_aif_memento',
            ),
            pytest.param(
                COMMAND_NEWS_CULTURE,
                RUBRIC_API_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_AIF_SOURCE),
                id='culture_news_aif_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SCIENCE,
                RUBRIC_API_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_AIF_SOURCE),
                id='science_news_aif_memento',
            ),
            pytest.param(
                COMMAND_NEWS_RBK,
                NEWS_RBK_RE,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_AIF_SOURCE),
                id='rbk_news_aif_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SERP,
                NO_NEWS_NLG,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_AIF_SOURCE),
                id='serp_news_aif_memento',
            ),
        ],
    )
    def test_smoke(self, alice, command, response_re):
        r = alice(voice(command))
        assert re.fullmatch(
            response_re,
            r.run_response.ResponseBody.Layout.OutputSpeech,
        )

    @pytest.mark.experiments('news_disable_rubric_api')
    @pytest.mark.experiments(
        'alice_news_onboarding_first_proba=1',
        'alice_news_onboarding_default_full_proba=1',
        'alice_news_onboarding_default_part_proba=1',
    )
    @pytest.mark.parametrize(
        'command, response_re, update_memento',
        [
            pytest.param(
                COMMAND_NEWS,
                NEWS_MAIN_ONBOARDING_FULL_RE,
                True,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT),
                id='news_default_memento',
            ),
            pytest.param(
                COMMAND_NEWS,
                NEWS_MAIN_ONBOARDING_FULL_RE,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT_ONBOARDED),
                id='news_default_memento_onboarded',
            ),
            pytest.param(
                COMMAND_NEWS_MAIN,
                NEWS_MAIN_ONBOARDING_FULL_RE,
                True,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT),
                id='main_news_default_memento',
            ),
            pytest.param(
                COMMAND_NEWS_MAIN,
                NEWS_MAIN_ONBOARDING_FULL_RE,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT_ONBOARDED),
                id='main_news_default_memento_onboarded',
            ),
            pytest.param(
                COMMAND_NEWS_SCIENCE,
                NEWS_SCIENCE_RE,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT),
                id='science_news_default_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SCIENCE,
                NEWS_SCIENCE_RE,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT_ONBOARDED),
                id='science_news_default_memento_onboarded',
            ),

            pytest.param(
                COMMAND_NEWS,
                NEWS_SCIENCE_ONBOARDING_FULL_RE,
                True,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC),
                id='news_science_memento',
            ),
            pytest.param(
                COMMAND_NEWS,
                NEWS_SCIENCE_ONBOARDING_FULL_RE,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC_ONBOARDED),
                id='news_science_memento_onboarded',
            ),
            pytest.param(
                COMMAND_NEWS_MAIN,
                NEWS_MAIN_ONBOARDING_FULL_RE,
                True,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC),
                id='main_news_science_memento',
            ),
            pytest.param(
                COMMAND_NEWS_MAIN,
                NEWS_MAIN_ONBOARDING_FULL_RE,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC_ONBOARDED),
                id='main_news_science_memento_onboarded',
            ),
            pytest.param(
                COMMAND_NEWS_SCIENCE,
                NEWS_SCIENCE_RE,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC),
                id='science_news_science_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SCIENCE,
                NEWS_SCIENCE_RE,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC_ONBOARDED),
                id='science_news_science_memento_onboarded',
            ),

            pytest.param(
                COMMAND_NEWS,
                NEWS_NPLUS1_ONBOARDING_PART_RE,
                True,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_NPLUS1_SOURCE),
                id='news_nplus1_memento',
            ),
            pytest.param(
                COMMAND_NEWS,
                NEWS_NPLUS1_ONBOARDING_PART_RE,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_NPLUS1_SOURCE_ONBOARDED),
                id='news_nplus1_memento_onboarded',
            ),
            pytest.param(
                COMMAND_NEWS_MAIN,
                NEWS_MAIN_RE,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_NPLUS1_SOURCE),
                id='main_news_nplus1_memento',
            ),
            pytest.param(
                COMMAND_NEWS_MAIN,
                NEWS_MAIN_RE,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_NPLUS1_SOURCE_ONBOARDED),
                id='main_news_nplus1_memento_onboarded',
            ),
            pytest.param(
                COMMAND_NEWS_SCIENCE,
                NEWS_SCIENCE_RE,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_NPLUS1_SOURCE),
                id='science_news_nplus1_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SCIENCE,
                NEWS_SCIENCE_RE,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_NPLUS1_SOURCE_ONBOARDED),
                id='science_news_nplus1_memento_onboarded',
            ),
        ],
    )
    def test_onboarding(self, alice, command, response_re, update_memento):
        r = alice(voice(command))
        response_body = r.run_response.ResponseBody
        assert re.fullmatch(
            response_re,
            response_body.Layout.OutputSpeech,
        )

        update_memento_directive = _try_find_server_directive(response_body, 'MementoChangeUserObjectsDirective')
        if update_memento:
            assert update_memento_directive
        else:
            assert not update_memento_directive

    @pytest.mark.experiments('hw_disable_news')
    @pytest.mark.parametrize(
        'command, response_re, is_irrelevant',
        [
            pytest.param(
                COMMAND_NEWS,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT),
                id='news_default_memento',
            ),
            pytest.param(
                COMMAND_NEWS_MAIN,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT),
                id='main_news_default_memento',
            ),
            pytest.param(
                COMMAND_NEWS_CULTURE,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT),
                id='culture_news_default_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SCIENCE,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT),
                id='science_news_default_memento',
            ),
            pytest.param(
                COMMAND_NEWS_RBK,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT),
                id='rbk_news_default_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SERP,
                NO_NEWS_NLG,
                True,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_DEFAULT),
                id='serp_news_default_memento',
            ),

            pytest.param(
                COMMAND_NEWS,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC),
                id='news_science_memento',
            ),
            pytest.param(
                COMMAND_NEWS_MAIN,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC),
                id='main_news_science_memento',
            ),
            pytest.param(
                COMMAND_NEWS_CULTURE,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC),
                id='culture_news_science_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SCIENCE,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC),
                id='science_news_science_memento',
            ),
            pytest.param(
                COMMAND_NEWS_RBK,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC),
                id='rbk_news_science_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SERP,
                NO_NEWS_NLG,
                True,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_SCIENCE_RUBRIC),
                id='serp_news_science_memento',
            ),

            pytest.param(
                COMMAND_NEWS,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_CULTURE_RUBRIC),
                id='news_culture_memento',
            ),
            pytest.param(
                COMMAND_NEWS_MAIN,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_CULTURE_RUBRIC),
                id='main_news_culture_memento',
            ),
            pytest.param(
                COMMAND_NEWS_CULTURE,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_CULTURE_RUBRIC),
                id='culture_news_culture_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SCIENCE,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_CULTURE_RUBRIC),
                id='science_news_culture_memento',
            ),
            pytest.param(
                COMMAND_NEWS_RBK,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_CULTURE_RUBRIC),
                id='rbk_news_culture_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SERP,
                NO_NEWS_NLG,
                True,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_CULTURE_RUBRIC),
                id='serp_news_culture_memento',
            ),

            pytest.param(
                COMMAND_NEWS,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_RIA_SOURCE),
                id='news_ria_memento',
            ),
            pytest.param(
                COMMAND_NEWS_MAIN,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_RIA_SOURCE),
                id='main_news_ria_memento',
            ),
            pytest.param(
                COMMAND_NEWS_CULTURE,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_RIA_SOURCE),
                id='culture_news_ria_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SCIENCE,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_RIA_SOURCE),
                id='science_news_ria_memento',
            ),
            pytest.param(
                COMMAND_NEWS_RBK,
                NO_NEWS_NLG,
                False,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_RIA_SOURCE),
                id='rbk_news_ria_memento',
            ),
            pytest.param(
                COMMAND_NEWS_SERP,
                NO_NEWS_NLG,
                True,
                marks=pytest.mark.memento(MEMENTO_CONFIG_NEWS_RIA_SOURCE),
                id='serp_news_ria_memento',
            ),
        ],
    )
    def test_disabled_news(self, alice, command, response_re, is_irrelevant):
        r = alice(voice(command))
        assert re.fullmatch(
            response_re,
            r.run_response.ResponseBody.Layout.OutputSpeech,
        )
        assert r.run_response.Features.IsIrrelevant == is_irrelevant
