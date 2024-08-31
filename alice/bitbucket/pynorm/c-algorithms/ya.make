LIBRARY()

OWNER(akastornov)

NO_UTIL()

ADDINCL(
    alice/bitbucket/pynorm/c-algorithms/include/libcalg
    GLOBAL alice/bitbucket/pynorm/c-algorithms/include
)

SRCS(
    src/arraylist.c
    src/compare-string.c
    src/hash-string.c
    src/hash-table.c
)

END()
