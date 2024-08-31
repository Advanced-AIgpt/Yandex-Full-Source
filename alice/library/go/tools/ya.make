GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    base64.go
    bools.go
    bytes.go
    env.go
    errors.go
    huid.go
    ints.go
    json.go
    logging.go
    strings.go
    url.go
)

GO_TEST_SRCS(
    ints_test.go
    json_test.go
    strings_test.go
    url_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
