from alice.uniproxy.library.logging import Logger
Logger.init("uniproxy", is_debug=True)


from alice.uniproxy.library.vins import validate_vins_event
from alice.uniproxy.library.events import Event, EventException
from alice.uniproxy.library.testing.mocks import LogCollector
import pytest
import uuid


def create_vins_event(vins_url=None):
    event = Event({
        "header": {
            "namespace": "Vins",
            "name": "VoiceInput",
            "messageId": str(uuid.uuid4())
        },
        "payload": {}
    })
    if vins_url is not None:
        event.payload["vinsUrl"] = vins_url
    return event


def test_vins_url_validation__valid():
    lc = LogCollector(Logger._log)

    with lc:  # no exceptions here
        validate_vins_event(create_vins_event())
        validate_vins_event(create_vins_event(vins_url="http://custom.vins.yandex.ru/speechkit/app/something?some=parameters"))
        validate_vins_event(create_vins_event(vins_url="https://custom.megamind.yandex-team.ru:80/speechkit/app/xxx"))
        validate_vins_event(create_vins_event(vins_url="http://my.custom-url.yandex.net:444/speechkit?a=A&b=B&c=C"))


def test_vins_url_validation__invalid_hostname():
    lc = LogCollector(Logger._log)
    with lc:
        with pytest.raises(EventException):
            validate_vins_event(create_vins_event(vins_url="http://custom.vins.yandex.com/speechkit/app/something?some=parameters"))
        with pytest.raises(EventException):
            validate_vins_event(create_vins_event(vins_url="https://x-yandex-team.org/speechkit/"))
        with pytest.raises(EventException):
            validate_vins_event(create_vins_event(vins_url="http://192.168.1.1/speechkit/"))
        with pytest.raises(EventException):
            validate_vins_event(create_vins_event(vins_url="vins.alice.yandex.net/speechkit/app/quasar/"))


def test_vins_url_validation__invalid_path():
    lc = LogCollector(Logger._log)
    with lc:
        with pytest.raises(EventException):
            validate_vins_event(create_vins_event(vins_url="https://megamind.yandex.net/search/alice"))
        with pytest.raises(EventException):
            validate_vins_event(create_vins_event(vins_url="http://vins.alice.yandex.net/speechkit/app/quasar/shutdown?x=X"))
        with pytest.raises(EventException):
            validate_vins_event(create_vins_event(vins_url="https://megamind.alice.yandex.net/speechkit/app?shutdown=True"))
        with pytest.raises(EventException):
            validate_vins_event(create_vins_event(vins_url="http://vins.yandex.net/speechkit/xxx/quasar"))
