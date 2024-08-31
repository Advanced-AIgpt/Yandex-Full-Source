GO_LIBRARY()

OWNER(g:alice_iot)

DEPENDS(vendor/github.com/go-swagger/go-swagger/cmd/swagger)

SRCS(swagger.go)

GO_TEST_SRCS(swagger_test.go)

GO_EMBED_PATTERN(swagger.json)

END()

RECURSE_FOR_TESTS(gotest)
