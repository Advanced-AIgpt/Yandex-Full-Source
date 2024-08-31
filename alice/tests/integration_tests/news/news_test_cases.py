import json

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from .news_regexp import (
    check_nonews_text,
    check_smi_text,
    check_stub_text,
    check_wizard_text,
)


# Analytics info.
def get_news_type(response):
    selected_source_event = response.scenario_analytics_info.event('selected_source_event')
    return selected_source_event.source if selected_source_event else 'unknown'


def get_news_slot_value(response):
    if 'news' not in response.slots:
        return None
    return json.loads(response.slots['news'].string)


def check_slot_is_empty(response, slot_name):
    if slot_name in response.slots:
        assert response.slots[slot_name].string == 'null'


def assert_slot_value_is_equal(response, slot_name, slot_value):
    if slot_name in response.slots:
        assert response.slots[slot_name].string in (
            slot_value,  # protocol scenario stores 'value'
            f'"{slot_value}"'  # Vins stores '"value"'
        )


def get_open_uri_value(response):
    uri_btn = response.button('Открыть Новости')
    assert uri_btn is not None
    assert len(uri_btn.directives) > 0
    open_uri_directive = [
        _ for _ in uri_btn.directives
        if _.name == directives.names.OpenUriDirective
    ]
    assert len(open_uri_directive) == 1
    return open_uri_directive[0].payload.uri


def check_suggests(response, command, check_more_news=True):
    suggest_it = iter(response.suggests)

    have_rubric_suggests = False
    try:
        suggest = next(suggest_it)
        # Can contains feedback.
        while suggest.title in {'👍', '👎'}:
            suggest = next(suggest_it)
        # Nonews haven't "more" and "settings" suggests.
        if check_more_news:
            # Setting suggest.
            assert suggest.title == 'Настроить новости'
            suggest = next(suggest_it)
            assert suggest.title in {
                'Больше новостей',
                'Дальше',
                'Ещё новости',
                'Расскажи ещё',
            }
            suggest = next(suggest_it)
        # SearchApps show search intent.
        if suggest.title in {f'🔍 "{command.lower()}"', command.lower()}:
            suggest = next(suggest_it)
        # Discovery suggest.
        assert suggest.title == 'Что ты умеешь?'
        suggest = next(suggest_it)
        # News rubric suggests (Cycle breaks with StopIteration).
        while True:
            suggest = next(suggest_it)
            assert suggest.title in {
                'Новости политики',
                'Новости общества',
                'Новости экономики',
                'Мировые новости',
                'Новости спорта',
                'Новости о происшествиях',
                'Новости культуры',
                'Новости технологий',
                'Новости науки',
                'Новости авто',
            }
            have_rubric_suggests = True

    except StopIteration:
        assert not have_rubric_suggests, 'Did not expect rubric suggests'


@pytest.mark.version(hollywood=213)
class _TestNews(object):
    owners = ('olegator', )


@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.searchapp,
    surface.station,
])
class _TestNewsWithSurface(_TestNews):
    pass


