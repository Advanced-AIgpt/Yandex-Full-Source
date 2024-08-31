LIBRARY()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    context_hash.h
    context_transform.cpp
    utterance_transform.cpp
)

PEERDIR(
    util
)

PEERDIR(
    dict/dictutil
)

END()
