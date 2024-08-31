LIBRARY()

OWNER(
    alexanderplat
    g:hollywood
)

SRCS(
    helper.cpp
    hwf_state.cpp

    render_nlg/render_bass_block_context.cpp
    render_nlg/render_nlg.cpp
    render_nlg/render_suggest.cpp
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/framework/helpers/beggins
    alice/hollywood/library/http_proxy
    alice/hollywood/library/request
    alice/library/json
    alice/library/network
    alice/megamind/protos/scenarios
    alice/vins/api/vins_api/speechkit/connectors/protocol/protos
    alice/vins/api/vins_api/speechkit/protos
    alice/protos/data
    alice/protos/data/language
    library/cpp/cgiparam
    library/cpp/string_utils/base64
)

END()
