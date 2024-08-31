LIBRARY()

OWNER(
    akhruslan
    g:megamind
)

PEERDIR(
    alice/hollywood/library/dispatcher/common_handles/util
    alice/hollywood/library/global_context
    alice/hollywood/library/metrics
    alice/hollywood/library/util
    alice/library/metrics
    alice/library/scenarios/data_sources
    alice/library/websearch/response
    alice/protos/websearch
    apphost/api/service/cpp
    apphost/lib/proto_answers
)

SRCS(
    split_web_search.cpp
)

END()
