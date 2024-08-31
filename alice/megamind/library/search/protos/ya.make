PROTO_LIBRARY()

OWNER(
    g:megamind
    yagafarov
)

SRCS(
    alice_meta_info.proto
)

IF(NZ_SYNC)
    PEERDIR(
        alice/megamind/library/search/protos/protos_nz_sync
    )
ELSE()
    PEERDIR(
        alice/megamind/library/search/protos/protos
    )
ENDIF()

EXCLUDE_TAGS(GO_PROTO)

END()

RECURSE(
    protos
    protos_nz_sync
)
