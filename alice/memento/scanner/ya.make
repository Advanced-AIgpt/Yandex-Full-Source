JAVA_LIBRARY(memento-scanner)

OWNER(
    g:paskills
)
JDK_VERSION(17)
INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/com/google/protobuf/protobuf-java-util

    contrib/java/org/springframework/boot/spring-boot-starter-log4j2
    contrib/java/org/springframework/spring-core
    contrib/java/org/springframework/spring-beans
    contrib/java/org/springframework/spring-context
    contrib/java/org/apache/commons/commons-lang3

    alice/memento/proto

    contrib/java/org/projectlombok/lombok
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.memento.scanner SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR ../proto/defaults **/*.pb.txt)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)


END()


RECURSE_FOR_TESTS(
    src/test
)


