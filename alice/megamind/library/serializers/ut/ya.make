UNITTEST_FOR(alice/megamind/library/serializers)

OWNER(
    alkapov
    g:megamind
)

PEERDIR(
    alice/library/frame
    alice/library/unittest
    alice/megamind/library/testing
    alice/protos/data
    alice/protos/div
    library/cpp/langs
    library/cpp/testing/unittest
)

SRCS(
    scenario_proto_deserializer_ut.cpp
    shared_ut.cpp
    speechkit_proto_serializer_ut.cpp
    speechkit_struct_serializer_ut.cpp
)

END()
