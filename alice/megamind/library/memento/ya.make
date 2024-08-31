LIBRARY()

OWNER(
    g:megamind
    g-kostin
)

PEERDIR(
    alice/megamind/library/speechkit
    alice/megamind/library/util
    alice/memento/proto
)

SRCS(
    memento.cpp
)

END()

RECURSE_FOR_TESTS(ut)
