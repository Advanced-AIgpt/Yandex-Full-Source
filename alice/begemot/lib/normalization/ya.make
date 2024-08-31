LIBRARY()

OWNER(
    dan-anastasev
    g:alice_quality
)

PEERDIR(
    alice/begemot/lib/locale
    alice/nlu/libs/request_normalizer
    library/cpp/langs
    search/begemot/rules/internal/locale/proto
)

SRCS(
    normalization.cpp
)

END()
