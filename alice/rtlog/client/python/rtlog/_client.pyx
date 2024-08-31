from util.datetime.base cimport TDuration
from util.generic.maybe cimport TMaybe
from util.generic.string cimport TString
from util.system.types cimport ui32, ui64
from traceback import format_exception
import library.cpp.eventlog.proto.events_extension_pb2 as eventlog
import datetime
import sys
import json
from six import PY3
from libcpp cimport bool
from libcpp.memory cimport shared_ptr
from google.protobuf.message import Message
from google.protobuf.descriptor import FieldDescriptor
from time import time


cdef ui32 EXCEPTION_BACKTRACE_LIMIT = 20
cdef bool py3 = PY3


cdef extern from "util/charset/utf8.h" nogil:
    cdef enum RECODE_RESULT:
        RECODE_OK,
        RECODE_EOINPUT,
        RECODE_EOOUTPUT,
        RECODE_BROKENSYMBOL,
        RECODE_ERROR

    cdef RECODE_RESULT GetUTF8CharLen(size_t& n, const unsigned char* p, const unsigned char* e)

    cdef bool IsUTF8ContinuationByte(unsigned char c)

cdef extern from "google/protobuf/map.h" namespace "google::protobuf" nogil:
    cdef cppclass Map[Key, T]:
        T& operator[](const Key& k)

cdef extern from "util/string/printf.h" nogil:
    TString Sprintf(const char* c, ...)

cdef extern from "alice/rtlog/protos/rtlog.ev.pb.h" namespace "NRTLogEvents" nogil:
    ctypedef enum ESeverity 'NRTLogEvents::ESeverity':
        RTLOG_SEVERITY_INFO 'NRTLogEvents::RTLOG_SEVERITY_INFO',
        RTLOG_SEVERITY_ERROR 'NRTLogEvents::RTLOG_SEVERITY_ERROR'

    cdef cppclass LogEvent:
        LogEvent()
        void SetSeverity(ESeverity value)
        void SetMessage(const TString& value)
        void SetBacktrace(const TString& value)
        inline Map[TString, TString]* MutableFields()

    cdef cppclass CreateRequestContext:
        CreateRequestContext()
        inline Map[TString, TString]* MutableFields()

cdef extern from "library/cpp/eventlog/eventlog.h" nogil:
    ctypedef ui64 TEventTimestamp

cdef extern from "alice/rtlog/client/client.h" namespace "NRTLog" nogil:
    cdef struct TClientStats:
        ui64 ActiveLoggersCount
        ui64 EventsCount
        ui64 PendingBytesCount
        ui64 WrittenFramesCount
        ui64 WrittenBytesCount
        ui64 ErrorsCount

    cdef cppclass TRequestLogger:
        void LogEvent(TEventTimestamp timestamp, ui64 eventId, const TString& bytes)
        void LogEvent[T](TEventTimestamp timestamp, const T& ev)
        TString LogChildActivationStarted(TEventTimestamp timestamp, bool newReqid, const TString& description)
        void LogChildActivationFinished(TEventTimestamp timestamp, const TString& token, bool ok)
        TString GetToken() const
        void Finish(TEventTimestamp timestamp)
    ctypedef shared_ptr[TRequestLogger] TRequestLoggerPtr

    cdef struct TClientOptions:
        bool Async
        TMaybe[TDuration] FlushPeriod
        TMaybe[TDuration] FileStatCheckPeriod
        TMaybe[ui64] FlushSize

    cdef cppclass TClient:
        TClient(const TString& fileName, const TString& serviceName, const TClientOptions& options)
        TRequestLoggerPtr CreateRequestLogger(const TString& token, bool session, TEventTimestamp timestamp)
        TClientStats GetStats() const


cdef size_t _valid_utf8_size(const TString& s, size_t size):
    if size == 0:
        return 0
    cdef const unsigned char* start = <const unsigned char*>(s.data())
    cdef const unsigned char* finish = start + size
    cdef const unsigned char* p = finish - 1
    if p != start and IsUTF8ContinuationByte(p[0]):
        p -= 1
    if p != start and IsUTF8ContinuationByte(p[0]):
        p -= 1
    if p != start and IsUTF8ContinuationByte(p[0]):
        p -= 1
    cdef size_t rune_size = 0
    GetUTF8CharLen(rune_size, p, finish)
    return p - start + rune_size


cdef void _shrink(TString& s, size_t maxSize):
    cdef TString comment = Sprintf('... (%lu)', s.size());
    cdef newSize = maxSize - comment.size() if comment.size() < maxSize else 0
    newSize = _valid_utf8_size(s, newSize)
    _instance.shrinked_bytes_count += s.size() - newSize
    s.resize(newSize)
    s.append(comment)


