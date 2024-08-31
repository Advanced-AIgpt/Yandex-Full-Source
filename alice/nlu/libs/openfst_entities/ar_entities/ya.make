LIBRARY()

OWNER(
    moath-alali
    g:alice_quality
)

NO_COMPILER_WARNINGS()

PEERDIR(
    alice/library/json
    contrib/libs/openfst
    contrib/libs/openfst/src/extensions/far/fstfar
    contrib/libs/re2
    contrib/python/pynini/extensions
)

SRCS(
    ar_entities.cpp
)

GENERATE_ENUM_SERIALIZATION(ar_entities.h)

END()

RECURSE_FOR_TESTS(
    ut
)
