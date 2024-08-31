from library.python import resource

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

import notifications.notificator_client as notificator_client
import notifications.subscriptions as subscriptions


def _assert_subscription_accept(response):
    assert response.intent == intent.NotificationsSubscribe
    assert 'notification_subscription_accept' in response.scenario_analytics_info.objects


def _assert_unsubscription_accept(response):
    assert response.intent == intent.NotificationsUnsubscribe
    assert 'notification_unsubscription_accept' in response.scenario_analytics_info.objects


def _assert_subscription_refuse(response):
    assert response.intent in intent.NotificationsSubscribe
    assert 'notification_subscription_refuse' in response.scenario_analytics_info.objects


def _assert_unsubscription_refuse(response):
    assert response.intent in intent.NotificationsUnsubscribe
    assert 'notification_unsubscription_refuse' in response.scenario_analytics_info.objects


def _try_subscribe(alice, about=''):
    response = alice('подпишись на уведомления' + about)
    assert response.intent == intent.NotificationsSubscribe
    alice('да')


def _try_unsubscribe(alice, about=''):
    response = alice('я хочу отписаться от уведомлений' + about)
    assert response.intent == intent.NotificationsUnsubscribe
    if 'У вас нет активных подписок' in response.text or 'Вы не подписаны' in response.text:
        alice('не надо')
    else:
        alice('да')


def _unsubscribe_all(personal_uid):
    for subscription_id in subscriptions.all_subscriptions:
        notificator_client.unsubscribe(personal_uid, subscription_id)


About = [
    ' про новые функции',
    ' про музыкальные новинки',
    ' о новых эпизодах',
]


@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments(
    f'mm_enable_protocol_scenario={scenario.NotificationsManager}',
    'bg_fresh_granet',
)
class _TestNotifications(object):

    owners = ('tolyandex', 'karina-usm',)


@pytest.mark.oauth(auth.Yandex)
class TestSubscribe(_TestNotifications):

    commands = [
        'я хочу подписаться на уведомления',
        'включи оповещения',
    ]

    @pytest.mark.parametrize('command', commands)
    @pytest.mark.parametrize('about', About)
    def test_subscribe_accept(self, alice, command, about):
        _try_unsubscribe(alice, about)
        response = alice(command + about)
        assert response.intent == intent.NotificationsSubscribe
        response = alice('да')
        _assert_subscription_accept(response)

    @pytest.mark.parametrize('command', commands)
    def test_subscribe_general_accept(self, alice, command):
        about = ' про новые функции'
        _try_unsubscribe(alice, about)
        response = alice(command)
        assert response.intent == intent.NotificationsSubscribe
        response = alice('да')
        _assert_subscription_accept(response)

    @pytest.mark.parametrize('command', commands)
    @pytest.mark.parametrize('about', About)
    def test_subscribe_refuse(self, alice, command, about):
        _try_unsubscribe(alice, about)
        response = alice(command + about)
        assert response.intent == intent.NotificationsSubscribe
        response = alice('не надо')
        _assert_subscription_refuse(response)

    @pytest.mark.parametrize('command', commands)
    @pytest.mark.parametrize('about', About)
    def test_subscribe_already(self, alice, command, about):
        _try_subscribe(alice, about)
        response = alice(command + about)
        assert response.intent == intent.NotificationsSubscribe
        assert 'уже подписаны' in response.text


@pytest.mark.oauth(auth.Yandex)
class TestUnsubscribe(_TestNotifications):

    commands = [
        'я хочу отписаться от уведомлений',
        'как мне не получать уведомления',
    ]

    @pytest.mark.parametrize('command', commands)
    @pytest.mark.parametrize('about', About)
    def test_unsubscribe_accept(self, alice, command, about):
        _try_subscribe(alice, about)
        response = alice(command + about)
        assert response.intent == intent.NotificationsUnsubscribe
        response = alice('да')
        _assert_unsubscription_accept(response)

    @pytest.mark.parametrize('command', commands)
    @pytest.mark.parametrize('about', About)
    def test_unsubscribe_refuse(self, alice, command, about):
        _try_subscribe(alice, about)
        response = alice(command + about)
        assert response.intent == intent.NotificationsUnsubscribe
        response = alice('не надо')
        _assert_unsubscription_refuse(response)

    @pytest.mark.parametrize('command', commands)
    @pytest.mark.parametrize('about', About)
    def test_unsubscribe_already(self, alice, command, about):
        _try_unsubscribe(alice, about)
        response = alice(command + about)
        assert response.intent == intent.NotificationsUnsubscribe
        assert 'не подписаны' in response.text or 'У вас нет активных подписок' in response.text

    @pytest.mark.parametrize('command', commands)
    def test_unsubscribe_general(self, alice, command):
        response = alice(command)
        assert response.intent == intent.NotificationsUnsubscribe
        assert ' в приложении Яндекс' in response.text


@pytest.mark.oauth(auth.Yandex)
class TestSubscriptionsList(_TestNotifications):

    def test_subscriptions_list(self, alice):
        _unsubscribe_all(personal_uid=1083813279)
        response = alice('мои подписки')
        assert response.intent == intent.NotificationsSubscriptionsList
        assert 'У вас нет активных подписок.' in response.text

        notificator_client.subscribe(personal_uid=1083813279, subscription_id=subscriptions.MusicReleases)
        response = alice('мои подписки')
        assert response.intent == intent.NotificationsSubscriptionsList
        assert 'Вы подписаны на Больше музыки' in response.text


class TestReadNotifications(_TestNotifications):

    commands = [
        'что нового',
    ]

    @pytest.mark.oauth(auth.Yandex)
    @pytest.mark.parametrize('command', commands)
    def test_no_notifications(self, alice, command):
        response = alice(command)
        assert response.intent == intent.NotificationsRead
        assert response.text == 'У вас нет новых уведомлений.'

    # Keep YandexPlus subscribed. Yandex is used in subscribe/unsubscribe tests and is inconsistent.
    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('command', commands)
    def test_music_notification(self, alice, command):
        msg = notificator_client.parse_message(resource.find('music_push.txt'))
        notificator_client.send_message(msg)
        response = alice(command)
        assert response.intent == intent.NotificationsRead
        assert 'А как насчет новинок музыки?' in response.text
        response = alice('давай')
        assert response.directive.name == directives.names.MusicPlayDirective

        # check that a matching music event occured
        music_event = response.scenario_analytics_info.event('music_event')
        assert music_event
        assert music_event.answer_type == 'Album'
