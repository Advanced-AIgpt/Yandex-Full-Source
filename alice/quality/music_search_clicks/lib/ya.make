LIBRARY()

OWNER(
    vl-trifonov
    skorodumov-s
    olegator
    g:alice_quality
)

PEERDIR(
    alice/quality/music_search_clicks/proto

    kernel/alice/music_scenario/web_url_canonizer/lib
    kernel/hosts/owner
    library/cpp/codecs
    library/cpp/resource
    mapreduce/yt/interface

    quality/personalization/big_rt/aggregator_vcdiff/lib
    quality/personalization/big_rt/rapid_clicks_common/aggregator
    quality/personalization/big_rt/rapid_clicks_common/config
    quality/personalization/big_rt/rapid_clicks_common/counters
    quality/personalization/big_rt/rapid_clicks_common/proto
)

SRCS(
    clicks_computer.cpp
)

RESOURCE(
    yweb/urlrules/2ld.list /2ld.list
    yweb/urlrules/ungrouped.list /ungrouped.list
)

END()

RECURSE_FOR_TESTS(ut)
