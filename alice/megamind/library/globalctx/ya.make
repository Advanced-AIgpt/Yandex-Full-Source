LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/megamind/library/config
    alice/megamind/library/config/protos

    library/cpp/logger
)

SRCS(
    globalctx.cpp
)

END()

RECURSE(
    impl
)
