import time

from alice.uniproxy.library.backends_common.apphost import AppHostHttpClient, AppHostGrpcClient, Request as AppHostRequest, ItemFormat
from alice.uniproxy.library.global_counter import GlobalCounter

from alice.cuttlefish.library.protos.audio_pb2 import TAudio
from alice.cuttlefish.library.protos.session_pb2 import TAudioOptions, TVoiceOptions, TRequestContext
from alice.cuttlefish.library.protos.tts_pb2 import TRequest as TTtsRequest, TTimings as TTtsTimings


GRPC_CHUNK_TIMEOUT = 3
REQUEST_TIMEOUT = 15


def _get_float_or_default(number, default):
    if number is None:
        return default

    try:
        return float(number)
    except:
        return default


def _get_str_or_default(st, default):
    if st is None:
        return default

    return st


def _get_protobuf_enum_from_str_or_default(st, enum_type, default):
    if st is None:
        return default

    try:
        return enum_type.Value(st.upper())
    except:
        return default


class AppHostedTtsClient:
    def __init__(
        self,
        system,
        on_audio_data,
        on_tts_timings,
        on_error,
        rtlog_token,
    ):
        self.system = system
        self.on_audio_data = on_audio_data
        self.on_tts_timings = on_tts_timings
        self.on_error = on_error
        self.rtlog_token = rtlog_token
        self.stream = None

    async def run(
        self,

        message_id,
        text,

        # Audio options
        audio_format=None,

        # Voice options
        volume=None,
        speed=None,
        lang=None,
        voice=None,
        emotion=None,
        quality=None,

        # TtsRequest options
        replace_shitova_with_shitova_gpu=False,
        need_tts_backend_timings=False,
        enable_tts_backend_timings=False,
        enable_get_from_cache=False,
        enable_cache_warm_up=False,
        enable_save_to_cache=False,

        use_grpc=False,
    ):
        GlobalCounter.U2_COUNT_APPHOSTED_TTS_SUMM.increment()

        request_context = TRequestContext(
            # Defaults from https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/backends_tts/ttsutils.py?rev=r8321793#L134-145

            AudioOptions=TAudioOptions(
                Format=_get_str_or_default(audio_format, "Opus")
            ),

            VoiceOptions=TVoiceOptions(
                Volume=_get_float_or_default(volume, 1.0),
                Speed=_get_float_or_default(speed, 1.0),
                Lang=_get_str_or_default(lang, "ru"),
                Voice=_get_str_or_default(voice, "shitova"),
                UnrestrictedEmotion=_get_str_or_default(emotion, "neutral"),
                Quality=_get_protobuf_enum_from_str_or_default(quality, TVoiceOptions.EVoiceQuality, TVoiceOptions.EVoiceQuality.ULTRAHIGH),
            ),
        )

        tts_request = TTtsRequest(
            Text=text,

            PartialNumber=0,
            RequestId=message_id,

            ReplaceShitovaWithShitovaGpu=replace_shitova_with_shitova_gpu,
            NeedTtsBackendTimings=need_tts_backend_timings,
            EnableTtsBackendTimings=enable_tts_backend_timings,
            EnableGetFromCache=enable_get_from_cache,
            EnableCacheWarmUp=enable_cache_warm_up,
            EnableSaveToCache=enable_save_to_cache,
        )

        try:
            if use_grpc:
                await self._run_grpc(message_id, request_context, tts_request)
            else:
                await self._run_http(message_id, request_context, tts_request)
        except Exception as exc:
            GlobalCounter.U2_FAILED_APPHOSTED_TTS_SUMM.increment()
            self.system.logger.log_directive({
                "ForEvent": message_id,
                "type": "AppHostedTtsFailed",
                "Body": {"reason": "AppHostedTts error: " + str(exc)},
            })
            self.on_error(str(exc))

    async def _run_http(self, message_id, request_context, tts_request):
        request = AppHostRequest(
            path="tts",
            items={
                "request_context": request_context,
                "tts_request": tts_request,
            },
        )

        if self.rtlog_token:
            request.add_params({"reqid": self.rtlog_token})

        ah_client = AppHostHttpClient()
        ah_resp = await ah_client.fetch(request, request_timeout=REQUEST_TIMEOUT)

        audio_items = [it for it in ah_resp.get_items(item_type="audio", proto_type=TAudio, item_format=ItemFormat.PROTOBUF)]
        tts_timings_items = [it for it in ah_resp.get_items(item_type="tts_timings", proto_type=TTtsTimings, item_format=ItemFormat.PROTOBUF)]
        if not audio_items:
            raise RuntimeError("No audio items")
        else:
            for item in tts_timings_items:
                for timings in item.data.Timings:
                    self.on_tts_timings(timings, item.data.IsFromCache)

            for item in audio_items:
                if item.data.HasField("Chunk"):
                    self.on_audio_data(data=item.data.Chunk.Data, last=False)
            self.on_audio_data(data=None, last=True)

            self.system.logger.log_directive({
                "ForEvent": message_id,
                "type": "AppHostedTtsSuccess",
                "Body": {"reason": "AppHostedTts success!"},
            })

    async def _run_grpc(self, message_id, request_context, tts_request):
        start_time = time.time()
        deadline = start_time + REQUEST_TIMEOUT

        got_start_of_stream = False
        got_end_of_stream = False

        client = AppHostGrpcClient()
        if self.rtlog_token:
            params = {"reqid": self.rtlog_token}
        else:
            params = None

        # Assume that GRPC_CHUNK_TIME <= REQUEST_TIMEOUT
        async with client.create_stream(path="tts", params=params, timeout=GRPC_CHUNK_TIMEOUT) as stream:
            items = {
                "request_context": [request_context],
                "tts_request": [tts_request],
            }

            stream.write_items(
                items=items,
                last=True,
            )

            while True:
                current_time = time.time()
                if current_time >= deadline:
                    stream.cancel()
                    raise RuntimeError("stream timeout")

                time_left = deadline - current_time
                next_chunk_timeout = min(time_left, GRPC_CHUNK_TIMEOUT)

                chunk = await stream.read(timeout=next_chunk_timeout)

                if chunk is None:
                    if not got_start_of_stream:
                        raise RuntimeError("Apphost stream finished before start of audio stream")
                    elif not got_end_of_stream:
                        raise RuntimeError("Apphost stream finished before end of audio stream")

                    self.system.logger.log_directive({
                        "ForEvent": message_id,
                        "type": "AppHostedTtsSuccess",
                        "Body": {"reason": "AppHostedTts success!"},
                    })

                    self.on_audio_data(data=None, last=True)
                    break
                elif isinstance(chunk, Exception):
                    stream.cancel()
                    raise chunk
                else:
                    for audio in chunk.get_item_datas(item_type="audio", proto_type=TAudio):
                        if audio.HasField("BeginStream"):
                            got_start_of_stream = True
                        elif audio.HasField("EndStream"):
                            got_end_of_stream = True
                        elif audio.HasField("Chunk"):
                            self.on_audio_data(data=audio.Chunk.Data, last=False)

                    for timings in chunk.get_item_datas(item_type="tts_timings", proto_type=TTtsTimings):
                        for subtimings in timings.Timings:
                            self.on_tts_timings(subtimings, timings.IsFromCache)
