LIBRARY()

OWNER(akastornov)

NO_COMPILER_WARNINGS()

NO_UTIL()

PEERDIR(
    alice/bitbucket/pynorm/c-algorithms
    ADDINCL contrib/libs/zlib
)

CFLAGS(
    -DEIGEN_DONT_PARALLELIZE
    -DHAVE_PTHREAD
    -DSREAL
    -DTHREADPOOL
    -DUSE_BLAS=1
    -DUSE_COZY
    -fms-extensions
)

SRCS(
    util/util.c
    util/log.c
#    util/xalloc.c
    util/configfile.c
    util/wfst.c
    util/ofst-symbol-table.c
    util/fst-best-path.c
    util/string-utils.c
    util/cuckoo-hash.c
    util/small_stack.cpp
    util/sparse_vector.cpp
    util/token.cpp
    util/token_allocator.cpp
)

END()

RECURSE(
    util/ut
)
