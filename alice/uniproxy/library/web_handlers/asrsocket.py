import json
import uuid
from functools import partial
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.backends_asr import YaldiStream
from alice.uniproxy.library.backends_common.protohelpers import proto_to_json
from alice.uniproxy.library.backends_common.apikey import check_key
from alice.uniproxy.library.common_handlers import CommonWebSocketHandler
from alice.uniproxy.library.common_handlers.utils import HandlerWithYandexHeaders
from alice.uniproxy.library.utils.wshelpers import default_init_request
from alice.uniproxy.library.extlog import WebHandlerLogger
import time


class AsrWebSocket(CommonWebSocketHandler, HandlerWithYandexHeaders):
    unistat_handler_name = 'asrsocket_ws'

    def open(self):
        self._logger = Logger.get('.asrwebsocker')
        self._logger.debug("Start new connection")
        self.lastChunkSent = False
        self.client_ip = self.get_client_ip()
        self.status = 200
        self.uuid = ""
        self.topic = ""
        self.lang = ""
        self.key = ""
        self.start_time = time.time()
        self.totalSent = 0
        self.stream = None
        self.finalize = False
        self.received = 0
        self.sent = 0
        self.session_id = str(uuid.uuid4())

    def get_compression_options(self):
        return {}

    def on_error(self, error):
        self.status = 500  # any error

        self._logger.error(error)
        try:
            self.write_message({
                "type": "Error",
                "data": {
                    "message": "Error: " + str(error)
                }
            })
        except Exception as err:
            self._logger.error(f"Failed to send error message: {err}")

        if self.stream:
            self.stream.close()

        try:
            self.close()
        except Exception as err:
            self._logger.error(f"Failed to close itself: {err}")

    def send_init_response(self, code, disregard_that=None):
        self.status = code
        self.write_message({
            'type': "InitResponse",
            'data':
            {
                'sessionId': self.session_id,
                'code': code
            }
        })

    def on_message(self, message):
        try:
            if not self.stream:
                msg = json.loads(message)
                self._logger.info(message)
                req = default_init_request(msg['data'])
                self.uuid = req.uuid
                self.topic = req.topic
                self.lang = req.lang
                self.key = req.apiKey
                self.stream = YaldiStream(
                    self.on_result_asr,
                    self.on_error_asr,
                    proto_to_json(req),
                    self.session_id,
                    str(uuid.uuid4()),
                    close_callback=self.on_close_asr,
                    unistat_counter='ws_asr'
                )
                check_key(
                    self.key,
                    self.client_ip,
                    partial(self.send_init_response, 200),
                    partial(self.send_init_response, 403),
                    None
                )
                return
            if message[:2] == '{"' and message[-1] == '}':
                try:
                    msg = json.loads(message).get('data', {})
                    if msg.get('command', '') == 'finish':
                        if not self.lastChunkSent:
                            self.stream.add_chunk()
                            self._logger.info("%s: Got finish command. Total sent: %s" % (self.session_id,
                                                                                          self.totalSent))
                            self.sent += 1
                        self.lastChunkSent = True
                    return
                except Exception as exp:
                    self._logger.debug(exp)
                    return
            if not self.lastChunkSent:
                self._logger.debug("Sending {0} bytes to backend.".format(len(message)))
                self.totalSent += len(message)
                self.stream.add_chunk(message)
                self.sent += 1
                if len(message) == 0:
                    self._logger.info("%s: Got last chunk. Total sent: %s" % (self.session_id, self.totalSent))
                    self.stream.add_chunk()
                    self.lastChunkSent = True
        except Exception as exp:
            self.on_error(exp)

    def on_result_asr(self, result):
        if result.get("earlyEndOfUtt", False):
            self._logger.debug("ASR got earlyEndOfUtt (ignore message)")
            return
        self.received += 1

        if len(result.get("recognition", [])) > 0:
            text = result["recognition"][0].get("normalized", "")
            uttr = result.get("endOfUtt", False)

            if uttr:
                self._logger.info(self.session_id + ': End of utterance')

            metainfo = result.get("metainfo", {})
            self.write_message(
                {
                    'type': "AddDataResponse",
                    'data': {
                        'text': text,
                        'uttr': uttr,
                        'merge': result.get("messagesCount"),
                        'close': self.lastChunkSent and uttr and self.received >= self.sent,
                        'metainfo': {
                            'minBeam': metainfo.get("minBeam"),
                            'maxBeam': metainfo.get("maxBeam"),
                            'topic': metainfo.get("topic"),
                            'lang': metainfo.get("lang"),
                            'version': metainfo.get("version"),
                            'load_timestamp': metainfo.get("load_timestamp"),
                        },
                        'biometry': [{
                            'class': bio.get("classname"),
                            'confidence': bio.get("confidence"),
                            'tag': bio.get("tag")
                        } for bio in result.get("bioResult", [])],
                        'words': [
                            {
                                'words': [{
                                    'confidence': word.get("confidence", 1.0),
                                    'value': word.get("value", text),
                                    'align': {
                                        'start': word.get("align_info", {}).get("start_time", 0),
                                        'end': word.get("align_info", {}).get("end_time", 0),
                                        'acoustic': word.get("align_info", {}).get("acoustic_score", 0),
                                        'graph': word.get("align_info", {}).get("graph_score", 0),
                                        'lm': word.get("align_info", {}).get("lm_score", 0),
                                        'total': word.get("align_info", {}).get("total_score", 0)
                                    }
                                } for word in recognition.get("words", [])],
                                'confidence': recognition.get("confidence", 1.0),
                                'start': recognition.get("align_info", {}).get("start_time", 0),
                                'end': recognition.get("align_info", {}).get("end_time", 0),
                                'acoustic': recognition.get("align_info", {}).get("acoustic_score", 0),
                                'graph': recognition.get("align_info", {}).get("graph_score", 0),
                                'lm': recognition.get("align_info", {}).get("lm_score", 0),
                                'total': recognition.get("align_info", {}).get("total_score", 0),
                                'normalized': recognition.get("normalized", text)
                            } for recognition in result.get("recognition", [])] if uttr else []
                    }})
            if self.finalize and uttr and self.received >= self.sent:
                self._logger.info(self.session_id + ': Recognition finished')
                self.stream.close()

    def on_error_asr(self, err):
        self.on_error(err)

    def on_close_asr(self):
        self.close()

    def on_close(self):
        self._logger.debug("WebSocket closed")
        Logger.access(
            "asrsocket.ws?key=%s&requestId=%s&uuid=%s&topic=%s&lang=%s" %
            (
                self.key,
                self.session_id,
                self.uuid,
                self.topic,
                self.lang
            ),
            self.status,
            self.client_ip,
            time.time() - self.start_time
        )

        WebHandlerLogger()({
            "path": self.request.path,
            "query": self.request.query,
            "ip": self.client_ip,
            "status": self.status,
            "key": self.key,
            "session_id": self.session_id,
            "uuid": self.uuid,
            "topic": self.topic,
            "lang": self.lang,
            "total_size": self.totalSent
        })

        if self.stream:
            self.stream.close()

    def check_origin(self, origin):
        return True
