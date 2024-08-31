LIBRARY()

OWNER(g:voicetech-infra)

NEED_REVIEW()

SRCS(
    request_builder.cpp
    request_builder_impl.cpp
    response_builder.cpp
    response_builder_impl.cpp
)

PEERDIR(
    library/cpp/json
    library/cpp/string_utils/base64
    alice/gproxy/library/protos
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/cuttlefish/common
    voicetech/library/idl/log
    alice/gproxy/library/events
    alice/megamind/protos/grpc_request
)

END()

RECURSE_FOR_TESTS(ut)
