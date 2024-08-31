JAVA_LIBRARY(alice-library-protobuf-utils)

OWNER(g:paskills)

IF (YA_IDE_IDEA)
    # tests require java 17 for records but in IDEA you can't have main different jdk version for module and it's tests
    JDK_VERSION(17)
ELSE()
    JDK_VERSION(11)
ENDIF()

WITH_KOTLIN()

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)
INCLUDE(${KOTLIN_BOM_FILE})

PEERDIR(
    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/com/google/protobuf/protobuf-java-util
    contrib/java/com/fasterxml/jackson/core/jackson-core
    contrib/java/com/fasterxml/jackson/core/jackson-databind
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.library.protobufutils SRCDIR  src/main/java **/*)

LINT(extended)

END()

RECURSE_FOR_TESTS(
    src/test
)

