LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    starter.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/protos
    alice/megamind/protos/speechkit
    apphost/api/service/cpp
)

END()

RECURSE_FOR_TESTS(ut)
