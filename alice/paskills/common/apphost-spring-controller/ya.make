JAVA_LIBRARY(apphost-spring-controller)

OWNER(g:paskills)

JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/org/apache/logging/log4j/log4j-api
    contrib/java/org/springframework/spring-context

    library/java/annotations

    apphost/api/service/java/apphost
    apphost/lib/proto_answers
    library/java/monlib/metrics
    alice/paskills/common/apphost-service
    alice/paskills/common/apphost-http-request
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.common.apphost.spring SRCDIR  src/main/java **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
#    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

EXCLUDE(
    contrib/java/com/google/protobuf/protobuf-javalite
    iceberg/inside-yt/core
)

END()

RECURSE_FOR_TESTS(
    src/test
)
