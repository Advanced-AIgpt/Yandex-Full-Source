UNITTEST()

SIZE(MEDIUM)

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
    alice/nlu/data/ar/granet
    alice/nlu/data/ar/test/granet/small
    alice/nlu/data/kk/granet
    alice/nlu/data/kk/test/granet/small
    alice/nlu/data/paskills_ru/granet
    alice/nlu/data/paskills_ru/test/granet/small
    alice/nlu/data/ru/granet
    alice/nlu/data/ru/test/granet/small
    alice/nlu/data/snezhana_ru/granet
    alice/nlu/data/snezhana_ru/test/granet/small
    alice/nlu/data/tr/granet
    alice/nlu/data/tr/test/granet/small
)

END()
