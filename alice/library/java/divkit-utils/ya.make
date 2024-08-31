JAVA_LIBRARY(alice-divkit-utils)

JDK_VERSION(11)
WITH_KOTLIN()

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

OWNER(g:paskills)

PEERDIR(
    contrib/java/com/google/protobuf/protobuf-java-util

    divkit/public/json-builder/kotlin
    alice/library/java/protobuf_utils
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/protos/api/voice_control
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.divkit SRCDIR src/main/kotlin **/*)

KOTLINC_FLAGS(
    -java-parameters
)

JAVAC_FLAGS(
    -parameters
)

LINT(extended)

END()
