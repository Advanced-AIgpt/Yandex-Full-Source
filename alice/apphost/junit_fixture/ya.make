JAVA_LIBRARY(junit-apphost-fixture)

JDK_VERSION(11)

OWNER(
    g:paskills
)

WITH_KOTLIN()

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)
INCLUDE(${KOTLIN_BOM_FILE})

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.apphost SRCDIR src/main/kotlin **/*)

LINT(extended)

PEERDIR(
    devtools/jtest
    apphost/lib/proto_answers

    contrib/java/org/apache/logging/log4j/log4j-core
    contrib/java/com/fasterxml/jackson/core/jackson-databind
    contrib/java/com/fasterxml/jackson/module/jackson-module-kotlin
)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JAVAC_FLAGS(
    -parameters
)

END()

