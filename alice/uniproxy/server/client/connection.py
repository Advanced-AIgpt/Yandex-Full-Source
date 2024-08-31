from tornado.websocket import websocket_connect
from tornado.concurrent import run_on_executor
from tornado import gen
from tornado.queues import Queue
from tornado.ioloop import IOLoop
import tornado.httpserver
import functools
import datetime
import json
import uuid
import logging
import pyaudio
from collections import namedtuple
from concurrent.futures import ThreadPoolExecutor
import struct
import time
import sys


gLog = logging.getLogger("uniclient")
streamHandler = logging.StreamHandler(sys.stdout)
streamFormatter = logging.Formatter(u"%(asctime)s %(levelname)s - %(message)s")
streamHandler.setFormatter(streamFormatter)
gLog.addHandler(streamHandler)


class SoundSystem(object):
    _pyaudio = pyaudio.PyAudio()
    leftover = b""
    queue = Queue()
    _out_stream = _pyaudio.open(
        rate=48000,
        channels=1,
        output=True,
        format=pyaudio.get_format_from_width(2),
        # stream_callback=play_next,
        # start=False
    )

    @classmethod
    def stop_recording(cls):
        cls.recording = False

    @classmethod
    def play(cls, data):
        gLog.info("Size: %s" % (len(data)))
        # cls.queue.put_nowait(data)
        # cls._out_stream.start_stream()
        cls._out_stream.write(data)

    @classmethod
    def record(cls, chunk_size):
        cls.recording = True
        cls.input_stream = cls._pyaudio.open(
            rate=16000,
            channels=1,
            input=True,
            format=pyaudio.get_format_from_width(2),
        )
        while cls.recording:
            yield cls.input_stream.read(chunk_size)


class Session(object):
    _timeout = None
    socket = None
    key = None
    uuid = None
    url = None

    @classmethod
    def fuckit(cls, msg):
        gLog.error(msg)
        IOLoop.current().stop()

    @classmethod
    def clear_timeout(cls):
        IOLoop.current().remove_timeout(cls._timeout)
        cls._timeout = None

    @classmethod
    def set_timeout(cls, timeout):
        cls._timeout = IOLoop.current().call_later(timeout, functools.partial(cls.fuckit, "Timeout"))


def synchronize():
    Session.socket.write_message(json.dumps({
        "event": {
            "header": {
                "namespace": "System",
                "name": "SynchronizeState",
                "messageId": str(uuid.uuid4()),
            },
            "payload": {
                "auth_token": Session.key,
                "uuid": Session.uuid,
                "lang": "ru-RU",
                "voice": "levitan",
                "AdvancedASROptions": {
                    "partial_results": True,
                }
            }
        }
    }))


def start_recognition():
    synchronize()
    Session.socket.write_message(json.dumps({
        "event": {
            "header": {
                "namespace": "Vins",
                "name": "VoiceInput",
                "messageId": str(uuid.uuid4()),
                "streamId": 1,
            },
            "payload": {
                "topic": "general",
                "format": "audio/x-pcm;bit=16;rate=16000",
                "advancedASROptions": {
                    "utterance_silence": 120
                },
                "application": {
                    "app_id": "com.yandex.search",
                    "app_version": "1.2.3",
                    "os_version": "5.0",
                    "platform": "android",
                    "uuid": str(uuid.uuid4()),
                    "lang": "ru-RU",
                    "client_time": datetime.datetime.now().strftime("%Y%m%dT%H%M%S"),
                    "timezone": "Europe/Moscow",
                    "timestamp": "1486736347",
                },
                "header": {
                    "request_id": str(uuid.uuid4()),
                },
                "request": {
                    "event": {
                        "type": "voice_input",
                    },
                    "voice_session": True,
                }
            }
        }
    }))
    context = namedtuple("Context", ["ioloop", "executor"])
    context.ioloop = IOLoop.current()
    context.executor = ThreadPoolExecutor(max_workers=4)
    return recognize_stream(context)


