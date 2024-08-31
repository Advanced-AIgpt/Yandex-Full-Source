LIBRARY()

OWNER(
    yagafarov
    g:megamind
)

PEERDIR(
    alice/hollywood/library/request

    alice/library/logger
    alice/library/metrics
    alice/library/util
    alice/megamind/protos/modifiers
)

SRCS(
    context.cpp
)

END()
