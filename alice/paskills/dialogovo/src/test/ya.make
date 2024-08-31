OWNER(
    g:paskills
)

JUNIT5(dialogovo-ut)

# FIXME Discrepancy between the available test versions by dependencies with the launcher version was found.
# Remove ENV macro below to reproduce the problem.
# For more info see https://st.yandex-team.ru/DEVTOOLSSUPPORT-7454#6128ec627e6507138f034e45
ENV(DISABLE_JUNIT_COMPATIBILITY_TEST=1)

SIZE(MEDIUM)

WITH_KOTLIN()
JDK_VERSION(17)

JAVA_SRCS(SRCDIR ${ARCADIA_ROOT}/alice/paskills/dialogovo/src/test/java **/*)
JAVA_SRCS(SRCDIR ${ARCADIA_ROOT}/alice/paskills/dialogovo/src/test/resources **/*)

DATA(
    arcadia/alice/paskills/dialogovo/config
)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

TEST_CWD(alice/paskills/dialogovo)

PEERDIR(
    alice/paskills/dialogovo
    alice/kronstadt/core-test
    alice/kronstadt/server
    alice/paskills/dialogovo/src/test/proto

    alice/kronstadt/scenarios/theremin/src/test

    contrib/java/org/mockito/mockito-core
    contrib/java/org/springframework/boot/spring-boot-starter-test
    contrib/java/commons-io/commons-io
    contrib/java/com/squareup/okhttp3/mockwebserver # 3.12.0
    contrib/java/org/skyscreamer/jsonassert
    contrib/java/org/junit/jupiter/junit-jupiter
    contrib/java/org/mockito/kotlin/mockito-kotlin
)

EXCLUDE(
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
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
    -Xfriend-paths=alice/kronstadt/scenarios/theremin/theremin-scenario.jar
)

JAVAC_FLAGS(
    -parameters
)

REQUIREMENTS(ram:12)

END()
