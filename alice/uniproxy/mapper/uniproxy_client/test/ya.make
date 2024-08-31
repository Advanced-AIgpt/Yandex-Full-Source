UNITTEST()

SIZE(MEDIUM)

TAG(ya:external)

OWNER(
    g:alice_downloaders
)

NEED_CHECK()

SRCS(
    uniproxy_client_ut.cpp
)

DATA(
    sbr://2479875247  # address.opus
)

PEERDIR(
    alice/uniproxy/mapper/uniproxy_client/lib
    library/cpp/testing/unittest
)

REQUIREMENTS(network:full)

END()
