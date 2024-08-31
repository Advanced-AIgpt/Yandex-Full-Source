JAVA_PROGRAM(quasar-billing)

OWNER(
    g:paskills
    g:smarttv
)
JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/billing/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/org/springframework/boot/spring-boot-starter-web
    contrib/java/org/springframework/boot/spring-boot-starter-jetty
    contrib/java/org/springframework/boot/spring-boot-starter-log4j2
    contrib/java/org/springframework/boot/spring-boot-starter-data-jdbc
    contrib/java/org/springframework/boot/spring-boot-starter-actuator
    contrib/java/org/springframework/boot/spring-boot-starter-validation

    contrib/java/org/apache/httpcomponents/httpclient

    # for log4j2 async logging
    contrib/java/com/lmax/disruptor

    contrib/java/com/google/code/findbugs/jsr305
    contrib/java/com/google/guava/guava
    contrib/java/org/postgresql/postgresql
    contrib/java/com/zaxxer/HikariCP

    contrib/java/org/projectlombok/lombok
    contrib/java/org/json/json

    alice/paskills/common/tvm-with-metrics
    alice/paskills/common/model/billing
    alice/library/java/routing_datasource

    library/java/monlib/metrics
    library/java/monlib/metrics-jvm
    library/java/tvmauth
)

WITH_JDK()
JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

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
    contrib/java/org/apache/tomcat/embed/tomcat-embed-core
    contrib/java/org/apache/tomcat/embed/tomcat-embed-websocket
    contrib/java/junit/junit
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
    contrib/java/io/micrometer/micrometer-core
)

IDEA_EXCLUDE_DIRS(
    quasar-billing
    test-results
    src/intTest/test-results
    src/test/test-results
)

END()

IF(YA_IDE_IDEA)
    RECURSE(
        ../common/model/billing
    )
ENDIF()

RECURSE_FOR_TESTS(
    src/test
    src/intTest
)

