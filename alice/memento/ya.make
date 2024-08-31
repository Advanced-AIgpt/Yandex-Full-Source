JAVA_PROGRAM(memento)

OWNER(
    g:paskills
)
INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

JDK_VERSION(17)
WITH_JDK()
WITH_KOTLIN()

PEERDIR(
    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/com/google/protobuf/protobuf-java-util

    contrib/java/org/springframework/boot/spring-boot-starter-web
    contrib/java/org/springframework/boot/spring-boot-starter-actuator
    contrib/java/org/springframework/boot/spring-boot-starter-jetty
    contrib/java/org/springframework/boot/spring-boot-starter-log4j2

    # for log4j2 async logging
    contrib/java/com/lmax/disruptor

    # arcadia deps goes on the last to use lib version defined in the dependency rather then defined in deps
    alice/memento/proto
    alice/memento/grpc
    alice/memento/scanner
    apphost/tools/java_grpc_client
    alice/paskills/common/apphost-service
    alice/paskills/common/tvm-spring-handler
    alice/paskills/common/tvm-with-metrics
    alice/paskills/common/solomon-utils
    alice/paskills/common/protoseq-logger
    alice/paskills/common/proto-utils
    alice/paskills/common/ydb-client
    library/java/monlib/metrics
    library/java/monlib/metrics-jvm

    library/java/svnversion
    kikimr/public/sdk/java/table
    logbroker/unified_agent/client/java/log4j2
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.memento SRCDIR src/main/java **/*)
JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.memento SRCDIR src/main/kotlin **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    #FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)


EXCLUDE(
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
    contrib/java/org/apache/tomcat/embed/tomcat-embed-websocket
    contrib/java/org/springframework/boot/spring-boot-starter-tomcat
    contrib/java/io/micrometer/micrometer-core
)

JAVAC_FLAGS(
    -parameters
)

EXCLUDE(contrib/java/junit/junit)

END()


RECURSE(
    ammo_v2
    grpc
    proto
    scanner
)

# for IDEA project generation
IF(YA_IDE_IDEA)
    RECURSE(
        ../paskills/common/solomon-utils
        ../paskills/common/tvm-spring-handler
        ../paskills/common/tvm-with-metrics
        ../paskills/common/ydb-client
        ../paskills/common/proto-utils
        ../paskills/common/protoseq-logger
        ../paskills/ammo_gen_dsl
    )
ENDIF()

RECURSE_FOR_TESTS(
    src/test
    scanner/src/test
)


