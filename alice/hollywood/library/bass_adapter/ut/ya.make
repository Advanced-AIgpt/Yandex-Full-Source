UNITTEST_FOR(alice/hollywood/library/bass_adapter)

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/library/registry
    alice/hollywood/library/scenarios/music/nlg
    alice/library/unittest
    apphost/lib/service_testing
)

SRCS(
    alice/hollywood/library/bass_adapter/bass_adapter_ut.cpp
    alice/hollywood/library/bass_adapter/bass_renderer_ut.cpp
    alice/hollywood/library/bass_adapter/bass_stats_ut.cpp
)

END()
