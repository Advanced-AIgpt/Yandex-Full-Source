LIBRARY()

OWNER(
    vitvlkv
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/biometry
    alice/hollywood/library/capability_wrapper
    alice/hollywood/library/personal_data
    alice/hollywood/library/scenarios/music/music_backend_api/content_id
    alice/hollywood/library/scenarios/music/proto
    alice/hollywood/library/scenarios/music/util
    alice/library/biometry
    alice/library/data_sync
    alice/library/logger
    alice/megamind/protos/scenarios
    apphost/lib/proto_answers
)

SRCS(
    process_biometry.cpp
)

END()

RECURSE_FOR_TESTS(ut)
