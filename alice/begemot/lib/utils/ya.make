LIBRARY()

OWNER(
    smirnovpavel
    g:alice_quality
)

PEERDIR(
    alice/library/frame
    alice/library/proto
    alice/megamind/protos/common
    alice/nlu/granet/lib/grammar
    alice/nlu/libs/frame
    library/cpp/langs
    search/begemot/core
    search/begemot/rules/alice/microintents/proto
    search/begemot/rules/granet/proto
    search/begemot/rules/granet_config/proto
)

SRCS(
    config.cpp
    form_to_frame.cpp
    frame_description.cpp
    granet_config.cpp
    microintent_to_frame.cpp
)

END()

RECURSE_FOR_TESTS(ut)
