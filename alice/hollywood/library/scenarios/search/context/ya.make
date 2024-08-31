LIBRARY()

OWNER(
    tolyandex
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/search/proto
    alice/hollywood/library/scenarios/search/resources
    alice/hollywood/library/bass_adapter
    alice/hollywood/library/request/utils
    alice/library/geo
    alice/library/json
    alice/library/logger
    alice/library/url_builder
    alice/megamind/protos/common
    library/cpp/langs
    search/alice/serp_summarizer/runtime/proto
)

SRCS(
    context.cpp
)

END()
