PROGRAM()

OWNER(
    smirnovpavel
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/anaphora_resolver/measure_quality/lib
    library/cpp/getopt/small
)

SRCS(
    main.cpp
)

END()

RECURSE(
    lib
)
