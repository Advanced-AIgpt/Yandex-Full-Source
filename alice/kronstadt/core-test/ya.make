JAVA_LIBRARY(kronstadt-core-test)

OWNER(
    g:paskills
)

WITH_KOTLIN()
JDK_VERSION(17)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.kronstadt.test SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)
PEERDIR(
    alice/kronstadt/core
    alice/kronstadt/server
    contrib/java/org/springframework/boot/spring-boot-starter-test
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
    -Xopt-in=kotlin.RequiresOptIn
)

JAVAC_FLAGS(
    -parameters
)

IDEA_EXCLUDE_DIRS(
    kronstadt-core-test
    test-results
)

END()


