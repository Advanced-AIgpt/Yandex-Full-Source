JAVA_PROGRAM(social_sharing)

JDK_VERSION(11)

OWNER(g:paskills)

WITH_KOTLIN()

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/com/beust/jcommander
    contrib/java/com/fasterxml/jackson/core/jackson-databind
    contrib/java/com/fasterxml/jackson/module/jackson-module-kotlin
    contrib/java/com/lmax/disruptor # for log4j2 async logging
    contrib/java/javax/servlet/javax.servlet-api # initially provided by apphost
    contrib/java/org/apache/httpcomponents/httpclient

    contrib/java/org/springframework/boot/spring-boot-starter
    contrib/java/org/springframework/boot/spring-boot-starter-actuator
    contrib/java/org/springframework/boot/spring-boot-starter-log4j2
    contrib/java/org/springframework/boot/spring-boot-starter-jetty
    contrib/java/org/springframework/boot/spring-boot-starter-web

    library/java/annotations

    apphost/api/service/java/apphost
    apphost/lib/proto_answers
    alice/paskills/common/instrumented-executor
    alice/paskills/common/solomon-utils
    alice/paskills/common/tvm-spring-handler
    alice/paskills/common/tvm-with-metrics
    alice/paskills/common/vcs
    alice/paskills/common/ydb-client
    alice/paskills/social_sharing/proto/api
    alice/paskills/social_sharing/proto/context
    alice/protos/api/notificator
    library/java/svnversion
    library/java/monlib/metrics
    library/java/monlib/metrics-jvm
    logbroker/unified_agent/client/java/log4j2
    mapreduce/yt/interface/protos
)

JAVA_SRCS(SRCDIR src/main/kotlin **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
#    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JAVAC_FLAGS(
    -parameters
)

EXCLUDE(
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
    contrib/java/org/apache/tomcat/embed/tomcat-embed-websocket
    contrib/java/org/springframework/boot/spring-boot-starter-tomcat
    contrib/java/io/micrometer/micrometer-core
)

END()

RECURSE(
    proto
)

RECURSE_FOR_TESTS(
    src/test
    src/integration_test
)

IF(YA_IDE_IDEA)
    RECURSE(
        ../common/instrumented-executor
        ../../apphost/junit_fixture
    )
ENDIF()
