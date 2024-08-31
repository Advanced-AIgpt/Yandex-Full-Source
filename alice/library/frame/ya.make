LIBRARY()

OWNER(
    the0
    petrk
    g:alice
)

PEERDIR(
    alice/library/json
    alice/library/proto
    alice/megamind/protos/common
    alice/megamind/protos/speechkit
    library/cpp/cgiparam
    library/cpp/iterator
    library/cpp/json
)

SRCS(
    builder.cpp
    description.cpp
    directive_builder.cpp
    utils.cpp
)

END()

RECURSE_FOR_TESTS(ut)
