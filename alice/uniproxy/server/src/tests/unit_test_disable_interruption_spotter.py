from .mocks import tvm_client_mock, subway_client_mock, no_lass, WebsocketMock
from .mocks.tts_server import TtsServerMock
from .checks import match

import alice.uniproxy.library.testing
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.settings import config

import tornado.gen
import tornado.httpserver
import tornado.web
import logging
import json
import uuid
import pytest


logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)12s: %(name)-16s %(message)s')
GlobalCounter.init()


def bind_on_free_port(srv: tornado.httpserver.HTTPServer):
    for port in range(50000, 0x10000):
        try:
            srv.bind(port)
            return port
        except OSError:
            pass
    raise RuntimeError("HTTPServer couldn't find free TCP port")


class AnyHandler(tornado.web.RequestHandler):
    def post(self):
        request = json.loads(self.request.body.decode("utf-8"))
        request_text = request["request"]["event"].get("text", "")

        response = {
            "header": {
                "request_id": request["header"]["request_id"],
            }, "voice_response": {
                "output_speech": {
                    "text": "you said: {}".format(request_text)
                }
            }
        }
        if "force_interruption_spotter" in request_text:
            response["voice_response"].setdefault("directives", []).append({
                "type": "uniproxy_action",
                "name": "force_interruption_spotter"
            })
        self.finish(response)


class VinsServerMock:
    def __enter__(self, *args, **kwargs):
        self.start()
        self.__orig_url = config["vins"]["url"]
        config.set_by_path("vins.url", "http://localhost:{}".format(self.port))
        return self

    def __exit__(self, *args, **kwargs):
        self.stop()
        config.set_by_path("vins.url", self.__orig_url)

    def __init__(self):
        self._app = tornado.web.Application([(r'/.*', AnyHandler)], compress_response=True)
        self._srv = tornado.httpserver.HTTPServer(self._app)
        self.port = None

    def start(self):
        self.port = bind_on_free_port(self._srv)
        self._srv.start(1)

    def stop(self):
        self._srv.stop()


# ====================================================================================================================
def make_text_input_event(text, experiments=None):
    return {
        "header": {
            "namespace": "VINS",
            "name": "TextInput",
            "messageId": "ffffffffffffffff"
        },
        "payload": {
            "header": {
                "request_id": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            },
            "request": {
                "voice_session": True,
                "event": {
                    "type": "text_input",
                    "text": text
                },
                "experiments": experiments or {
                    # If this flag is set and voice response contains spotter words, then TTS.Speak directive
                    # must contain "disableInterruptionSpotter = True" (but some exceptions)
                    "disable_interruption_spotter": True  
                }
            },
            "key": "developers-simple-key",
            "disable_cache": True
        }
    }


# ====================================================================================================================
@pytest.mark.usefixtures("tvm_client_mock", "subway_client_mock", "no_lass")
@alice.uniproxy.library.testing.ioloop_run
def test_dont_disable_interruption_spotter():
    """ Interruption spotter must work if generated voice doesn't contain a spotter phrase
    (or the feature is disabled)
    """
    with VinsServerMock() as vins, TtsServerMock() as tts:
        ws = WebsocketMock()
        ws.send_event({"header": {"name": "SynchronizeState", "namespace": "System"}})
        ws.send_event(make_text_input_event(text="Какой-то текст без споттерных слов"))

        tts_directive = yield ws.pop_directive(namespace="TTS", name="Speak")
        assert tts_directive["payload"]["disableInterruptionSpotter"] is False

        # disable feature
        ws = WebsocketMock()
        ws.send_event({"header": {"name": "SynchronizeState", "namespace": "System"}})
        ws.send_event(make_text_input_event(
            text="Эй, яндекс и алиса!",
            experiments={"disable_interruption_spotter": False}
        ))

        tts_directive = yield ws.pop_directive(namespace="TTS", name="Speak")
        assert tts_directive["payload"]["disableInterruptionSpotter"] is False


# ====================================================================================================================
@pytest.mark.usefixtures("tvm_client_mock", "subway_client_mock", "no_lass")
@alice.uniproxy.library.testing.ioloop_run
def test_disable_interruption_spotter():
    """ Interruption spotter must be disabled if generated voice contains a spotter phrase
    """

    with VinsServerMock() as vins, TtsServerMock() as tts:
        ws = WebsocketMock()
        ws.send_event({"header": {"name": "SynchronizeState", "namespace": "System"}})
        ws.send_event(make_text_input_event(text="Эй, яндекс и алиса"))

        tts_directive = yield ws.pop_directive(namespace="TTS", name="Speak")
        assert tts_directive["payload"]["disableInterruptionSpotter"] is True


# ====================================================================================================================
@pytest.mark.usefixtures("tvm_client_mock", "subway_client_mock", "no_lass")
@alice.uniproxy.library.testing.ioloop_run
def test_dont_disable_interruption_spotter_for_yandex_novosti():
    """ Interruption spotter must always work for response from Yandex.Novosti
    """
    with VinsServerMock() as vins, TtsServerMock() as tts:
        ws = WebsocketMock()
        ws.send_event({"header": {"name": "SynchronizeState", "namespace": "System"}})
        ws.send_event(make_text_input_event(text="Эй, яндекс и алиса, это Яндекс Новости"))

        tts_directive = yield ws.pop_directive(namespace="TTS", name="Speak")
        assert tts_directive["payload"]["disableInterruptionSpotter"] is False


# ====================================================================================================================
@pytest.mark.usefixtures("tvm_client_mock", "subway_client_mock", "no_lass")
@alice.uniproxy.library.testing.ioloop_run
def test_dont_disable_interruption_spotter_if_vins_forces_it():
    """ Interruption spotter must always work if VINS forces it
    """
    with VinsServerMock() as vins, TtsServerMock() as tts:
        for text in (
            "Эй, яндекс и алиса, force_interruption_spotter, пожалуйста",  # with spotter phrase
            "Эй, force_interruption_spotter, пожалуйста"  # without spotter phrase
        ):
            ws = WebsocketMock()
            ws.send_event({"header": {"name": "SynchronizeState", "namespace": "System"}})
            ws.send_event(make_text_input_event(text=text))

            vins_response = yield ws.pop_directive(namespace="Vins", name="VinsResponse")
            assert match(vins_response, {"payload": {
                "voice_response": {
                    "directives": [{
                        "type": "uniproxy_action",
                        "name": "force_interruption_spotter"
                    }]
                }
            }})

            tts_directive = yield ws.pop_directive(namespace="TTS", name="Speak")
            assert tts_directive["payload"]["disableInterruptionSpotter"] is False
