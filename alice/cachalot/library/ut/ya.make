UNITTEST_FOR(alice/cachalot/library)

OWNER(
    g:voicetech-infra
)

SRCS(
    modules/megamind_session/ut.cpp
    modules/vins_context/ut.cpp
    modules/yabio_context/ut.cpp
    storage/inmemory/imdb_ut.cpp
    storage/inmemory/policy_ut.cpp
    storage/mock_ut.cpp
)

PEERDIR(
    alice/cuttlefish/library/cachalot_client
    alice/cuttlefish/library/cachalot_client/http_client
)

END()
