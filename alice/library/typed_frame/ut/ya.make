UNITTEST_FOR(alice/library/typed_frame)

OWNER(g:megamind)

PEERDIR(
    alice/library/unittest
    alice/protos/data
    alice/protos/endpoint
    alice/protos/endpoint/capabilities/opening_sensor
    library/cpp/testing/unittest
)

SRCS(
    typed_frames_ut.cpp
    typed_semantic_frame_request_ut.cpp
)

END()
