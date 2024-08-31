OWNER(
    g:paskills
)

JUNIT5(dialogovo-pg-test)

SIZE(MEDIUM)

WITH_KOTLIN()
JDK_VERSION(17)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

DATA(
    arcadia/alice/paskills/dialogovo/config
)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/alice/paskills/pg_embedded.ya.make)

TEST_CWD(alice/paskills/dialogovo)

PEERDIR(
    alice/paskills/dialogovo
    alice/paskills/dialogovo/src/test/proto
    alice/kronstadt/scenarios/theremin/src/test
    contrib/java/org/mockito/mockito-core
    contrib/java/org/springframework/boot/spring-boot-starter-test
    contrib/java/ru/yandex/qatools/embed/postgresql-embedded
    contrib/java/de/flapdoodle/embed/de.flapdoodle.embed.process # /2.1.2
    contrib/java/commons-io/commons-io
    contrib/java/com/squareup/okhttp3/mockwebserver # 3.12.0
    contrib/java/org/skyscreamer/jsonassert
    contrib/java/org/junit/jupiter/junit-jupiter
)

EXCLUDE(
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
    contrib/java/junit/junit
)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    #FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

KOTLINC_FLAGS(
    -Xjvm-default=all
    -java-parameters
    # В IDEA добавлять не нужно, нужно только для ya make -t
    -Xfriend-paths=alice/paskills/dialogovo/dialogovo.jar
)

JAVAC_FLAGS(
    -parameters
)

END()
