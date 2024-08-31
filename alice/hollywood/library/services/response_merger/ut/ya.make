GTEST()

OWNER(g:paskills)

PEERDIR(
    alice/hollywood/library/global_context
    alice/hollywood/library/hw_service_context
    alice/hollywood/library/services/response_merger
    alice/hollywood/library/util
    alice/library/logger
    alice/megamind/protos/speechkit
    apphost/api/service/cpp
    apphost/example/howto/unit_tested_servant/handlers
    apphost/lib/service_testing
    library/cpp/json
    library/cpp/resource
)

SRCS(
    response_merger_ut.cpp
)

END()
