from alice.uniproxy.library.extlog.sessionlog import ClientEvent, SessionLogger
import json
import tornado.gen

from alice.uniproxy.library.logging import Logger
import alice.uniproxy.library.testing
from alice.uniproxy.library.extlog.mocks import SessionLogMock


# VOICESERV-310
def test_oauth_token_not_logged():
    Logger.init('unittest', True)
    message = json.loads("""{"event":{"header":{"messageId":"84cab18c-aeff-4d4c-adc4-6f73e495e421","name":"VoiceInput","namespace":"Vins","streamId":1},"payload":{"advancedASROptions":{"partial_results":true},"application":{"client_time":"20170613T105610","lang":"ru-RU","timestamp":"1497340570","timezone":"Europe/Kiev"},"disableAntimatNormalizer":true,"format":"audio/opus","header":{"request_id":"c22eec0c-f004-4841-817b-64389c2cacbd"},"lang":"ru-RU","punctuation":true,"request":{"additional_options":{"bass_options":{"cookie":"123", "cookies":{"q":3},"filtration_level":1,"user_agent":"Mozilla/5.0 (Linux; Android 5.1.1; LG-H734 Build/LMY47V; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/58.0.3029.83 Mobile Safari/537.36 YandexSearch/6.50"},"oauth_token":"AQAAAAAJp9e3AAJ-IbQhG90b4UrHpucJonALIfg"},"event":{"type":"voice_input"},"experiments":[""],"location":{"accurancy":154.8270111083984,"lat":50.00822067260742,"lon":36.23822021484375,"recency":2684},"reset_session":true,"voice_session":true},"topic":"queries","vinsUrl":"https://vins-int.voicetech.yandex.net/speechkit/app/pa/"}}}""")  # noqa
    event = ClientEvent("test", None, message)
    event.save()
    oauth_token = event.message["event"]["payload"]["request"]["additional_options"]["oauth_token"]
    assert(oauth_token)
    assert(event.message["event"]["payload"]["request"]["additional_options"]["bass_options"]["cookie"] == "...")
    assert(event.message["event"]["payload"]["request"]["additional_options"]["bass_options"]["cookies"] == "...")
    assert(isinstance(oauth_token, str))
    obscured_part = oauth_token[len(oauth_token)//2:]
    assert(obscured_part.count("*") == len(obscured_part))
    ClientEvent("test", None, {"event": {"payload": None}})
    ClientEvent("test", None, {"event": {"payload": {"request": None}}})
    ClientEvent("test", None, {"event": {"payload": {"request": {"additional_options": None}}}})

    message["event"]["payload"]["request"]["additional_options"]["oauth_token"] = 2
    event = ClientEvent("test", None, message)
    event.save()
    oauth_token = event.message["event"]["payload"]["request"]["additional_options"]["oauth_token"]
    assert(oauth_token == 2)


# VOICESERV-1201
def test_http_authorization_not_logged():
    Logger.init('unittest', True)
    message = json.loads(
"""
{"event":{
        "header":{"messageId":"84cab18c-aeff-4d4c-adc4-6f73e495e421","name":"VoiceInput","namespace":"Vins","streamId":1},
        "payload":{
            "music_request2":{"headers":{"Authorization":"top_secret_password"}},
            "advancedASROptions":{"partial_results":true},
            "application":{"client_time":"20170613T105610","lang":"ru-RU","timestamp":"1497340570","timezone":"Europe/Kiev"},
            "disableAntimatNormalizer":true,
            "format":"audio/opus",
            "header":{"request_id":"c22eec0c-f004-4841-817b-64389c2cacbd"},
            "lang":"ru-RU","punctuation":true,
            "request":{
                "additional_options":{
                    "bass_options":{
                        "filtration_level":1,
                        "user_agent":"Mozilla/5.0 (Linux; Android 5.1.1; LG-H734 Build/LMY47V; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/58.0.3029.83 Mobile Safari/537.36 YandexSearch/6.50"
                    },
                    "oauth_token":"AQAAAAAJp9e3AAJ-IbQhG90b4UrHpucJonALIfg"
                },
                "event":{"type":"voice_input"},
                "experiments":[""],
                "location":{"accurancy":154.8270111083984,"lat":50.00822067260742,"lon":36.23822021484375,"recency":2684},
                "reset_session":true,
                "voice_session":true
            },
            "topic":"queries",
            "vinsUrl":"https://vins-int.voicetech.yandex.net/speechkit/app/pa/"
        }
}}""")  # noqa
    event = ClientEvent("test", None, message)
    orig_auth = event.message["event"]["payload"]["music_request2"]["headers"]["Authorization"]
    event.save()
    auth = event.message["event"]["payload"]["music_request2"]["headers"]["Authorization"]
    assert(auth)
    assert(isinstance(auth, str))
    obscured_part = auth[len(auth)//2:]
    orig_auth.startswith(obscured_part)
    assert(obscured_part.count("*") == len(obscured_part))
    ClientEvent("test", None, {"event": {"payload": None}})
    ClientEvent("test", None, {"event": {"payload": {"music_request2": None}}})
    ClientEvent("test", None, {"event": {"payload": {"music_request2": [1, 2]}}})
    ClientEvent("test", None, {"event": {"payload": {"music_request2": {"headers": None}}}})

    message["event"]["payload"]["music_request2"]["headers"]["Authorization"] = 2
    event = ClientEvent("test", None, message)
    event.save()
    auth = event.message["event"]["payload"]["music_request2"]["headers"]["Authorization"]
    assert(auth == 2)


@alice.uniproxy.library.testing.ioloop_run
def test_stream_without_spotter_phrase():
    Logger.init('unittest', True)
    with SessionLogMock() as log:
        logger = SessionLogger("test-session-id", rt_log=None)

        logger.start_stream("start-message-id", 1, save_to_mds=False)
        for i in range(0, 24):
            logger.log_data(1, b"y" * 512)
        logger.close_stream(1)
        logger.close()
        yield tornado.gen.sleep(0.2)

        assert len(log.records) == 3
        assert log.records[1]["Stream"]["isSpotter"] is False


@alice.uniproxy.library.testing.ioloop_run
def test_stream_with_spotter_phrase():
    Logger.init('unittest', True)
    with SessionLogMock() as log:
        logger = SessionLogger("test-session-id", rt_log=None)

        logger.start_stream("start-message-id", 1, save_to_mds=False)
        for i in range(0, 8):
            logger.log_data(1, b"x" * 512)
        logger.flush_stream(1)  # end of spotter phrase
        for i in range(0, 24):
            logger.log_data(1, b"y" * 512)
        logger.close_stream(1)
        logger.close()
        yield tornado.gen.sleep(0.2)

        assert len(log.records) == 4
        assert log.records[1]["Stream"]["isSpotter"] is True
        assert log.records[2]["Stream"]["isSpotter"] is False
