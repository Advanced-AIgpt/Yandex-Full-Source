LIBRARY()

OWNER(
    g:voicetech-infra
)

SRCS(
    asr_adapter.cfgproto
    asr_fake_server.cfgproto
    cloud_synth.cfgproto
    cuttlefish.cfgproto
    music_match_adapter.cfgproto
    music_match_fake_server.cfgproto
    rtlog.cfgproto
    service.cfgproto
    tts_adapter.cfgproto
    tts_cache_fake_server.cfgproto
    tts_cache_proxy.cfgproto
    tts_fake_server.cfgproto
    yabio_adapter.cfgproto
    yabio_fake_server.cfgproto
)

PEERDIR(
    library/cpp/proto_config/protos
)

END()
