OWNER(g:paskills)

LIBRARY()

PEERDIR(
    alice/nlu/granet/lib
    alice/paskills/granet_server/config/proto
    alice/paskills/granet_server/proto
    kernel/server
    library/cpp/http/misc
    library/cpp/json
    library/cpp/logger
    library/cpp/logger/global
    library/cpp/protobuf/json
    library/cpp/string_utils/base64
    library/cpp/svnversion
)

SRCS(
    handlers.cpp
    granet_wrapper.cpp
    json_log_backend.cpp
    server.cpp
)

END()