cdef TString _to_string(s):
    if s is None:
        return TString()
    cdef type s_type = type(s)
    cdef unicode u

    if s_type is bytes or (not py3) and s_type is str:
        return s
    elif py3 and s_type is str:
        return (<str>s).encode('utf8')
    else:
        u = (<unicode>s) if s_type is unicode else unicode(s)
        return u.encode('utf8')


cdef TString _to_string_with_limit(s):
    cdef TString result = _to_string(s)
    if result.size() > _instance.max_string_size:
        _shrink(result, _instance.max_string_size)
    return result


cdef TString _get_str_value(o, name):
    return _to_string(o.get(name))


cdef TEventTimestamp python_timestamp():
    cdef double result = time()
    result *= 1000000
    return <TEventTimestamp>result


def _format_exception_info(exception_info):
    etype, value, tb = exception_info
    return ''.join(format_exception(etype, value, tb, EXCEPTION_BACKTRACE_LIMIT))


cdef class RequestLogger:
    cdef TRequestLoggerPtr _request_logger
    cdef bool _session
    cdef RequestLogger _parent

    def __call__(self, _message, **kwargs):
        if not self._request_logger:
            return

        self._log_event(python_timestamp(), RTLOG_SEVERITY_INFO, _message, kwargs)

    def create_request_logger(self, description):
        if not self._request_logger:
            return _null_logger

        cdef TEventTimestamp ts = python_timestamp()
        if not self._session:
            raise ValueError('can be called for session request logger only')
        cdef TString token = self._request_logger.get().LogChildActivationStarted(ts,
                                                                                  True,
                                                                                  _to_string_with_limit(description))
        cdef RequestLogger result = do_begin_request(token, False, ts)
        result._parent = self
        return result

    def exception(self, _message, **kwargs):
        if not self._request_logger:
            return

        kwargs['exc_info'] = 1
        self._log_event(python_timestamp(), RTLOG_SEVERITY_ERROR, _message, kwargs)

    def exception_info(self, _exc_info, _message, **kwargs):
        if not self._request_logger:
            return

        kwargs['exc_info'] = _exc_info
        self._log_event(python_timestamp(), RTLOG_SEVERITY_ERROR, _message, kwargs)

    def error(self, _message, **kwargs):
        if not self._request_logger:
            return

        self._log_event(python_timestamp(), RTLOG_SEVERITY_ERROR, _message, kwargs)

    def log_event(self, _ts, _severity, _message, **kwargs):
        if not self._request_logger:
            return

        self._log_event(_ts * 1000000, RTLOG_SEVERITY_ERROR if _severity == 'error' else RTLOG_SEVERITY_INFO, _message, kwargs)

    def log_proto_event(self, _ev, **kwargs):
        if not self._request_logger:
            return

        cdef TEventTimestamp ts = python_timestamp()
        if not isinstance(_ev, Message):
            _ev = _ev()
            for k in kwargs:
                v = kwargs[k]
                if v is not None:
                    field_type = _ev.DESCRIPTOR.fields_by_name[k].type
                    if field_type == FieldDescriptor.TYPE_STRING:
                        v = _to_string_with_limit(v)
                    setattr(_ev, k, v)

        cdef ui64 eventId = _ev.DESCRIPTOR.GetOptions().Extensions[eventlog.message_id]
        self._request_logger.get().LogEvent(ts, eventId, _ev.SerializeToString())

    def log_child_activation_started(self, description, new_reqid=False):
        if not self._request_logger:
            return None

        return self._request_logger.get().LogChildActivationStarted(python_timestamp(),
                                                                    new_reqid,
                                                                    _to_string_with_limit(description))

    def log_child_activation_finished(self, token, ok):
        if not self._request_logger:
            return

        self._request_logger.get().LogChildActivationFinished(python_timestamp(), token, ok)

    def log_request_context(self, **kwargs):
        if not self._request_logger:
            return

        cdef TEventTimestamp ts = python_timestamp()
        cdef CreateRequestContext ev;
        for k in kwargs:
            v = kwargs[k]
            if v is not None:
                ev.MutableFields()[0][_to_string_with_limit(k)] = _to_string_with_limit(v)
        self._request_logger.get().LogEvent(ts, ev)

    def get_token(self):
        if not self._request_logger:
            return None

        return self._request_logger.get().GetToken()

    def end_request(self):
        if not self._request_logger:
            return

        cdef TEventTimestamp ts = python_timestamp()
        if self._parent:
            self._parent._request_logger.get().LogChildActivationFinished(ts,
                                                                          self._request_logger.get().GetToken(),
                                                                          True)
        self._request_logger.get().Finish(ts)

    cdef _log_event(self, ui64 _ts, ESeverity _severity, _message, dict kwargs):
        cdef LogEvent ev;
        ev.SetSeverity(_severity)
        ev.SetMessage(_to_string_with_limit(_message))

        exc_info = kwargs.get('exc_info', None)
        if exc_info is not None:
            if isinstance(exc_info, BaseException):
                exc_info = (type(exc_info), exc_info, exc_info.__traceback__) if PY3 else 1
            if not isinstance(exc_info, tuple):
                exc_info = sys.exc_info()
            ev.SetBacktrace(_to_string_with_limit(_format_exception_info(exc_info)))
            del kwargs['exc_info']

        for k in kwargs:
            v = kwargs[k]
            if v is not None:
                if isinstance(v, AsJson):
                    ev.MutableFields()[0][_to_string_with_limit(k + '__format')] = 'json'
                    v = (<AsJson>v).value
                    if isinstance(v, dict) or isinstance(v, list):
                        v = json.dumps(v, ensure_ascii=False)
                ev.MutableFields()[0][_to_string_with_limit(k)] = _to_string_with_limit(v)
        self._request_logger.get().LogEvent(_ts, ev)

    @staticmethod
    cdef RequestLogger wrap(const TRequestLoggerPtr& request_logger):
        cdef RequestLogger result = RequestLogger.__new__(RequestLogger)
        result._request_logger = request_logger
        return result


