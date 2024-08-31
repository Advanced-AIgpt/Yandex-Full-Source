LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    session.cpp
    message.cpp
    converters.h
    enum_converter.h
    converter_handlers.cpp
    converter_handlers.h
)

PEERDIR(
    library/cpp/json
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/convert
)

END()

RECURSE_FOR_TESTS(ut)
