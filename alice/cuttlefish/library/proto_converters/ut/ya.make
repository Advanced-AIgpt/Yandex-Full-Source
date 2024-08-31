UNITTEST()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/proto_converters
)

SRCS(
    common.h
    test_auth_token_handler.cpp
    test_session_context.cpp
    test_message_header.cpp
)

END()
