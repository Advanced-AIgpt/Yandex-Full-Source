OWNER(
    g-kostin
    g:alice
)

GO_LIBRARY()

PEERDIR(
    vendor/golang.org/x/xerrors
)

SRCS(
    dialog.go
    cue.go
    replies.go
)

GO_TEST_SRCS(
    dialog_test.go
    cue_test.go
)

END()

RECURSE_FOR_TESTS(
    gotest
)
