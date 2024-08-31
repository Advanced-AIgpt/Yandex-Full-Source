UNITTEST_FOR(alice/nlg/library/nlg_renderer)

OWNER(alexanderplat g:alice)

SRCS(
    alice/nlg/library/nlg_renderer/nlg_library_registry_ut.cpp
    alice/nlg/library/nlg_renderer/nlg_renderer_ut.cpp
)

PEERDIR(
    library/cpp/testing/unittest
    alice/nlg/library/nlg_renderer/ut/nlg
)

END()
