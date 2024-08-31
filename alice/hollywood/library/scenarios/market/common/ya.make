LIBRARY()

OWNER(
    g:marketinalice
)

SRCS(
    context.cpp
    fast_data.cpp
    handle.cpp
    market_geo_support.cpp
    market_url_builder.cpp
    report/client.cpp
    report/handle.cpp
    report/proxy.cpp
    report/request.cpp
    report/response.cpp
    response_builder.cpp
    search_info.cpp
    types.cpp
    types/picture.cpp
)

GENERATE_ENUM_SERIALIZATION(experiments.h)
GENERATE_ENUM_SERIALIZATION(types.h)

PEERDIR(
    alice/hollywood/library/scenarios/market/common/nlg
    alice/hollywood/library/scenarios/market/common/proto

    alice/hollywood/library/framework

    alice/library/geo
    alice/library/url_builder
)

END()
