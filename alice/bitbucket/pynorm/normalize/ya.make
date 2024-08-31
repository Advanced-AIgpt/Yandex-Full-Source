LIBRARY()

OWNER(akastornov)

NO_COMPILER_WARNINGS()

PEERDIR(
    ADDINCL alice/bitbucket/pynorm/util
    alice/bitbucket/pynorm/c-algorithms
)

SRCS(
    normalize.c
    reverse-normalize.c
)

END()
