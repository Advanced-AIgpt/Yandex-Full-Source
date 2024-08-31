JAVA_PROGRAM(kronstadt-server)

OWNER(
    g:paskills
)

WITH_KOTLIN()
JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/com/google/protobuf/protobuf-java-util

    contrib/java/org/springframework/boot/spring-boot-starter-web
    contrib/java/org/springframework/boot/spring-boot-starter-jetty
    contrib/java/org/springframework/boot/spring-boot-starter-log4j2
    contrib/java/org/springframework/boot/spring-boot-starter-actuator
    contrib/java/org/springframework/boot/spring-boot-starter-json

    contrib/java/com/fasterxml/jackson/module/jackson-module-kotlin

    contrib/java/commons-lang/commons-lang

    # for log4j2 async logging
    contrib/java/com/lmax/disruptor

    # arcadia deps goes on the last to use lib version defined in the dependency rather then defined in deps
    alice/paskills/common/instrumented-executor
    alice/paskills/common/protoseq-logger
    alice/paskills/common/tvm-spring-handler
    alice/paskills/common/tvm-with-metrics
    alice/paskills/common/apphost-spring-controller

    library/java/monlib/metrics
    library/java/monlib/metrics-jvm

    alice/kronstadt/core

    # for grpc
    alice/gproxy/library/protos
    alice/library/client/protos
    alice/cuttlefish/library/protos
    # для прогрева, уедет после https://st.yandex-team.ru/KRONSTADT-44
    alice/kronstadt/scenarios/video_call/src/main/proto

    apphost/tools/java_grpc_client
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.kronstadt.server SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    #FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
#FORBID_CONFLICT_DM_RECENT

)

KOTLINC_FLAGS(
    -Xjvm-default=all
    -java-parameters
    -Xopt-in=kotlin.RequiresOptIn
)

JAVAC_FLAGS(
    -parameters
)

EXCLUDE(
    contrib/java/com/google/protobuf/protobuf-javalite
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
    contrib/java/org/apache/tomcat/embed/tomcat-embed-websocket
    contrib/java/org/springframework/boot/spring-boot-starter-tomcat
    contrib/java/io/micrometer/micrometer-core
)

IDEA_EXCLUDE_DIRS(
    kronstadr-server
    test-results
)

END()

RECURSE_FOR_TESTS(
    src/test
)

