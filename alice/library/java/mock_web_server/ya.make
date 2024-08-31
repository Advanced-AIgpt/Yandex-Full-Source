JAVA_LIBRARY(mock-web-server)

JDK_VERSION(11)

OWNER(
    g:paskills
)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.apphost SRCDIR src/main/kotlin **/*)

LINT(extended)

PEERDIR(
    contrib/java/org/apache/logging/log4j/log4j-core
)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JAVAC_FLAGS(
    -parameters
)

END()
