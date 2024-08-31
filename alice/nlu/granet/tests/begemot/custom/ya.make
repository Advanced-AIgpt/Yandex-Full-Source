UNITTEST()

OWNER(
    samoylovboris
    g:alice_quality
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
    alice/nlu/data/ru/granet
    alice/nlu/data/ru/test/granet/custom
    alice/nlu/data/ru/test/granet/tom
)

REQUIREMENTS(ram:11)

END()
