UNITTEST()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/converter
)

SRCS(
    common.h
    test_synchronize_state.cpp
    test_directives.cpp
)

END()
