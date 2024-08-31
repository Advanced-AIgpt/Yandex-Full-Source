LIBRARY()

OWNER(
    flimsywhimsy
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/datasync_adapter
    alice/hollywood/library/scenarios/suggesters/common
    alice/hollywood/library/scenarios/video_rater/nlg
    alice/hollywood/library/scenarios/video_rater/proto
    alice/library/experiments
    alice/library/logger
)

SRCS(
    commit_prepare_handle.cpp
    commit_render_handle.cpp
    entity_search_adapter.cpp
    prepare_handle.cpp
    render_handle.cpp
    GLOBAL video_rater.cpp
)

GENERATE_ENUM_SERIALIZATION(render_handle.h)

END()

RECURSE_FOR_TESTS(it)
