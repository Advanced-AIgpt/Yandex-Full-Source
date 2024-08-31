PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:megamind
)

EXCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/protos/api/typed_callbacks/centaur_main_screen_callback
)

SRCS(
    all_typed_callbacks.proto
)

END()

RECURSE_FOR_TESTS(ut)
