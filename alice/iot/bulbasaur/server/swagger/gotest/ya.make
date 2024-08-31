GO_TEST_FOR(alice/iot/bulbasaur/server/swagger)

OWNER(g:alice_iot)

INCLUDE(${ARCADIA_ROOT}/library/go/test/go_toolchain/recipe.inc)

SIZE(MEDIUM)

DEPENDS(vendor/github.com/go-swagger/go-swagger/cmd/swagger)

TEST_CWD(alice/iot/bulbasaur/server)

DATA(arcadia/alice/iot/bulbasaur)

END()