def vins_request(request):
    synchronize()
    Session.socket.write_message(json.dumps({
        "event": {
            "header": {
                "namespace": "Vins",
                "name": "TextInput",
                "messageId": str(uuid.uuid4()),
            },
            "payload": {
                "request": {
                    "event": {
                        "type": "text_input",
                        "text": request,
                    },
                    "voice_session": True,
                },
                "application": {
                    "app_id": "com.yandex.search",
                    "app_version": "1.2.3",
                    "os_version": "5.0",
                    "platform": "android",
                    "uuid": str(uuid.uuid4()),
                    "lang": "ru-RU",
                    "client_time": datetime.datetime.now().strftime("%Y%m%dT%H%M%S"),
                    "timezone": "Europe/Moscow",
                    "timestamp": "1486736347",
                },
                "header": {
                    "request_id": str(uuid.uuid4()),
                },
            }
        }
    }))


@run_on_executor
def recognize_stream(self):
    for chunk in SoundSystem.record(10024):
        gLog.info("recording")
        prepend = struct.pack(">I", 1)
        Session.socket.write_message(prepend + chunk, binary=True)


def on_message(msg):
    gLog.info("message")
    try:
        if msg is None:
            gLog.error("Socket closed unexpected")
            return

        if len(msg) < 4:
            gLog.error("Something nasty")
            return
        elif isinstance(msg, bytes):
            SoundSystem.play(msg[4:])
        else:
            try:
                res = json.loads(msg)
                gLog.info(res)
                if "streamcontrol" in res:
                    if int(res.get("streamcontrol").get("streamId")) % 2 == 1:
                        SoundSystem.stop_recording()
                    else:
                        Session.socket.write_message(
                            json.dumps({
                                "event": {
                                    "header": {
                                        "namespace": "TTS",
                                        "name": "SpeechFinished",
                                        "messageId": str(uuid.uuid4())
                                    },
                                    "payload": {}
                                }
                            })
                        )
                        start_recognition()
                    gLog.info("Stream control")
                    return
                elif "directive" not in res:
                    gLog.error("Got not a directive: %s" % (msg,))
                    gLog.info(res)
                directive = res.get("directive").get("header", {}).get("namespace", "") + "." + \
                    res.get("directive").get("header", {}).get("name", "")
                if directive == "TTS.Speak":
                    Session.socket.write_message(json.dumps({
                        "event": {
                            "header": {
                                "namespace": "TTS",
                                "name": "SpeechStarted",
                                "messageId": str(uuid.uuid4())
                            },
                            "payload": {}
                        }
                    }))
                elif directive == "ASR.Result":
                    #res = res.get("directive").get("payload").get("recognition", [{}])[0].get("normalized", "")
                    gLog.info(res)
                    """Session.socket.write_message(json.dumps({
                        "event": {
                            "header": {
                                "namespace": "TTS",
                                "name": "Generate",
                                "messageId": str(uuid.uuid4()),
                            },
                            "payload": {
                                "text": res,
                                "voice": "omazh",
                                "application": "test",
                                "format": "Pcm",
                                "quality": "UltraHigh",
                                "key": Session.key,
                                "lang": "ru-RU",
                            }
                        }
                    }))"""
                return
            except Exception as e:
                gLog.error("Exception in test: %s" % (e))
    except Exception as exc:
        gLog.error("Exception: %s" % (exc,))
        IOLoop.current().stop()


def connect(on_connect, on_message):
    def on_connect_save_socket(res):
        try:
            if res.exception():
                Session.fuckit("No connection to %s" % (Session.url,))
                return
            Session.socket = res.result()
        except Exception as exc:
            Session.fuckit("Exception on connection: %s" % (exc,))
            return
        gLog.info("on_connect")
        on_connect()

    websocket_connect(Session.url, callback=on_connect_save_socket, on_message_callback=on_message)
    IOLoop.current().start()


def run(**kwargs):
    Session.key = kwargs.get("key")
    Session.uuid = kwargs.get("uuid")
    Session.url = kwargs.get("url")
    gLog.info("Start")
    if kwargs.get("vins"):
        connect(functools.partial(vins_request, kwargs.get("vins")), on_message)
    else:
        connect(start_recognition, on_message)
    gLog.info("End")
