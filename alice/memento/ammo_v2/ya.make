JAVA_PROGRAM(memento_ammo)

IF(JDK_VERSION == "")
    JDK_VERSION(11)
ENDIF()

OWNER(
    g:paskills
)

WITH_KOTLIN()

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

PEERDIR(
    alice/paskills/ammo_gen_dsl
    alice/memento/proto
    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/com/google/protobuf/protobuf-java-util
)


JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.memento.ammo SRCDIR src/main/kotlin **/*)

LINT(extended)

END()
