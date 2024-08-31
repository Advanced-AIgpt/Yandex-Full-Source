import logging
import time

from libcpp cimport bool
from libcpp.unordered_set cimport unordered_set
from util.datetime.base cimport TDuration
from util.generic.maybe cimport TMaybe
from util.generic.ptr cimport THolder
from util.generic.string cimport TString, TStringBuf
from util.generic.vector cimport TVector
from util.system.types cimport i64, ui64

from .event import ImageInput
from .serialization import serialize, deserialize


cdef extern from '<optional>' namespace 'std' nogil:
    cdef cppclass optional[T]:
        optional()
        optional(optional&) except +
        optional(T&) except +


cdef extern from 'library/cpp/logger/log.h' nogil:
    cdef cppclass TLog:
        TLog() except +
        TLog(const TString& fname, ELogPriority)


cdef extern from 'library/cpp/logger/priority.h' nogil:
    cdef enum ELogPriority:
        TLOG_DEBUG       # = 7


cdef extern from 'library/cpp/json/writer/json_value.h' namespace 'NJson':
    cdef cppclass TJsonValue:
        pass


cdef extern from 'library/cpp/json/json_reader.h' namespace 'NJson':
    cdef TJsonValue ReadJsonFastTree(TStringBuf, bool notClosedBracketIsError) except +


cdef extern from 'util/stream/input.h' nogil:
    cdef cppclass IInputStream:
        IInputStream()


cdef extern from 'util/stream/mem.h' nogil:
    cdef cppclass TMemoryInput(IInputStream):
        TMemoryInput()
        TMemoryInput(const void*, size_t)
        size_t Avail()


cdef extern from 'alice/uniproxy/mapper/library/flags/container.h' namespace 'NAlice::NUniproxy':
    ctypedef unordered_set[TString] TFlags

    cdef cppclass TFlagsContainer:
        TFlagsContainer()
        TFlagsContainer(const TFlags&)


cdef extern from 'alice/uniproxy/mapper/uniproxy_client/lib/uniproxy_client.h' namespace 'NAlice::NUniproxy':
    cdef cppclass TUniproxyClientParams:
        TUniproxyClientParams() except +
        TString AuthToken
        TString OAuthToken
        bool DisableLocalExperiments
        TJsonValue SyncStateExperiments
        TString VinsUrl
        TString ShootingSource

        TString UniproxyUrl
        i64 AsrChunkSize
        ui64 AsrChunkDelayMs
        bool DisableServerCertificateValidation
        TDuration ConnectTimeout
        TDuration SendTimeout
        TDuration ReceiveTimeout
        TFlagsContainer* FlagsContainer

        TJsonValue Application
        TString Uuid
        TString Language
        TString Timezone

        TJsonValue SupportedFeatures

        TLog* Logger

    cdef cppclass TExtraRequestParams:
        TExtraRequestParams()
        optional[TString] RequestId
        TJsonValue PayloadTemplate

    cdef cppclass TExtraVoiceRequestParams(TExtraRequestParams):
        TExtraVoiceRequestParams()
        optional[TString] AudioFormat

    cdef cppclass TExtraTtsGenerateParams:
        TExtraTtsGenerateParams()
        optional[TString] AudioFormat
        optional[TString] Voice
        optional[TString] Speed
        optional[TString] Emotion

    cdef enum EResponseType 'NAlice::NUniproxy::EResponseType':
        Asr 'NAlice::NUniproxy::EResponseType::Asr'
        Vins 'NAlice::NUniproxy::EResponseType::Vins'
        TtsStream 'NAlice::NUniproxy::EResponseType::TtsStream'

    cdef cppclass TResponse:
        EResponseType Type
        TString Data

    ctypedef TVector[TResponse] TResponses


cdef extern from 'alice/uniproxy/mapper/uniproxy_client/lib/async_uniproxy_client.h' namespace 'NAlice::NUniproxy':
    cdef cppclass TAsyncUniproxyClient:
        TAsyncUniproxyClient(const TUniproxyClientParams&) except +
        TResponses GenerateVoice(TStringBuf text, const TExtraTtsGenerateParams&)
        TResponses SendTextRequest(TStringBuf text, const TExtraRequestParams&, bool reloadTimestamp)
        TResponses SendServerAction(const TExtraRequestParams&, bool reloadTimestamp)
        TResponses SendImageInput(const TExtraRequestParams&, bool reloadTimestamp)
        TResponses SendVoiceRequest(TStringBuf topic, IInputStream&, const TExtraVoiceRequestParams&, bool reloadTimestamp, const TMaybe[size_t])


cdef TString _to_TString(s):
    if not s:
        return TString()

    assert isinstance(s, basestring)
    if isinstance(s, unicode):
        s = s.encode('UTF-8')
    return TString(<const char*>s, len(s))


cdef TJsonValue _to_TJsonValue(py_obj):
    return ReadJsonFastTree(_to_TString(serialize(py_obj)), <bool> True)


