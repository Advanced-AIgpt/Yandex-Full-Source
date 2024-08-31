PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    contacts_ut.py
    smart_home_ut.py
    memento_ut.py
)

PEERDIR(
    alice/memento/proto
    alice/megamind/protos/common
    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/auth
    alice/uniproxy/library/auth/mocks
    alice/uniproxy/library/backends_ctxs
    alice/uniproxy/library/backends_common
    alice/uniproxy/library/logging
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/settings
    alice/uniproxy/library/testing
    alice/uniproxy/library/utils

    contrib/python/tornado/tornado-4
)

SET(PWD "arcadia/alice/uniproxy/library/backends_ctxs/ut")
DATA(
    ${PWD}/contacts_list_empty.dump
    ${PWD}/contacts_list.dump
    ${PWD}/smart_home.dump
)

END()
