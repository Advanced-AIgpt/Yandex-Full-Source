LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/protos/extensions
    alice/library/json
    alice/megamind/protos/common
    library/cpp/http/io
    library/cpp/http/misc
    library/cpp/json
    library/cpp/string_utils/base64
)

SRCS(
    typed_frames.cpp
    typed_semantic_frame_request.cpp
)

END()

RECURSE_FOR_TESTS(ut)
