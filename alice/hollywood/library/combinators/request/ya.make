LIBRARY()

OWNER(g:hollywood)

SRCS(
    request.cpp
    utils.cpp
)

PEERDIR(
    alice/hollywood/library/hw_service_context
    alice/hollywood/library/request
    alice/library/blackbox
    alice/library/logger
    alice/library/scenarios/utils
    alice/megamind/protos/scenarios
    apphost/lib/proto_answers
    contrib/libs/protobuf
    library/cpp/http/misc
)

END()
