from util.generic.ptr cimport THolder, MakeHolder
from util.generic.string cimport TString
from util.generic.maybe cimport TMaybe
from util.system.types cimport ui32, ui64
from libcpp cimport bool, nullptr
from cython.operator cimport dereference as deref


cdef extern from "google/protobuf/message.h" namespace "google::protobuf" nogil:
    cdef cppclass Message:
        TString SerializeAsString() const

cdef extern from "library/cpp/eventlog/eventlog.h" nogil:
    ctypedef ui32 TEventClass
    ctypedef ui64 TEventTimestamp

    cdef cppclass TEvent:
        TEventClass Class
        TEventTimestamp Timestamp

        const Message* GetProto() const

    cdef cppclass TConstEventPtr:
        const TEvent* Get() const

    cdef enum:
        TEndOfFrameEvent_EventClass "TEndOfFrameEvent::EventClass"

    cdef cppclass IEventFactory:
        pass

cdef extern from "library/cpp/eventlog/eventlog.h" namespace "NEvClass" nogil:
    cdef IEventFactory* Factory()

cdef extern from "util/stream/input.h" nogil:
    cdef cppclass IInputStream:
        pass

cdef extern from "util/stream/file.h" nogil:
    cdef cppclass TFileInput(IInputStream):
        TFileInput(...) except +

cdef extern from "library/cpp/eventlog/logparser.h" nogil:
    cdef cppclass TFrame:
        TFrame(IInputStream&, IEventFactory*)
        inline ui32 FrameId() const

    TMaybe[TFrame] FindNextFrame(IInputStream*, IEventFactory*)

    cdef cppclass TEventFilter:
        pass

    cdef cppclass TFrameDecoder:
        TFrameDecoder(const TFrame&, const TEventFilter* const filter, bool strict=False, bool withRawData=False)
        TConstEventPtr operator*() const
        bool Next()

cdef extern from *:
    ui64 eventlogFrameCounter

cdef class TEventData:
    cdef public TEventClass event_class
    cdef public TEventTimestamp timestamp
    cdef public bytes data

cdef class TFrameData:
    cdef public ui32 id
    cdef public list events

cdef TEventData convert_event(const TEvent& e):
    cdef TEventData result = TEventData()

    result.event_class = e.Class
    result.timestamp = e.Timestamp
    result.data = e.GetProto().SerializeAsString()
    return result

cdef TFrameData load_frame(const TFrame& frame):
    cdef TFrameData result = TFrameData()
    cdef THolder[TFrameDecoder] decoder = MakeHolder[TFrameDecoder](frame, <const TEventFilter *>nullptr, False, True)
    cdef TConstEventPtr ev
    cdef const TEvent* e

    result.id = frame.FrameId()
    result.events = []
    while True:
        ev = deref(deref(decoder.Get()))
        e = ev.Get()
        if not e or e.Class == TEndOfFrameEvent_EventClass:
            break
        result.events.append(convert_event(deref(e)))
        if not decoder.Get().Next():
            break;
    return result

def load_frames(const TString& file_name):
    cdef list result = []
    cdef THolder[TFileInput] input = MakeHolder[TFileInput](file_name)
    cdef TMaybe[TFrame] frame

    while True:
        frame = FindNextFrame(input.Get(), Factory())
        if not frame.Defined():
            break
        result.append(load_frame(frame.GetRef()))
    return result

def reset_eventlog():
    global eventlogFrameCounter

    eventlogFrameCounter = 0