class TestNewsTop(_TestNewsWithSurface):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1242
    """

    @pytest.mark.experiments('news_disable_block_mode')
    @pytest.mark.parametrize('command', [
        'покажи новости',
    ])
    def test_default(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        check_slot_is_empty(response, 'topic')
        check_stub_text(response)


class TestNewsAPI(_TestNewsWithSurface):
    """
    test_rubric https://testpalm.yandex-team.ru/testcase/alice-1336
    test_geo_api https://testpalm.yandex-team.ru/testcase/alice-2542
    """

    @pytest.mark.experiments('news_disable_block_mode')
    @pytest.mark.parametrize('command, topic_value', [
        ('главные новости', 'index'),
        ('новости спорта', 'sport'),
        ('новости коронавирус', 'koronavirus'),
        ('происшествия', 'incident'),
    ])
    def test_rubric(self, alice, command, topic_value):
        response = alice(command)
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        assert_slot_value_is_equal(response, 'topic', topic_value)
        check_stub_text(response)

    @pytest.mark.experiments('news_disable_block_mode')
    @pytest.mark.parametrize('where_value', [
        'москвы',
        'россии',
        'италии',
    ])
    def test_geo_api(self, alice, where_value):
        response = alice(f'новости {where_value}')
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        assert_slot_value_is_equal(response, 'where', where_value)
        check_wizard_text(response)


class TestNewsSMI(_TestNewsWithSurface):

    command_and_topic_data = [
        ('dtf', 'dtf'),
        ('московский комсомолец', 'mk'),
    ]

    @pytest.mark.region(region.Moscow)
    @pytest.mark.experiments('news_disable_block_mode')
    @pytest.mark.parametrize('command, topic_value', command_and_topic_data)
    def test_api(self, alice, command, topic_value):
        response = alice(f'новости {command}')
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        assert_slot_value_is_equal(response, 'topic', topic_value)
        check_smi_text(response)

    @pytest.mark.region(region.Berlin)
    @pytest.mark.experiments('news_disable_block_mode')
    @pytest.mark.parametrize('command, topic_value', command_and_topic_data)
    def test_foreign(self, alice, command, topic_value):
        response = alice(f'новости {command}')
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        assert_slot_value_is_equal(response, 'topic', topic_value)
        check_smi_text(response)


@pytest.mark.voice
class TestNewsProhibited(_TestNewsWithSurface):

    @pytest.mark.parametrize('command', [
        'расскажи новости tjournal',
        'новости тиджорнал',
        'какие новости от медузы',
    ])
    def test_prohibited(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        check_nonews_text(response)


@pytest.mark.voice
class TestNewsVoice(_TestNewsWithSurface):

    @pytest.mark.experiments('news_disable_block_mode')
    @pytest.mark.parametrize('command', [
        'новости РБК',
        'новости DTF',
    ])
    def test_api(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        assert response.voice_response.should_listen
        check_smi_text(response)


class TestNewsWizard(_TestNewsWithSurface):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2530
    """

    @pytest.mark.parametrize('command, topic_value', [
        ('последние про путина', 'путина'),
        ('apple', 'apple'),
    ])
    def test_wizard_news(self, alice, command, topic_value):
        response = alice(f'новости {command}')
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        assert_slot_value_is_equal(response, 'topic', topic_value)
        check_wizard_text(response)

    @pytest.mark.parametrize('where_value', ['ишим', 'австралии'])
    def test_geo_wizard(self, alice, where_value):
        response = alice(f'новости {where_value}')
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        assert_slot_value_is_equal(response, 'where', where_value)
        check_wizard_text(response)


