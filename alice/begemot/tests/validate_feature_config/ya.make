UNITTEST()

SIZE(SMALL)

OWNER(
    vl-trifonov
    g:alice_quality
)

SRCS(
    main.cpp
)

PEERDIR(
    alice/begemot/lib/feature_aggregator
)

DEPENDS(
    alice/nlu/data/ru/config
)

END()
