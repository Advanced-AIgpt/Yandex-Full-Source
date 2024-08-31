OWNER(
    g:paskills
)

JUNIT5(alice4business-scenario-ut)

TAG(alice4businessTag)
SIZE(SMALL)

WITH_KOTLIN()
JDK_VERSION(17)
INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.kronstadt.scenarios.alice4business SRCDIR java **/*)

# FIXME Discrepancy between the available test versions by dependencies with the launcher version was found.
# Remove ENV macro below to reproduce the problem.
# For more info see https://st.yandex-team.ru/DEVTOOLSSUPPORT-7454#6128ec627e6507138f034e45
ENV(DISABLE_JUNIT_COMPATIBILITY_TEST=1)


PEERDIR(
    alice/kronstadt/scenarios/alice4business
    alice/kronstadt/core-test

    contrib/java/org/mockito/mockito-core
    contrib/java/org/springframework/boot/spring-boot-starter-test
    contrib/java/org/junit/jupiter/junit-jupiter
    contrib/java/com/squareup/okhttp3/mockwebserver # 3.12.0

)


LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

KOTLINC_FLAGS(
    -Xjvm-default=all
    -java-parameters
    # В IDEA добавлять не нужно, нужно только для ya make -t
    -Xfriend-paths=alice/kronstadt/scenarios/alice4business/alice4business-scenario.jar
)

JAVAC_FLAGS(
    -parameters
)

EXCLUDE(
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
)

END()
