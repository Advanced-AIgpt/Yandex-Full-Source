JAVA_PROGRAM(divkt-renderer)

OWNER(
    g:smarttv
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

    # arcadia deps goes on the last to use lib version defined in the dependency rather then defined in deps
    alice/divkt_renderer/api

    alice/library/java/divkit-utils
    alice/megamind/protos/scenarios
    alice/protos/api/renderer
    alice/protos/div
    divkit/public/json-builder/kotlin

    # for log4j2 async logging
    contrib/java/com/lmax/disruptor

    apphost/tools/java_grpc_client
    alice/paskills/common/apphost-service
    alice/paskills/common/solomon-utils
    alice/paskills/common/protoseq-logger
    alice/paskills/common/proto-utils
    library/java/monlib/metrics
    library/java/monlib/metrics-jvm

    library/java/svnversion
    logbroker/unified_agent/client/java/log4j2
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.divktrenderer SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

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



# RECURSE_FOR_TESTS(
#     src/test
#    scanner/src/test
#)


