UNITTEST()

SIZE(SMALL)

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    main.cpp
)

PEERDIR(
    alice/begemot/lib/trivial_tagger
)

DEPENDS(
    alice/nlu/data/ru/config
)

END()
