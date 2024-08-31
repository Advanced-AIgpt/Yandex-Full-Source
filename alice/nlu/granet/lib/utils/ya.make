LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    json_utils.cpp
    string_utils.cpp
    text_view.cpp
    utils.cpp
)

PEERDIR(
    alice/nlu/libs/interval
    library/cpp/containers/comptrie
    library/cpp/dbg_output
    library/cpp/json
    library/cpp/langs
    library/cpp/packers
    library/cpp/threading/future
)

END()

RECURSE_FOR_TESTS(ut)
