LIBRARY()

OWNER(
    g:hollywood
    klim-roma
)

PEERDIR(
    alice/hollywood/library/registry
    contrib/libs/openssl
    library/cpp/string_utils/secret_string
)

SRCS(
    aes.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
