LIBRARY()

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/library/dispatcher/common_handles/util
    alice/library/json
    alice/library/network
    alice/library/version
    library/cpp/neh
    apphost/api/service/cpp
)

SRCS(
    service.cpp
)

END()
