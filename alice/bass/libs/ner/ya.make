LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/bass/libs/fetcher
    alice/bass/libs/logging_v2
    library/cpp/scheme
)

SRCS(ner.cpp)

END()
