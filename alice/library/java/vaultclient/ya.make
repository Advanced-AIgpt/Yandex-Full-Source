JAVA_LIBRARY(vault-client)

JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

OWNER(g:paskills)

PEERDIR(
    contrib/java/com/fasterxml/jackson/core/jackson-databind
    contrib/java/org/slf4j/slf4j-api
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.vault SRCDIR src/main/java **/*)

JAVAC_FLAGS(
    -parameters
)

LINT(extended)

END()
