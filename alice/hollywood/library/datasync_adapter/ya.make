LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/http_proxy
    alice/library/logger
)

SRCS(
    datasync_adapter.cpp
)

END()