class TestNewsComplexQuery(_TestNewsWithSurface):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2529
    https://testpalm.yandex-team.ru/testcase/alice-2575
    """

    @pytest.mark.experiments('alice_complex_news_to_wizard')
    @pytest.mark.parametrize('command, topic_value, where_value', [
        ('коронавирус в австралии', 'koronavirus', 'в австралии'),
        ('думы на украине', 'думы', 'на украине'),
    ])
    def test(self, alice, command, topic_value, where_value):
        response = alice(f'новости {command}')
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        assert_slot_value_is_equal(response, 'topic', topic_value)
        assert_slot_value_is_equal(response, 'where', where_value)


class TestNewsSuggests(_TestNewsWithSurface):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2536
    https://testpalm.yandex-team.ru/testcase/alice-2546
    https://testpalm.yandex-team.ru/testcase/alice-2679

    test_api_suggests
    https://testpalm.yandex-team.ru/testcase/alice-2545
    https://testpalm.yandex-team.ru/testcase/alice-2576

    test_nonews_suggests
    https://testpalm.yandex-team.ru/testcase/alice-2680
    """

    @pytest.mark.parametrize('command', [
        'новости РБК',
        'новости DTF',
    ])
    def test_api_suggests(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        check_suggests(response, command)

    @pytest.mark.parametrize('command', [
        'новости про путина',
        'новости apple',
    ])
    def test_wizard_suggests(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews

        # DIALOG-6538: Wizard may be absent. No news - nothing to do here.
        if get_news_type(response) == 'nonews':
            return
        check_suggests(response, command)

    def test_nonews_suggests(self, alice):
        command = 'новости хиромантии'

        response = alice(command)
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews

        # Wizard may return something.
        if get_news_type(response) != 'nonews':
            return
        check_suggests(response, command, check_more_news=False)


class TestMoreNews(_TestNewsWithSurface):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2543
    """

    @pytest.mark.experiments('news_disable_block_mode')
    def test_enabled(self, alice):
        response = alice('новости')
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        check_slot_is_empty(response, 'topic')
        check_stub_text(response)

        response = alice('дальше')
        assert response.intent == intent.GetNews
        check_slot_is_empty(response, 'topic')
        check_stub_text(response)

        response = alice('покажи новости армии')
        assert response.intent == intent.GetNews
        assert_slot_value_is_equal(response, 'topic', 'army')
        check_stub_text(response)

        response = alice('следующая новость')
        assert response.intent == intent.GetNews
        assert_slot_value_is_equal(response, 'topic', 'army')
        check_stub_text(response)

        response = alice('найди новости тюмени')
        assert response.intent == intent.GetNews
        assert_slot_value_is_equal(response, 'where', 'тюмени')
        check_wizard_text(response)

        response = alice('ещё')
        assert response.intent == intent.GetNews
        assert_slot_value_is_equal(response, 'where', 'тюмени')
        check_wizard_text(response)

        response = alice('продолжить')
        assert response.intent == intent.GetNews
        assert_slot_value_is_equal(response, 'where', 'тюмени')
        check_wizard_text(response)


@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestNewsAnalyticsInfo(_TestNews):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2540
    """

    # Flag to enable kurezy rubric.
    @pytest.mark.experiments('alice_funny_news')
    @pytest.mark.parametrize('command, expected_news_type', [
        ('главные новости', 'nonews'),
        ('новости спорта', 'nonews'),
        ('новости про путина', 'nonews'),
        ('новости хиромантии', 'nonews'),
        ('смешные новости', 'nonews'),
        ('новости москвы', 'nonews'),
        ('новости РБК', 'smi'),
    ])
    def test_news_type(self, alice, command, expected_news_type):
        response = alice(command)
        assert response.scenario == scenario.News

        # DIALOG-6538: Wizard may be absent and return 'nonews' type. Don't assert.
        if expected_news_type == 'wizard' and get_news_type(response) == 'nonews':
            return
        # Wizard cat return 1-2 common stories for nonsense queries.
        elif expected_news_type == 'nonews' and get_news_type(response) != 'nonews':
            return

        assert get_news_type(response) == expected_news_type


class TestNewsForeign(_TestNewsWithSurface):

    @pytest.mark.region(region.Berlin)
    @pytest.mark.parametrize('command', [
        'новости',
        'происшествия',
        'новости про путина',
    ])
    def test_berlin(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        if get_news_type(response) == 'nonews':
            return
        check_wizard_text(response)

    @pytest.mark.region(region.Minsk)
    @pytest.mark.experiments('news_disable_block_mode')
    @pytest.mark.parametrize('command', [
        'новости',
        'происшествия',
        'новости спорта',
    ])
    def test_minsk(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        check_stub_text(response)


@pytest.mark.skip(reason='https://st.yandex-team.ru/ALICERELEASE-1419#5f749ef2b28e526740e8d752')
class TestNewsPersonalNews(_TestNewsWithSurface):

    def test_cold_topic(self, alice):
        # Personal disabled for cold user.
        response = alice('интересные новости')
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        assert get_news_type(response) == 'nonews'

    @pytest.mark.oauth(auth.NewsAPIHotUser)
    def test_hot_topic(self, alice):
        response = alice('интересные новости')
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        assert get_news_type(response) == 'nonews'

    @pytest.mark.oauth(auth.NewsAPIHotUser)
    @pytest.mark.experiments('alice_personal_news')
    def test_hot_default(self, alice):
        response = alice('новости')
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        assert get_news_type(response) == 'nonews'


@pytest.mark.parametrize('surface', [surface.loudspeaker, surface.station])
class TestNewsPlayerPause(_TestNews):

    def test_no_music(self, alice):
        response = alice('новости РБК')
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        assert response.directive
        assert response.directive.name == directives.names.MmStackEngineGetNextCallback

    @pytest.mark.oauth(auth.YandexPlus)
    def test_music_is_playing(self, alice):
        response = alice('включи Queen – Bohemian Rhapsody')
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('новости РБК')
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        assert len(response.directives) == 4, 'Expect player pause, tts, and get_next directives'
        assert response.directives[0].name == directives.names.PlayerPauseDirective
        assert response.directives[1].name == directives.names.ClearQueueDirective
        assert response.directives[2].name == directives.names.TtsPlayPlaceholderDirective
        assert response.directives[3].name == directives.names.MmStackEngineGetNextCallback

        response.next()
        assert response.intent == intent.GetNews


class TestNoRubrics(_TestNewsWithSurface):
    commands = [
        'новости науки'
    ]

    @pytest.mark.version(hollywood=204)
    @pytest.mark.region(region.Moscow)
    @pytest.mark.experiments('news_disable_block_mode', 'news_disable_rubric_api')
    @pytest.mark.parametrize('command', commands)
    def test_api(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.News
        assert response.intent == intent.GetNews
        check_suggests(response, command)
        check_smi_text(response)
