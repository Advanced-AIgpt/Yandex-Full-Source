JAVA_PROGRAM(dialogovo_ammo)

JDK_VERSION(11)

OWNER(
    g:paskills
)

WITH_KOTLIN()

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

PEERDIR(
    alice/paskills/ammo_gen_dsl
    alice/megamind/protos/scenarios
    alice/paskills/dialogovo/src/main/proto
    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/com/google/protobuf/protobuf-java-util
)


JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.dialogovo.ammo SRCDIR src/main/kotlin **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

LINT(extended)

END()
