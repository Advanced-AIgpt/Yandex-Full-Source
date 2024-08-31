LIBRARY()

OWNER(
    tolyandex
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/http_requester
    alice/hollywood/library/scenarios/search/context
    alice/hollywood/library/scenarios/search/proto
    alice/hollywood/library/scenarios/search/utils
    alice/library/app_navigation
    alice/library/client
    alice/library/experiments
    alice/library/json
    alice/library/logger
    alice/library/network
    alice/library/parsed_user_phrase
    alice/library/response_similarity
    alice/library/url_builder
    alice/megamind/protos/common
    alice/hollywood/library/frame_filler/lib
    alice/hollywood/library/scenarios/goodwin/handlers
    library/cpp/http/simple
    library/cpp/langs
    library/cpp/string_utils/url
    search/alice/serp_summarizer/runtime/proto
    quality/functionality/facts/multilanguage_facts/processor/proto
)

SRCS(
    app_navigation.cpp
    base.cpp
    direct.cpp
    ellipsis_intents.cpp
    facts.cpp
    goodwin.cpp
    multilang_facts.cpp
    nav.cpp
    navigator_intent.cpp
    push_notification.cpp
)

END()
