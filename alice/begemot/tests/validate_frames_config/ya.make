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
    alice/begemot/lib/frame_aggregator
)

DEPENDS(
    alice/nlu/data/ar/config
    alice/nlu/data/ru/config
    alice/nlu/data/ru/dev
)

END()
