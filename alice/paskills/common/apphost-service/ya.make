JAVA_LIBRARY(apphost-service)

OWNER(g:paskills)

JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/org/apache/logging/log4j/log4j-api
    contrib/java/org/springframework/spring-context

    library/java/annotations

    apphost/api/service/java/apphost
    library/java/monlib/metrics
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.common.apphost.spring SRCDIR  src/main/java **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)


END()

