JAVA_PROGRAM(my_alice)

JDK_VERSION(11)

OWNER(g:paskills)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/org/springframework/boot/spring-boot-starter
    contrib/java/org/springframework/boot/spring-boot-starter-log4j2
    contrib/java/com/lmax/disruptor # for log4j2 async logging

    library/java/annotations
    contrib/java/org/springframework/spring-web
    contrib/java/org/springframework/boot/spring-boot-starter-jetty
    contrib/java/org/springframework/boot/spring-boot-starter-web
    contrib/java/javax/servlet/javax.servlet-api # initially provided by apphost
    contrib/java/com/fasterxml/jackson/core/jackson-databind

    contrib/java/org/projectlombok/lombok

    apphost/api/service/java/apphost
    apphost/lib/proto_answers
    geobase/java
    alice/paskills/common/vcs
    library/java/svnversion
    library/java/monlib/metrics
    library/java/monlib/metrics-jvm
)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

IF(YA_IDE_IDEA)
    # additional sources to be included by IDEA plugin to ease changes
    JAVA_SRCS(SRCDIR ../../../apphost/conf/backends/ALICE dummy_file_name_to_exclude_from_jar_resources.json)
    JAVA_SRCS(SRCDIR ../../../apphost/conf/verticals/ALICE dummy_file_name_to_exclude_from_jar_resources.json)
ENDIF()

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
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

RECURSE_FOR_TESTS(
    src/test
)

RECURSE(
    ../my_alice_pumpkin
)
