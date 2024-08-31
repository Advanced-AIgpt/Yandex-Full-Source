LIBRARY()

OWNER(
    smirnovpavel
    g:alice_quality
    g:begemot
)

PEERDIR(
    alice/begemot/lib/locale
    alice/nlu/libs/fst
    alice/nlu/libs/type_parser
    alice/begemot/lib/fst
    library/cpp/langs
    search/begemot/core
    search/begemot/core/proto
    search/begemot/rules/alice/session/proto
)

SRCS(
    time.cpp
)

END()
