JAVA_LIBRARY(kronstadt-core)

OWNER(
    g:paskills
)

WITH_KOTLIN()
JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/com/google/protobuf/protobuf-java-util

    contrib/java/org/springframework/boot/spring-boot-starter-log4j2
    contrib/java/org/springframework/boot/spring-boot-starter-json

    contrib/java/com/fasterxml/jackson/module/jackson-module-kotlin

    contrib/java/com/google/code/findbugs/jsr305

    # arcadia deps goes on the last to use lib version defined in the dependency rather then defined in deps

    # yandex libraries
    library/java/monlib/metrics
    library/java/monlib/metrics-jvm
    library/java/svnversion
    library/java/tvmauth

    # alice libs
    alice/megamind/protos/scenarios
    alice/protos/data
    alice/protos/div
    alice/library/contacts/proto
    alice/library/java/divkit-utils
    alice/library/java/protobuf_utils
    alice/memento/proto

    alice/kronstadt/core/src/main/proto
    alice/paskills/common/proto-utils

    # for ApphostResponseBuilder
    apphost/api/service/java/apphost
    alice/paskills/common/apphost-http-request
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.kronstadt.core SRCDIR src/main/java **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    #FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
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
    contrib/java/junit/junit
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
    contrib/java/org/apache/tomcat/embed/tomcat-embed-websocket
    contrib/java/org/springframework/boot/spring-boot-starter-tomcat
    contrib/java/io/micrometer/micrometer-core
    contrib/java/com/fasterxml/jackson/dataformat/jackson-dataformat-xml
)

IDEA_EXCLUDE_DIRS(
    kronstadt-core
    test-results
)

END()

IF(NOT YA_IDE_IDEA)
    RECURSE_FOR_TESTS(
        ../../paskills/dialogovo/src/test
    )
ENDIF()

RECURSE_FOR_TESTS(
    src/test
)


