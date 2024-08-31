UNITTEST()

OWNER(
    samoylovboris
    g:alice_quality
    moath-alali
)

SIZE(MEDIUM)
FORK_SUBTESTS()
SPLIT_FACTOR(30)

SRCS(
    main.cpp
)

PEERDIR(
    alice/nlu/granet/lib
)

DEPENDS(
    alice/nlu/data/ar/granet
    alice/nlu/data/ar/test/granet/medium
    alice/nlu/data/ru/granet
    alice/nlu/data/ru/test/granet/medium
    alice/nlu/data/tr/granet
    alice/nlu/data/tr/test/granet/medium
)

REQUIREMENTS(ram:11)

END()
