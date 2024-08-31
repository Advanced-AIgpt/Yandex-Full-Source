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
    alice/nlu/granet/lib
)

DEPENDS(
    alice/nlu/data/wizard_ru/granet
    alice/nlu/data/wizard_ru/test/granet/small
)

END()
