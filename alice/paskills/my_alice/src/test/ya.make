OWNER(
    g:paskills
)

JUNIT5()

JDK_VERSION(11)

# FIXME Discrepancy between the available test versions by dependencies with the launcher version was found.
# Remove ENV macro below to reproduce the problem.
# For more info see https://st.yandex-team.ru/DEVTOOLSSUPPORT-7454#6128ec627e6507138f034e45
ENV(DISABLE_JUNIT_COMPATIBILITY_TEST=1)

SIZE(MEDIUM)

JAVA_SRCS(SRCDIR java **/*)
#JAVA_SRCS(SRCDIR resources **/*)

DATA(
    arcadia/alice/paskills/my_alice/config
)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

TEST_CWD(alice/paskills/my_alice)

PEERDIR(
    alice/paskills/my_alice
    contrib/java/org/springframework/boot/spring-boot-starter-test
    contrib/java/org/junit/jupiter/junit-jupiter
    contrib/java/com/squareup/okhttp3/mockwebserver # 3.12.0
)

EXCLUDE(
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JAVAC_FLAGS(
    -parameters
)

END()
