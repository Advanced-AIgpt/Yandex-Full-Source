LIBRARY()

OWNER(
    g:bass
    g:smarttv
)

PEERDIR(
    alice/bass/libs/fetcher
    alice/bass/libs/logging_v2
    alice/bass/libs/metrics
    alice/bass/libs/source_request
    alice/bass/libs/tvm2/ticket_cache
    alice/bass/libs/video_content
    alice/bass/libs/video_content/protos
    alice/bass/libs/ydb_helpers
    alice/bass/util
    alice/library/json
    alice/library/logger
    alice/library/parsed_user_phrase
    alice/library/response_similarity
    alice/library/video_common
    alice/library/video_common/protos
    alice/megamind/protos/common
    alice/nlu/libs/request_normalizer
    alice/protos/data/search_result
    alice/protos/data/video
    catboost/libs/model
    contrib/libs/cctz
    kernel/inflectorlib/phrase/simple
    library/cpp/cgiparam
    library/cpp/http/misc
    library/cpp/langs
    library/cpp/resource
    library/cpp/scheme
    library/cpp/scheme/util
    library/cpp/string_utils/base64
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
    library/cpp/timezone_conversion
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_table
)

SRCS(
    amediateka_utils.cpp
    content_db.cpp
    content_db_check_utils.cpp
    defs.cpp
    feature_calculator.cpp
    formulas.cpp
    has_good_result/factors.cpp
    has_good_result/video.sc
    item_selector.cpp
    ivi_genres.cpp
    ivi_utils.cpp
    keys.cpp
    kinopoisk_recommendations.cpp
    kinopoisk_utils.cpp
    nlg_utils.cpp
    okko_utils.cpp
    providers.cpp
    rps_config.cpp
    show_or_gallery/factors.cpp
    show_or_gallery/video.sc
    tvm_cache_delegate.cpp
    universal_api_utils.cpp
    utils.cpp
    video.sc
    video_url_getter.cpp
    video_ut_helpers.cpp
    yavideo_utils.cpp
    youtube_utils.cpp
)

GENERATE_ENUM_SERIALIZATION(content_db.h)

GENERATE_ENUM_SERIALIZATION(defs.h)

GENERATE_ENUM_SERIALIZATION(keys.h)

FROM_SANDBOX(FILE 610641977 OUT_NOAUTO has_good_result.cbm)

FROM_SANDBOX(FILE 610615712 OUT_NOAUTO show_or_gallery.cbm)

RESOURCE(
    has_good_result.cbm has_good_result.cbm
)

RESOURCE(
    show_or_gallery.cbm show_or_gallery.cbm
)

END()

RECURSE(
    ut
)
