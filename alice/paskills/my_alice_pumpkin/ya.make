JAVA_PROGRAM(my_alice_pumpkin)

JDK_VERSION(11)

OWNER(g:paskills)

WITH_KOTLIN()

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/org/springframework/boot/spring-boot-starter
    contrib/java/org/springframework/boot/spring-boot-starter-log4j2
    contrib/java/com/lmax/disruptor # for log4j2 async logging

    library/java/annotations
    contrib/java/org/springframework/boot/spring-boot-starter-web
    contrib/java/org/springframework/boot/spring-boot-starter-jetty
    contrib/java/org/springframework/boot/spring-boot-starter-actuator
    contrib/java/javax/servlet/javax.servlet-api # initially provided by apphost
    contrib/java/com/fasterxml/jackson/core/jackson-databind
    contrib/java/org/apache/httpcomponents/httpclient

    contrib/java/org/projectlombok/lombok

    apphost/api/service/java/apphost
    apphost/lib/proto_answers
    alice/paskills/common/vcs
    library/java/svnversion
    library/java/monlib/metrics
    library/java/monlib/metrics-jvm
    logbroker/unified_agent/client/java/log4j2
)

JAVA_SRCS(SRCDIR src/main/kotlin **/*)
#JAVA_SRCS(SRCDIR src/main/resources **/*)

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

    # exclude dependencies from iceberg/inside-yt/core referenced by apphost
    mapreduce/yt/interface/protos
    iceberg/bolts
    iceberg/misc
    devtools/jdk-compat
    yt/yt_proto/yt/core
)

END()

#RECURSE_FOR_TESTS(
#    src/test
#)