cdef class ClientHolder:
    cdef TClient* client
    cdef ui32 max_string_size
    cdef ui64 shrinked_bytes_count

    def __dealloc__(self):
        del self.client


cdef ClientHolder _instance = None
cdef object _is_active = False
cdef RequestLogger _null_logger = RequestLogger.wrap(TRequestLoggerPtr())


cdef class AsJson:
    cdef object value;

    def __cinit__(self, value):
        self.value = value

    def __unicode__(self):
        return unicode(self.value)

    def __str__(self):
        s = str(self.value)
        return s if PY3 else s.encode('utf-8')


cdef class ClientStats:
    cdef public ui64 active_loggers_count
    cdef public ui64 events_count
    cdef public ui64 pending_bytes_count
    cdef public ui64 written_frames_count
    cdef public ui64 written_bytes_count
    cdef public ui64 errors_count
    cdef public ui64 shrinked_bytes_count


def activate(file_name, service_name,
             async=True,
             flush_period=datetime.timedelta(seconds=1),
             file_stat_check_period=datetime.timedelta(seconds=1),
             max_string_size=1000000,
             flush_size=1024 * 1024):
    global _instance
    global _is_active

    if _instance:
        raise RuntimeError('rtlog is already active')

    cdef TClientOptions cpp_options
    cpp_options.Async = async
    if flush_period is not None:
        cpp_options.FlushPeriod = TDuration.MicroSeconds(int(flush_period.total_seconds() * 1000000))
    if file_stat_check_period is not None:
        cpp_options.FileStatCheckPeriod = TDuration.MicroSeconds(int(file_stat_check_period.total_seconds() * 1000000))
    cpp_options.FlushSize = TMaybe[ui64](<ui64>flush_size) if flush_size is not None else TMaybe[ui64]()
    _instance = ClientHolder()
    _instance.client = new TClient(_to_string(file_name), _to_string(service_name), cpp_options)
    _instance.max_string_size = max_string_size
    _instance.shrinked_bytes_count = 0
    _is_active = True


def deactivate():
    global _instance
    global _is_active

    if not _instance:
        raise RuntimeError('rtlog is not active')

    _instance = None
    _is_active = False


def get_stats():
    cdef ClientStats result = ClientStats()
    cdef TClientStats stats

    if _instance:
        stats = _instance.client.GetStats()
        result.active_loggers_count = stats.ActiveLoggersCount
        result.events_count = stats.EventsCount
        result.pending_bytes_count = stats.PendingBytesCount
        result.written_frames_count = stats.WrittenFramesCount
        result.written_bytes_count = stats.WrittenBytesCount
        result.errors_count = stats.ErrorsCount
        result.shrinked_bytes_count = _instance.shrinked_bytes_count
    return result


def null_logger():
    return _null_logger


def is_active():
    return _is_active


cdef do_begin_request(const TString& token, bool session, TEventTimestamp timestamp):
    cdef RequestLogger result = RequestLogger.wrap(_instance.client.CreateRequestLogger(token, session, timestamp));
    result._session = session
    return result


def begin_request(token=None, session=False, timestamp=None):
    if not _instance:
        return _null_logger;

    if timestamp is None:
        timestamp = python_timestamp()
    return do_begin_request(_to_string(token), session, timestamp)
