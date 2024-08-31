import json
from distutils.util import strtobool
from tornado import gen
from tornado import web
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings, UnistatTiming, TimingsResolution
from alice.uniproxy.library.backends_asr import YaldiStream
from alice.uniproxy.library.common_handlers import CommonRequestHandler
from alice.uniproxy.library.common_handlers.utils import HandlerWithYandexHeaders
from alice.uniproxy.library.extlog import WebHandlerLogger
from alice.uniproxy.library.settings import config


# -------------------------------------------------------------------------------------------------
class YaldiStreamWrap(YaldiStream):
    def __on_result_callback(self, value):
        self.__partial_results.append(value)
        if value["endOfUtt"]:
            self.__fut.set_result(self.__partial_results)

    def __on_error_callback(self, error_str):
        if self._log_record:
            self._log_record['asr']['error'] = error_str
        self.__fut.set_exception(web.HTTPError(status_code=500, reason="Failed To Recognize"))

    def __init__(self, **kwargs):
        self.__partial_results = []
        self.__fut = gen.Future()
        self._log_record = kwargs.pop('log', None)
        super().__init__(
            callback=self.__on_result_callback,
            error_callback=self.__on_error_callback,
            **kwargs
        )

    async def get_results(self):
        return await self.__fut


# -------------------------------------------------------------------------------------------------
@web.stream_request_body
class PoastAsrHandlerBase(CommonRequestHandler, HandlerWithYandexHeaders):
    PAYLOAD_MAX_SIZE = 2 * (1 << 20)  # 2Mb
    ALLOW_UNAUTHORIZED = False
    UNISTAT_YALDI_METRICS = None
    UNISTAT_PAYLOAD_SIZE_METRIC = None
    UNISTAT_TIME_METRIC = None

    @staticmethod
    def format_helper(name):
        if name == "pcm":
            return "audio/x-pcm;bit=16;rate=16000"
        return name

    def prepare_quasar_topic(self):
        # pass quasar-general-gpu to hamster
        self.session_data["asr_balancer"] = "yaldi-hamster.alice.yandex.net"

        # enable eou
        self.session_data["AdvancedASROptions"] = {
            "early_eou_message": False,
            "partial_results": True,
            "disable_fallback": True,
            "enable_e2e_eou": True,
        }

    async def prepare(self):
        self.payload = bytearray()
        topic = self.get_argument("topic", "quasar-general")

        self.session_data = {
            "topic": topic,
            "format": self.format_helper(self.request.headers.get("Content-Type") or self.get_argument("format", "pcm")),
            "disableAntimatNormalizer": strtobool(self.get_argument("disable_antimat", "no"))
        }

        #   pass quasar-general-gpu to hamster
        if topic == "quasar-general-gpu":
            self.prepare_quasar_topic()

        self.__log_record = {
            "path": self.request.path,
            "query": self.request.query,
            "ip": self.get_client_ip(),
            "auth_token": self.get_client_auth_token(),
            "uuid": self.get_client_uuid(),
            "asr": {
                "topic": self.session_data["topic"],
                "format": self.session_data["format"],
                "disableAntimatNormalizer": self.session_data["disableAntimatNormalizer"]
            }
        }

        app_id = self.request.headers.get("X-Ya-App-Id")
        if app_id not in config.get("apikeys", {}).get("postasr", []):
            auth_res = await gen.multi({
                "auth_token": self.check_auth_token(),
                "service_ticket": self.check_service_ticket()
            })
            self.__log_record["auth"] = auth_res
            if not self.ALLOW_UNAUTHORIZED and not any(auth_res.values()):
                self.send_error(status_code=401, reason="Unauthorized")
                return

    def data_received(self, chunk):
        if (len(self.payload) + len(chunk)) > self.PAYLOAD_MAX_SIZE:
            # raising tornado.web.HTTPError doesn't work here by some reasons
            self.send_error(status_code=413, reason="Payload Too Large")
        else:
            self.payload += chunk

    def write_error(self, status_code, **kwargs):
        self.__log_record["status"] = status_code
        super().write_error(status_code, **kwargs)

    def on_finish(self):
        if self.UNISTAT_PAYLOAD_SIZE_METRIC is not None:
            GlobalTimings.store(self.UNISTAT_PAYLOAD_SIZE_METRIC, len(self.payload))
        self.__log_record["body_size"] = len(self.payload)
        self.__log_record.setdefault("status", 200)
        WebHandlerLogger()(self.__log_record)

    async def post(self):
        if self.UNISTAT_TIME_METRIC is not None:
            with UnistatTiming(self.UNISTAT_TIME_METRIC):
                return await self.post_impl()
        return await self.post_impl()

    async def post_impl(self):
        pretty = self.get_argument("pretty", "0")
        session_id = self.get_argument("session_id", "")
        message_id = self.get_argument("message_id", "")
        chunk_size = int(self.get_argument("chunk_size", "32000"))
        sleep_timeout = float(self.get_argument("sleep_timeout", "1.0"))

        asr = YaldiStreamWrap(
            params=self.session_data,
            session_id=session_id,
            message_id=message_id,
            unistat_counter=self.UNISTAT_YALDI_METRICS,
            log=self.__log_record,
        )

        data = bytes(self.payload)
        while data:
            chunk, data = data[:chunk_size], data[chunk_size:]
            asr.add_chunk(chunk)
            await gen.sleep(sleep_timeout)
        asr.add_chunk()

        try:
            results = await asr.get_results()
        except Exception as ex:
            self.set_status(500)
            self.write(str(ex))
            return

        if pretty == "3" and results:
            result = results[-1]
            if "recognition" in result:
                self.write(json.dumps([rec["normalized"] for rec in result["recognition"]], ensure_ascii=False))
            else:
                self.write(json.dumps(result, ensure_ascii=False))
            self.set_header('Content-Type', 'application/json')
        elif pretty == "2" and results:
            result = results[-1]
            if "recognition" in result:
                self.write(result["recognition"][0]["normalized"])
            else:
                self.write(json.dumps(result, ensure_ascii=False))
                self.set_header('Content-Type', 'application/json')
        elif pretty == "1" and results:
            self.write(results[-1]["recognition"][0])
        else:
            self.write(json.dumps(results, ensure_ascii=False))
            self.set_header('Content-Type', 'application/json')
        self.write("\r\n")


def create_post_asr_handler(name, allow_unathorized=False):
    GlobalTimings.register_counters(
        (f"handler_{name}_time", TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (f"handler_{name}_payload_size", TimingsResolution.AUDIO_SIZE_VALUES),
    )
    GlobalCounter.register_counter(
        f"handler_{name}_reqs_summ",
        f"handler_{name}_asr_2xx_ok_summ",
        f"handler_{name}_asr_4xx_err_summ",
        f"handler_{name}_asr_5xx_err_summ",
        f"handler_{name}_asr_other_err_summ",
    )

    class ActualHandler(PoastAsrHandlerBase):
        unistat_handler_name = name
        ALLOW_UNAUTHORIZED = allow_unathorized
        UNISTAT_PAYLOAD_SIZE_METRIC = f"handler_{name}_payload_size"
        UNISTAT_YALDI_METRICS = f"handler_{name}_asr"
        UNISTAT_TIME_METRIC = f"handler_{name}_time"

    return ActualHandler


# -------------------------------------------------------------------------------------------------
AsrHandler = create_post_asr_handler("asr")
