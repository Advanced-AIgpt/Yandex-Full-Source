JAVA_LIBRARY(proto-utils)

IF(JDK_VERSION == "")
    JDK_VERSION(11)
ENDIF()

OWNER(
    g:paskills
)

WITH_KOTLIN()

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)
INCLUDE(${KOTLIN_BOM_FILE})

PEERDIR(
    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/com/google/protobuf/protobuf-java-util
    contrib/java/com/fasterxml/jackson/module/jackson-module-kotlin
    contrib/java/org/apache/logging/log4j/log4j-core
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.common.proto.utils SRCDIR src/main/kotlin **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JAVAC_FLAGS(
    -parameters
)

END()
