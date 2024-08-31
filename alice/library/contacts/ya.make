LIBRARY()

OWNER(
    deemonasd
    g:alice_quality
)

PEERDIR(
    alice/library/compression
    alice/library/contacts/proto
    alice/megamind/protos/speechkit
    library/cpp/string_utils/base64
)

SRCS(
    contacts.cpp
)

END()

RECURSE(
    proto
)

RECURSE_FOR_TESTS(ut)
