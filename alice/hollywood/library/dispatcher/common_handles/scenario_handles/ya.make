LIBRARY()

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/dispatcher/common_handles/util
    alice/hollywood/library/global_context
    apphost/api/service/cpp
)

SRCS(
    scenario.cpp
)

END()
