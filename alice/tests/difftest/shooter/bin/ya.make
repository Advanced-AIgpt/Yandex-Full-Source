PROGRAM(shooter)

OWNER(sparkle)

PEERDIR(
    alice/joker/library/log
    alice/tests/difftest/shooter/library/core
    alice/tests/difftest/shooter/library/diff2html
    alice/tests/difftest/shooter/library/perfdiff
    alice/tests/difftest/shooter/library/ydb
    contrib/libs/re2
    library/cpp/getopt
    library/cpp/json
    library/cpp/scheme
    library/cpp/sighandler
)

SRCS(
    main.cpp
)

END()
