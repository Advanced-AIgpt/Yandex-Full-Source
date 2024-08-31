LIBRARY()

OWNER(
    yagafarov
    g:megamind
)

PEERDIR(
    alice/hollywood/library/config
    alice/hollywood/library/modifiers/base_modifier
    library/cpp/protobuf/util
)

SRCS(
    modifier_registry.cpp
)

END()
