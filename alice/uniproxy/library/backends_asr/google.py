"""Galdi is Google's Yaldi.

Docs: https://cloud.google.com/speech-to-text/docs/
APIs: https://cloud.google.com/speech-to-text/docs/reference/rpc/google.cloud.speech.v1p1beta1
Auth: https://google-auth.readthedocs.io/en/latest/user-guide.html#service-account-private-key-files
"""

import os
import time
import queue
import types
import weakref

from google.cloud import speech_v1p1beta1 as gspeech
import grpc
from grpc._channel import _handle_event, _EMPTY_FLAGS
from grpc._cython import cygrpc
import tornado.concurrent
import tornado.ioloop
import tornado.gen

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.global_counter import GlobalCounter


class GaldiIterator:
    def __init__(self, session_id):
        self._log = Logger.get('.galdiiter')
        self._buffer = queue.Queue()
        self._closed = False
        self._session_id = session_id
        self._timeout = config.get('google', {}).get('asr', {}).get('input_chunk_timeout', 3)

    def __iter__(self):
        return self

    def __next__(self):
        try:
            chunk = self._buffer.get_nowait()
        except queue.Empty:
            try:
                chunk = self._buffer.get(timeout=self._timeout)
            except queue.Empty:
                self._closed = True
                GlobalCounter.GALDI_CLIENT_TIMEOUT_SUMM.increment()
                self._log.error(self._session_id, 'failed to get next audio chunk')
                raise StopIteration

        if chunk is None:
            self._closed = True
            GlobalCounter.GALDI_CLIENT_OK_SUMM.increment()
            raise StopIteration
        return gspeech.types.StreamingRecognizeRequest(audio_content=chunk)

    def add_chunk(self, chunk=None):
        self._buffer.put_nowait(chunk)

    def close(self):
        if not self._closed:
            self.add_chunk()
            self._closed = True


class GaldiStream:
    def __init__(
            self,
            success_callback,
            error_callback,
            params,
            session_id,
            message_id,
            close_callback=None,
            host=None,  # TODO
            port=None,  # TODO?
            ioloop=None,
            unistat_counter=None,
            rt_log=None,  # TODO
            rt_log_label=None,  # TODO
            system=None,  # TODO
            context_futures=(),  # TODO
    ):
        self._log = Logger.get('.galdistream')
        self._on_success = weakref.WeakMethod(success_callback)
        self._on_error = weakref.WeakMethod(error_callback)
        self._on_close = weakref.WeakMethod(close_callback) if close_callback else None
        self._ioloop = ioloop or tornado.ioloop.IOLoop.current()
        self._session_id = '{}/{} galdi:'.format(session_id, message_id)

        self._galdi_iterator = GaldiIterator(self._session_id)
        self._google_auth_file = self._get_auth_file()
        self._google_client = gspeech.SpeechClient.from_service_account_file(self._google_auth_file)
        self._google_config = gspeech.types.RecognitionConfig(**self._get_recognition_config(params))
        self._google_streaming_config = gspeech.types.StreamingRecognitionConfig(
            config=self._google_config,
            single_utterance=True,
            interim_results=params.get('advancedASROptions', {}).get('partial_results', True),
        )
        self._google_timeout = config.get('google', {}).get('asr', {}).get('output_chunk_timeout', 5)

        self._ioloop.spawn_callback(self._process)

    @staticmethod
    def _convert_to_yaldi_response(response):
        result = response.results[0]
        return {
            'responseCode': 'OK',
            'recognition': [{
                'confidence': result.alternatives[0].confidence,
                'words': [{'confidence': w.confidence, 'value': w.word} for w in result.alternatives[0].words],
                'normalized': u''.join(r.alternatives[0].transcript for r in response.results),
            }],
            'messagesCount': 1,  # TODO: ???
            'endOfUtt': response.results[0].is_final,
            'bioResult': [],
        }

    @staticmethod
    def _get_auth_file():
        return os.environ.get(
            'GOOGLE_APPLICATION_CREDENTIALS',
            config.get('google', {}).get('asr', {}).get('auth_file', '/opt/google_asr_auth_file'),
        )

    @staticmethod
    def _get_recognition_config(params):
        if params.get('format') == 'audio/x-wav':
            # See also https://cloud.google.com/speech-to-text/docs/encoding
            encoding = gspeech.enums.RecognitionConfig.AudioEncoding.LINEAR16
        else:
            encoding = gspeech.enums.RecognitionConfig.AudioEncoding.OGG_OPUS

        return {
            'enable_automatic_punctuation': params.get('punctuation', False),
            'enable_word_confidence': True,
            'encoding': encoding,
            'language_code': params.get('lang', 'ru-RU'),
            'profanity_filter': not params.get('disableAntimatNormalizer', False),
            'sample_rate_hertz': int(params.get('sampleRate', 16000)),
        }

    @staticmethod
    def _patch_responses_to_futures(responses, ioloop=None):
        """Wrap response to future.

        Updated version of https://github.com/grpc/grpc/issues/7910
        """

        ioloop = ioloop or tornado.ioloop.IOLoop.current()

        def _process_future(self, state, future):
            if state.response is not None:
                response = state.response
                state.response = None
                ioloop.add_callback(future.set_result, response)
            elif cygrpc.OperationType.receive_message not in state.due:
                if state.code is grpc.StatusCode.OK:
                    ioloop.add_callback(future.set_exception, StopIteration())
                elif state.code is not None:
                    ioloop.add_callback(future.set_exception, self)

        def _tornado_event_handler(self, state, response_deserializer, future):
            def handle_event(event):
                with state.condition:
                    callbacks = _handle_event(event, state, response_deserializer)
                    state.condition.notify_all()
                    done = not state.due
                for callback in callbacks:
                    callback()
                _process_future(self, state, future)
                return done and state.fork_epoch >= cygrpc.get_fork_epoch()

            return handle_event

        def _next(self):
            if cygrpc.OperationType.receive_message in self._state.due:
                raise ValueError('Prior future was not resolved')

            with self._state.condition:
                if self._state.code is None:
                    future = tornado.concurrent.Future()
                    event_handler = _tornado_event_handler(
                        self,
                        self._state,
                        self._response_deserializer,
                        future,
                    )
                    self._call.operate(
                        (cygrpc.ReceiveMessageOperation(_EMPTY_FLAGS),),
                        event_handler,
                    )

                    self._state.due.add(cygrpc.OperationType.receive_message)
                    return future
                elif self._state.code is grpc.StatusCode.OK:
                    raise StopIteration
                else:
                    raise self

        responses._wrapped._next = types.MethodType(_next, responses._wrapped)
        return responses

    async def _process(self):
        GlobalCounter.GALDI_REQUEST_SUMM.increment()
        self._log.debug(self._session_id, 'processing')
        responses = self._google_client.streaming_recognize(self._google_streaming_config, self._galdi_iterator)
        try:
            for response_future in self._patch_responses_to_futures(responses, self._ioloop):
                try:
                    response = await tornado.gen.with_timeout(time.time() + self._google_timeout, response_future)
                except tornado.gen.TimeoutError:
                    # somehow google ends with unneeded future, that never finishes :(
                    GlobalCounter.GALDI_SERVER_TIMEOUT_SUMM.increment()
                    self._log.debug(self._session_id, 'timeout')
                except Exception as error:
                    GlobalCounter.GALDI_ERROR_SUMM.increment()
                    self._on_error() and self._on_error()(error)
                else:
                    self._log.debug(self._session_id, 'received {}'.format(type(response).__name__))
                    if not response.results:
                        continue
                    GlobalCounter.GALDI_OK_SUMM.increment()
                    yaldi_response = self._convert_to_yaldi_response(response)
                    self._on_success() and self._on_success()(yaldi_response)
        except Exception as error:
            GlobalCounter.GALDI_ERROR_SUMM.increment()
            self._on_error() and self._on_error()(error)

    def add_chunk(self, data=None):
        self._log.debug(self._session_id, 'got {} size chunk'.format('empty' if data is None else len(data)))
        self._galdi_iterator.add_chunk(data)

    def close(self):
        self._log.debug(self._session_id, 'closing')
        self._galdi_iterator.close()
        self._on_success = None
        self._on_error = None
        if self._on_close and self._on_close():
            call = self._on_close()
            self._on_close = None
            call()


