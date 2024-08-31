OWNER(
    g:paskills
)

JUNIT5(bilenko_skill)

JDK_VERSION(11)
INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)


JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.skills.bilenko SRCDIR kotlin **/*)


PEERDIR(

    contrib/java/org/springframework/boot/spring-boot-starter-test
    contrib/java/org/junit/jupiter/junit-jupiter
    alice/paskills/skills/4bilenko
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

END()
