from alice.cuttlefish.library.protos.audio_pb2 import TAudio, TAudioChunk, TBeginStream, TEndStream
from alice.cuttlefish.library.protos.session_pb2 import TVoiceOptions, TRequestContext
from alice.cuttlefish.library.protos.tts_pb2 import TRequest as TTtsRequest, TTimings as TTtsTimings
from alice.cuttlefish.library.python.apphost_message.packing import extract_protobuf, extract_json, pack_protobuf
from alice.uniproxy.library.backends_tts.apphosted_tts_client import AppHostedTtsClient
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter
from voicetech.library.proto_api.ttsbackend_pb2 import GenerateResponse as TtsGenerateResponse
from apphost.lib.grpc.protos.service_pb2 import TServiceRequest, TServiceResponse

import common
import logging
import tornado.concurrent


COMPARISON_EPS = 10**-9


def get_metrics_dict():
    metrics = GlobalCounter.get_metrics()
    metrics_dict = {}
    for it in metrics:
        metrics_dict[it[0]] = it[1]
    return metrics_dict


def clean_metrics():
    UniproxyCounter.init()
    for key in set(get_metrics_dict()):
        getattr(GlobalCounter, key.upper()).set(0)


class FakeLogger:
    def __init__(self):
        self._logs = []

    def log_directive(self, directive):
        self._logs.append(directive)


class FakeSystem:
    def __init__(self):
        self.puid = "some-puid"
        self.app_id = "some-app-id"
        self.logger = FakeLogger()


async def _test_apphosted_tts_ok(params_type):
    clean_metrics()

    result_audio = []
    result_tts_timings = []
    errors = []

    if params_type == "default":
        # Audio options
        audio_format, audio_format_expected = None, "Opus"

        # Voice options
        volume, volume_expected = None, 1.0
        speed, speed_expected = None, 1.0
        lang, lang_expected = None, "ru"
        voice, voice_expected = None, "shitova"
        emotion, emotion_expected = None, "neutral"
        quality, quality_expected = None, TVoiceOptions.EVoiceQuality.ULTRAHIGH
    elif params_type == "correct":
        # Audio options
        audio_format, audio_format_expected = "random", "random"

        # Voice options
        volume, volume_expected = 1.5, 1.5
        speed, speed_expected = 1.5, 1.5
        lang, lang_expected = "en", "en"
        voice, voice_expected = "shitova.gpu", "shitova.gpu"
        emotion, emotion_expected = "eViL", "eViL"
        quality, quality_expected = "lOw", TVoiceOptions.EVoiceQuality.LOW
    elif params_type == "incorrect":
        # Audio options
        audio_format, audio_format_expected = None, "Opus"

        # Voice options
        volume, volume_expected = "lul", 1.0
        speed, speed_expected = "qwer", 1.0
        lang, lang_expected = None, "ru"
        voice, voice_expected = None, "shitova"
        emotion, emotion_expected = "qwer", "qwer"
        quality, quality_expected = "rewq", TVoiceOptions.EVoiceQuality.ULTRAHIGH
    else:
        assert False, f"unknown params_type={params_type}"

    def on_audio_data(data, last):
        result_audio.append([data, last])

    def on_tts_timings(timings, from_cache):
        result_tts_timings.append((timings, from_cache))

    def on_error(error):
        errors.append(error)

    apphosted_tts_client = AppHostedTtsClient(
        system=FakeSystem(),
        on_audio_data=on_audio_data,
        on_tts_timings=on_tts_timings,
        on_error=on_error,
        rtlog_token="rtlog_token",
    )

    loop = tornado.ioloop.IOLoop.current()
    future = tornado.concurrent.Future()

    async def run():
        try:
            await apphosted_tts_client.run(
                message_id="some_message_id",
                text="some text",

                # Audio options
                audio_format=audio_format,

                # Voice options
                volume=volume,
                speed=speed,
                lang=lang,
                voice=voice,
                emotion=emotion,
                quality=quality,

                # TtsRequest options
                replace_shitova_with_shitova_gpu=True,
                need_tts_backend_timings=True,
                enable_tts_backend_timings=True,
                enable_get_from_cache=True,
                enable_cache_warm_up=True,
                enable_save_to_cache=True,
            )
            future.set_result(None)
        except Exception as exc:
            future.set_exception(exc)

    try:
        with common.QueuedTcpServer() as srv, common.ConfigPatch({"apphost": {"url": f"http://localhost:{srv.port}"}}):
            loop.add_callback(run)

            stream = await srv.pop_stream()
            request = await common.read_http_request(stream, with_body=True)

            # check request
            assert request.method == "POST"
            assert request.path == "/tts"

            sr = TServiceRequest()
            sr.MergeFromString(request.body)

            ans = sr.Answers
            assert len(ans) == 3

            ans.sort(key=lambda x: x.Type)

            assert ans[0].Type == "app_host_params"
            app_host_params = extract_json(ans[0].Data)
            assert app_host_params == {"reqid": "rtlog_token"}

            assert ans[1].Type == "request_context"
            rc = extract_protobuf(ans[1].Data, TRequestContext)
            assert rc.AudioOptions.Format == audio_format_expected
            assert abs(rc.VoiceOptions.Volume - volume_expected) < COMPARISON_EPS
            assert abs(rc.VoiceOptions.Speed - speed_expected) < COMPARISON_EPS
            assert rc.VoiceOptions.Lang == lang_expected
            assert rc.VoiceOptions.Voice == voice_expected
            assert rc.VoiceOptions.UnrestrictedEmotion == emotion_expected
            assert rc.VoiceOptions.Quality == quality_expected

            assert ans[2].Type == "tts_request"
            tts_req = extract_protobuf(ans[2].Data, TTtsRequest)
            assert tts_req.Text == "some text"
            assert tts_req.PartialNumber == 0
            assert tts_req.RequestId == "some_message_id"
            assert tts_req.ReplaceShitovaWithShitovaGpu
            assert tts_req.NeedTtsBackendTimings
            assert tts_req.EnableTtsBackendTimings
            assert tts_req.EnableGetFromCache
            assert tts_req.EnableCacheWarmUp
            assert tts_req.EnableSaveToCache

            rsp_items = [
                (
                    "audio",
                    TAudio(
                        BeginStream=TBeginStream(
                            Mime="audio/opus",
                        ),
                    ),
                ),
                (
                    "audio",
                    TAudio(
                        Chunk=TAudioChunk(
                            Data=b"data1",
                        ),
                    ),
                ),
                (
                    "tts_timings",
                    TTtsTimings(
                        Timings=[
                            TtsGenerateResponse.Timings(
                                timings=[
                                    TtsGenerateResponse.Timings.Timing(
                                        time=500 * j,
                                        phoneme="phoneme_{}".format(j),
                                    )
                                    for j in range(2)
                                ]
                            )
                            for i in range(3)
                        ],
                        IsFromCache=True,
                    ),
                ),
                (
                    "audio",
                    TAudio(
                        Chunk=TAudioChunk(
                            Data=b"data2",
                        ),
                    ),
                ),
                (
                    "tts_timings",
                    TTtsTimings(
                        Timings=[
                            TtsGenerateResponse.Timings(
                                timings=[
                                    TtsGenerateResponse.Timings.Timing(
                                        time=1500,
                                        phoneme="phoneme",
                                    )
                                ]
                            )
                        ],
                        IsFromCache=False,
                    ),
                ),
                (
                    "audio",
                    TAudio(
                        EndStream=TEndStream(),
                    ),
                ),
            ]

            sr = TServiceResponse()
            for item_type, item in rsp_items:
                answer = sr.Answers.add()
                answer.Type = item_type
                answer.Data = pack_protobuf(item)
            sr_str = sr.SerializeToString()

            await stream.write(
                b"HTTP/1.1 200 OK\r\n"
                b"Content-Length: " + str(len(sr_str)).encode() + b"\r\n"
                b"Connection: Keep-Alive\r\n"
                b"\r\n" +
                sr_str
            )

            await future

            assert errors == []
            assert result_tts_timings == [
                (
                    TtsGenerateResponse.Timings(
                        timings=[
                            TtsGenerateResponse.Timings.Timing(
                                time=500 * j,
                                phoneme="phoneme_{}".format(j),
                            )
                            for j in range(2)
                        ]
                    ),
                    True,
                ) for i in range(3)
            ] + [
                (
                    TtsGenerateResponse.Timings(
                        timings=[
                            TtsGenerateResponse.Timings.Timing(
                                time=1500,
                                phoneme="phoneme",
                            )
                        ]
                    ),
                    False,
                )
            ]
            assert result_audio == [
                [b"data1", False],
                [b"data2", False],
                [None, True],
            ]
    except Exception as e:
        logging.exception("server failed")
        assert False, f"Exception occurred: {str(e)}"