if __name__ == '__main__':

    class TestClient:
        def __init__(self, id_):
            self._id = id_
            self._asr_stream = GaldiStream(self._on_success, self._on_error, {}, self._id, self._id)

        def _on_success(self, response):
            try:
                print('success', self._id, response)
            except Exception as exc:
                print('exc', self._id, exc)

        def _on_error(self, error):
            print('error', self._id, error)

        def add_data(self, data):
            import threading

            def add(data):
                while data:
                    self._asr_stream.add_chunk(data[:2048])
                    data = data[2048:]
                    time.sleep(self._id)
                self._asr_stream.add_chunk(None)

            thread = threading.Thread(target=add, args=(data,))
            thread.daemon = True
            thread.start()

    @tornado.gen.coroutine
    def tic(t):
        s = time.monotonic()
        while tornado.ioloop.IOLoop.current()._running:
            yield tornado.gen.sleep(t)
            if abs(round(time.monotonic() - s - t, 3)) > 0.005:
                print('!' * 20, 'PANIC', '!' * 20)
            s = time.monotonic()

    def main():
        import os.path
        import pprint

        GlobalCounter.init()

        test_data_dir = os.path.join(os.path.dirname(__file__), '..', 'tests', 'data')

        m1 = TestClient(0.1)
        m2 = TestClient(0.9)
        m1.add_data(open(os.path.join(test_data_dir, 'school.opus'), 'rb').read())
        m2.add_data(open(os.path.join(test_data_dir, 'bio0.opus'), 'rb').read())

        ioloop = tornado.ioloop.IOLoop.current()
        ioloop.call_later(0, lambda: tic(0.05))
        ioloop.call_later(20, lambda: ioloop.stop())
        ioloop.start()

        pprint.pprint(sorted([k, v] for k, v in GlobalCounter.get_metrics() if k.startswith('galdi_')))

    main()
