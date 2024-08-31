UNITTEST_FOR(alice/cuttlefish/library/cuttlefish/config)

OWNER(g:voicetech-infra)

SRCS(
    config_ut.cpp
)

RESOURCE(
    alice/uniproxy/library/settings/rtc_production.json /python_uniproxy_config.json

    alice/uniproxy/configs/beta/configs/cuttlefish.json /cuttlefish_beta_config.json
    alice/uniproxy/configs/prod/configs/cuttlefish.json /cuttlefish_prod_config.json
    alice/uniproxy/configs/split/beta/cuttlefish/configs/cuttlefish.json /cuttlefish_split_beta_config.json
    alice/uniproxy/configs/split/main/cuttlefish/configs/cuttlefish.json /cuttlefish_split_main_config.json
)

END()
