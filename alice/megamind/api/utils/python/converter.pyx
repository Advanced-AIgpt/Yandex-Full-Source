from util.generic.string cimport TString

cdef extern from "alice/megamind/protos/speechkit/directives.pb.h" namespace "NAlice::NSpeechKit":
    cdef cppclass TDirective:
        int SerializeToString(TString* s);

cdef extern from "alice/megamind/api/utils/directives.h" namespace "NAlice::NMegamindApi":
    TDirective MakeDirectiveWithTypedSemanticFrame(const TSemanticFrameRequestData& data);

cdef extern from "alice/megamind/protos/common/frame.pb.h" namespace "NAlice":
    cdef cppclass TSemanticFrameRequestData:
        int ParseFromString(const TString&);

def make_directive_with_typed_semantic_frame(request: bytes):
    cdef TSemanticFrameRequestData request_directive
    cdef TString buffer
    request_directive.ParseFromString(request)
    directive = MakeDirectiveWithTypedSemanticFrame(request_directive)
    directive.SerializeToString(&buffer)
    return buffer
