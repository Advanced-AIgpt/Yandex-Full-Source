JAVA_LIBRARY(apphost-http-request)

OWNER(g:paskills)

JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/com/fasterxml/jackson/core/jackson-core
    contrib/java/com/fasterxml/jackson/core/jackson-databind

    library/java/annotations

    apphost/lib/proto_answers
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.common.apphost.http SRCDIR  src/main/java **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)


END()

