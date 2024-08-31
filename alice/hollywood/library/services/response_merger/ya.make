LIBRARY()

OWNER(
    g:yandexdialogs2
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_hw_service
    alice/hollywood/library/registry
    alice/megamind/protos/scenarios
    alice/protos/api/renderer
    tools/enum_parser/enum_serialization_runtime
    alice/protos/div
)

SRCS(
    response_merger.cpp
    utils.cpp
    GLOBAL register.cpp
)

GENERATE_ENUM_SERIALIZATION(response_merger.h)

END()

RECURSE_FOR_TESTS(ut)
