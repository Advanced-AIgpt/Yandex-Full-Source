GO_LIBRARY()

OWNER(
    g:amelie
    alkapov
)

PEERDIR(
    alice/amelie/internal/interceptor
    alice/amelie/internal/model
    alice/amelie/pkg/bass
    alice/amelie/pkg/iot
    alice/amelie/pkg/re
    alice/amelie/pkg/telegram
    alice/amelie/pkg/telegram/interceptor
    library/go/core/log

    vendor/gopkg.in/tucnak/telebot.v2
)

SRCS(
    account.go
    amelie.go
    command.go
    common.go
    device.go
    onboarding.go
    shortcuts.go
    support.go
    youtube.go
)

GO_TEST_SRCS(
    common_test.go
    youtube_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
