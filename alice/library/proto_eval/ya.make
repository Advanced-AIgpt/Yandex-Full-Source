LIBRARY()

OWNER(
    g:alice
    lavv17
)

PEERDIR(
    alice/library/logger
    alice/library/proto_eval/proto
    library/cpp/expression
    library/cpp/regex/pire
)

SRCS(
    proto_eval.cpp
    proto_eval_trace.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
