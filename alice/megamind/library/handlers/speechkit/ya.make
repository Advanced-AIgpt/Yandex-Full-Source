LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/megamind/library/globalctx
    alice/megamind/library/handlers/utils
    alice/megamind/library/registry
    alice/megamind/library/response
    alice/megamind/library/response_meta
    alice/megamind/library/scenarios/interface
    alice/megamind/library/sources
    alice/megamind/protos/speechkit
    library/cpp/json
    library/cpp/json/writer
    library/cpp/logger
)

SRCS(
    error_interceptor.cpp
    speechkit.cpp
)

END()

RECURSE_FOR_TESTS(ut)