@common.run_async
async def test_apphosted_tts_ok_default_params():
    await _test_apphosted_tts_ok(params_type="default")


@common.run_async
async def test_apphosted_tts_ok_correct_params():
    await _test_apphosted_tts_ok(params_type="correct")


@common.run_async
async def test_apphosted_tts_ok_incorrect_params():
    await _test_apphosted_tts_ok(params_type="incorrect")


@common.run_async
async def test_apphosted_tts_not_ok():
    clean_metrics()

    result_audio = []
    result_tts_timings = []
    errors = []

    def on_audio_data(data, last):
        result_audio.append([data, last])

    def on_tts_timings(timings, from_cache):
        result_tts_timings.append((timings, from_cache))

    def on_error(error):
        errors.append(error)

    apphosted_tts_client = AppHostedTtsClient(
        system=FakeSystem(),
        on_audio_data=on_audio_data,
        on_tts_timings=on_tts_timings,
        on_error=on_error,
        rtlog_token="rtlog_token",
    )

    loop = tornado.ioloop.IOLoop.current()
    future = tornado.concurrent.Future()

    async def run():
        try:
            await apphosted_tts_client.run(message_id="some_message_id", text="some text")
            future.set_result(None)
        except Exception as exc:
            future.set_exception(exc)

    with common.ConfigPatch({"apphost": {"url": "bad_url"}}):
        loop.add_callback(run)

        await future
        assert errors == ["HTTPError(code=599, body=connection failed [SCE]: Stream is closed ([Errno 111] Connection refused))"]
        assert result_audio == []
        assert result_tts_timings == []
