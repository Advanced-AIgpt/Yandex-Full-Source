LIBRARY()

OWNER(
    g:hollywood
    khr2
)

PEERDIR(
    alice/hollywood/library/bass_adapter
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/fast_command
    alice/hollywood/library/scenarios/news/nlg
    alice/hollywood/library/scenarios/news/proto
    alice/hollywood/library/scenarios/search
    alice/library/json
    alice/library/logger
    alice/library/sys_datetime
    alice/library/util
    alice/megamind/protos/scenarios
    alice/memento/proto
    alice/protos/data
    library/cpp/http/simple
    util/draft
)

SRCS(
    bass.cpp
    frame.cpp
    memento_helper.cpp
    news_block.cpp
    news_fast_data.cpp
    news_settings_push.cpp
    prepare_handle.cpp
    render_handle.cpp
    GLOBAL news.cpp
)

END()

RECURSE_FOR_TESTS(
    it
    it2
    nlg/ut
    ut
    ut_render
)
