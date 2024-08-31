JUNIT5(apphost-spring-controller)

OWNER(g:paskills)

JDK_VERSION(17)
SIZE(SMALL)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.common.apphost.spring SRCDIR java **/*)

LINT(extended)

PEERDIR(
    alice/paskills/common/apphost-spring-controller
    contrib/java/org/springframework/boot/spring-boot-starter-test
)

EXCLUDE(
    contrib/java/junit/junit
)

JAVA_DEPENDENCIES_CONFIGURATION(
    #    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JVM_ARGS(--add-opens=java.base/java.util=ALL-UNNAMED)

END()
