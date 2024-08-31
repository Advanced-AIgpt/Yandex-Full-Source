JAVA_PROGRAM(cowsay)

JDK_VERSION(11)

OWNER(
    vitvlkv
    g:megamind
    pazus
)


INCLUDE(${ARCADIA_ROOT}/contrib/java/org/springframework/boot/spring-boot-dependencies/2.5.3/ya.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/contrib/java/org/apache/logging/log4j/log4j-bom/2.17.0/ya.dependency_management.inc)


PEERDIR(
    alice/megamind/protos/scenarios

    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/com/google/protobuf/protobuf-java-util

    contrib/java/org/springframework/boot/spring-boot-starter-web
    contrib/java/org/springframework/boot/spring-boot-starter-log4j2
)

JAVA_SRCS(SRCDIR src/main/java **/*)

NO_LINT()

END()

IF(NOT SANITIZER_TYPE)
    RECURSE_FOR_TESTS(
        ut
    )
ENDIF()
