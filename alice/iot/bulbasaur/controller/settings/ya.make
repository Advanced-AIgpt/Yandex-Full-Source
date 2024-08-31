GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    const.go
    context.go
    controller.go
    interface.go
    iot_config.go
    model.go
    music_config.go
    order_config.go
    tts_whisper_config.go
    voiceprint_device_config.go
)

GO_TEST_SRCS(const_test.go)

END()

RECURSE(gotest)
