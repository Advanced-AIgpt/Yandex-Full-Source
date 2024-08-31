LIBRARY()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/client
    alice/bass/libs/config
    alice/bass/libs/globalctx
    alice/bass/libs/push_notification/handlers
    alice/bass/libs/push_notification/scheme
    alice/bass/libs/source_request
    library/cpp/cgiparam
    library/cpp/scheme
)

SRCS(
    create_callback_data.cpp
    request.cpp
)

END()
