LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    storage.cpp
)

PEERDIR(
    alice/matrix/library/ydb

    alice/megamind/protos/speechkit
    alice/protos/api/notificator
)

END()
