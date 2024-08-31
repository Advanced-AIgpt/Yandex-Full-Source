GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(user_agent.go)

GO_TEST_SRCS(user_agent_test.go)

END()

RECURSE(gotest)
