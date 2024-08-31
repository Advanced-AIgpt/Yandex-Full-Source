LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/joker/library/backend
    alice/joker/library/log
    alice/joker/library/proto
    alice/joker/library/stub
)

SRCS(
    run_log_writer.cpp
)

END()
