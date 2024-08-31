OWNER(
    g:paskills
)

JUNIT5()
JDK_VERSION(17)

ENV(DISABLE_JUNIT_COMPATIBILITY_TEST=1)
SIZE(MEDIUM)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

DATA(
    arcadia/alice/paskills/billing/configs
    arcadia/alice/paskills/billing/misc
)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/billing/src/test/recipe.inc)
INCLUDE(${ARCADIA_ROOT}/alice/paskills/billing/ya.make.dependency_management.inc)

TEST_CWD(alice/paskills/billing)

PEERDIR(
    alice/paskills/billing
    contrib/java/org/springframework/boot/spring-boot-starter-jetty # to force currect jetty version

    contrib/java/org/springframework/boot/spring-boot-starter-test
    contrib/java/ru/yandex/qatools/embed/postgresql-embedded
    contrib/java/de/flapdoodle/embed/de.flapdoodle.embed.process
    contrib/java/commons-io/commons-io
    contrib/java/com/squareup/okhttp3/mockwebserver
    contrib/java/com/github/tomakehurst/wiremock-jre8
    contrib/java/org/skyscreamer/jsonassert
    contrib/java/org/junit/jupiter/junit-jupiter

    # for testcontainers
    #contrib/java/org/junit/vintage/junit-vintage-engine
    contrib/java/junit/junit
    contrib/java/org/testcontainers/postgresql
)

EXCLUDE(
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
)

JAVAC_FLAGS(
    -parameters
)

END()
