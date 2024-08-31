PY3_LIBRARY()

OWNER(
    g:megamind
    yagafarov
)

PEERDIR(
    alice/acceptance/modules/request_generator/lib
    alice/hollywood/library/scenarios/alice_show/proto
    alice/library/python/testing/megamind_request
    alice/megamind/library/python/testing/session_builder
    alice/megamind/library/session/protos
    alice/megamind/mit/library/common/names
    alice/megamind/mit/library/response
    alice/megamind/mit/library/util
    contrib/python/protobuf
)

PY_SRCS(
    __init__.py
    request_builder.py
)

END()
