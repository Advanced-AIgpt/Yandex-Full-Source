LIBRARY()

OWNER(
    olegator
    g:alice
)

PEERDIR(
    alice/library/client/protos
    alice/library/logger
    alice/megamind/protos/scenarios
    library/cpp/scheme
    library/cpp/semver
)

SRCS(
    client_features.cpp
    client_info.cpp
    interfaces_util.cpp
)

END()
