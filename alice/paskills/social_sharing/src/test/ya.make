OWNER(
    g:paskills
)

JUNIT5()

JDK_VERSION(11)

# FIXME Discrepancy between the available test versions by dependencies with the launcher version was found.
# Remove ENV macro below to reproduce the problem.
# For more info see https://st.yandex-team.ru/DEVTOOLSSUPPORT-7454#6128ec627e6507138f034e45
ENV(DISABLE_JUNIT_COMPATIBILITY_TEST=1)

SIZE(SMALL)

WITH_KOTLIN()

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.social.sharing SRCDIR kotlin **/*)

DATA(
    arcadia/alice/paskills/social_sharing/config
)

TEST_CWD(alice/paskills/social_sharing)

PEERDIR(
    alice/apphost/junit_fixture
    alice/paskills/social_sharing
    contrib/java/org/springframework/boot/spring-boot-starter-test
)

EXCLUDE(
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    # FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JAVAC_FLAGS(
    -parameters
)

END()

IF(YA_IDE_IDEA)
    RECURSE(
        ../../../../apphost/junit_fixture
    )
ENDIF()
