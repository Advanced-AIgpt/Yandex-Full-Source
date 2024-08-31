LIBRARY()

OWNER(
    deemonasd
    g:alice_quality
)

SRCS(
    contacts_custom_entity.cpp
    contacts_debug_constant.cpp
    contacts_granet.cpp
)

PEERDIR(
    alice/library/contacts
    alice/nlu/granet/lib/compiler
    alice/nlu/libs/entity_searcher
    alice/nlu/libs/tokenization
    alice/megamind/protos/common
    kernel/lemmer/core
)

END()

RECURSE_FOR_TESTS(ut)
