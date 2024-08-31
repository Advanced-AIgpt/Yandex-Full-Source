LIBRARY()

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/library/base_hw_service
    alice/hollywood/library/dispatcher/common_handles/util
    alice/hollywood/library/global_context
    alice/hollywood/library/hw_service_context
    alice/hollywood/library/metrics
    apphost/api/service/cpp
)

SRCS(
    hw_service.cpp
)

END()