cdef _iter_response_by_type(const TResponses& responses, EResponseType response_type):
    return [responses[i].Data for i in range(responses.size()) if responses[i].Type == response_type]


cdef _get_response_by_type(const TResponses& responses, EResponseType response_type):
    for response in _iter_response_by_type(responses, response_type):
        return response


cdef class Uniclient:
    # _client must be the first on the member list
    # because of the order __Pyx_call_destructor in tp_dealloc
    cdef THolder[TAsyncUniproxyClient] _client
    cdef TString _asr_topic
    cdef TFlagsContainer _flags
    cdef TLog _log
    cdef bool _wait_save_vins_session_to_ydb

    def __init__(self, settings, application):
        self._asr_topic = _to_TString(settings.asr_topic)
        self._flags = TFlagsContainer({_to_TString('apphost_waitall_timeout5s')})
        self._log = TLog(_to_TString(settings.log_type), ELogPriority.TLOG_DEBUG)
        self._wait_save_vins_session_to_ydb = False

        cdef TUniproxyClientParams params
        params.Logger = &self._log
        params.UniproxyUrl = _to_TString(settings.uniproxy_url)
        params.DisableServerCertificateValidation = True
        params.AsrChunkSize = -1
        params.AsrChunkDelayMs = 0

        params.AuthToken = _to_TString(settings.auth_token)
        params.OAuthToken = _to_TString(settings.oauth_token)
        params.DisableLocalExperiments = False
        params.SyncStateExperiments = _to_TJsonValue(settings.experiments)
        params.VinsUrl = _to_TString(settings.vins_url)
        params.FlagsContainer = &self._flags
        params.ReceiveTimeout = TDuration.Seconds(15)
        params.ShootingSource = 'evo_tests'
        params.SupportedFeatures = _to_TJsonValue(settings.sync_state_supported_features)

        params.Application = _to_TJsonValue(application)
        params.Uuid = _to_TString(application.Uuid)
        params.Language = _to_TString(application.Lang)
        params.Timezone = _to_TString(application.Timezone)

        self._client = THolder[TAsyncUniproxyClient](new TAsyncUniproxyClient(params))

    def request(self, payload, request=None):
        logging.info(f'RequestId: {payload.header.RequestId}')
        logging.info(f'Setrace URL: https://setrace.yandex-team.ru/alice/sessions/{payload.header.RequestId}')
        logging.info(f'Send request: {request}')
        logging.info(f'Send payload: {serialize(payload)}')

        # Read more: ALICEINFRA-472
        if self._wait_save_vins_session_to_ydb:
            time.sleep(0.3)
        self._wait_save_vins_session_to_ydb = True

        responses = self._send_request(payload, request)
        tts_response = _get_response_by_type(responses, EResponseType.TtsStream)
        vins_response = deserialize(_get_response_by_type(responses, EResponseType.Vins))
        asr_responses = [deserialize(_) for _ in _iter_response_by_type(responses, EResponseType.Asr)]

        logging.info(f'Receive: {vins_response}')
        return vins_response, tts_response, asr_responses

    def generate_voice(self, text):
        cdef TExtraTtsGenerateParams params
        params.AudioFormat = optional[TString](_to_TString('audio/opus'))
        params.Voice = optional[TString](_to_TString('shitova.gpu'))
        responses = self._client.Get().GenerateVoice(_to_TString(text), params)
        return _get_response_by_type(responses, EResponseType.TtsStream)

    cdef TResponses _send_request(self, payload, request) except +:
        cdef TExtraRequestParams params
        params.RequestId = optional[TString](_to_TString(payload.header.RequestId))
        params.PayloadTemplate = _to_TJsonValue(payload)

        if not request:
            if isinstance(payload.request.get('event'), ImageInput):
                return self._send_image_input(params)
            else:
                return self._send_server_action(params)
        if isinstance(request, str):
            return self._send_text_request(params, request)
        if isinstance(request, bytes):
            return self._send_voice_request(params, request)

    cdef TResponses _send_server_action(self, const TExtraRequestParams& params):
        return self._client.Get().SendServerAction(params, False)

    cdef TResponses _send_image_input(self, const TExtraRequestParams& params):
        return self._client.Get().SendImageInput(params, False)

    cdef TResponses _send_text_request(self, const TExtraRequestParams& params, str request):
        return self._client.Get().SendTextRequest(_to_TString(request), params, False)

    cdef TResponses _send_voice_request(self, const TExtraRequestParams& params, bytes request):
        cdef TMemoryInput stream = TMemoryInput(<const char*>request, len(request))
        cdef TMaybe[size_t] size = TMaybe[size_t](stream.Avail())
        cdef TExtraVoiceRequestParams voice_params
        voice_params.RequestId = params.RequestId
        voice_params.PayloadTemplate = params.PayloadTemplate
        voice_params.AudioFormat = optional[TString](_to_TString('audio/opus'))
        return self._client.Get().SendVoiceRequest(self._asr_topic, stream, voice_params, False, size)
