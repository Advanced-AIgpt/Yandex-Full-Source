JAVA_PROGRAM(dialogovo)

OWNER(
    g:paskills
)

WITH_KOTLIN()
WITH_JDK()
JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/com/google/protobuf/protobuf-java-util

    contrib/java/org/springframework/boot/spring-boot-starter-web
    contrib/java/org/springframework/boot/spring-boot-starter-jetty
    contrib/java/org/springframework/boot/spring-boot-starter-log4j2
    contrib/java/org/springframework/boot/spring-boot-starter-data-jdbc
    contrib/java/org/springframework/boot/spring-boot-starter-validation
    contrib/java/org/springframework/boot/spring-boot-starter-actuator
    contrib/java/org/springframework/boot/spring-boot-starter-json

    contrib/java/org/apache/commons/commons-lang3/3.12.0
    contrib/java/org/apache/httpcomponents/httpclient
    contrib/java/com/fasterxml/jackson/module/jackson-module-kotlin
    contrib/java/com/fasterxml/jackson/dataformat/jackson-dataformat-xml

    contrib/java/commons-lang/commons-lang

    # for log4j2 async logging
    contrib/java/com/lmax/disruptor

    contrib/java/com/google/guava/guava
    contrib/java/org/postgresql/postgresql

    contrib/java/org/projectlombok/lombok

    # arcadia deps goes on the last to use lib version defined in the dependency rather then defined in deps
    alice/paskills/common/apphost-spring-controller
    alice/paskills/common/instrumented-executor
    alice/paskills/common/proto-utils
    alice/paskills/common/protoseq-logger
    alice/paskills/common/tvm-spring-handler
    alice/paskills/common/tvm-with-metrics
    alice/paskills/common/model/billing
    alice/paskills/common/rest-template-factory

    alice/paskills/dialogovo/common

    alice/paskills/dialogovo/src/main/proto
    #alice/paskills/dialogovo/src/main/fairy_tales_proto

    alice/protos/data
    library/java/monlib/metrics
    library/java/monlib/metrics-jvm

    alice/nlu/libs/request_normalizer/java
    alice/nlu/libs/request_normalizer/java/src/c
    library/java/svnversion
    kikimr/public/sdk/java/table
    iceberg/inside-yt

    alice/kronstadt/scenarios/afisha
    alice/kronstadt/scenarios/alice4business
    alice/kronstadt/scenarios/automotive_hvac
    alice/kronstadt/scenarios/contacts
    alice/kronstadt/scenarios/photoframe
    alice/kronstadt/scenarios/theremin
    alice/kronstadt/scenarios/video_call
    alice/kronstadt/scenarios/skills_discovery
    alice/kronstadt/scenarios/implicit_skills_discovery
    alice/kronstadt/scenarios/tv_channels
    alice/kronstadt/scenarios/tv_feature_boarding
    alice/kronstadt/scenarios/tv_main

    alice/kronstadt/core
    # на самом деле dialogovo от сервера не зависит, он нужен только для package, зависимость уйдет с появлением шардов
    alice/kronstadt/server
    apphost/api/service/java/apphost
    apphost/lib/proto_answers
)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

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
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
    contrib/java/org/apache/tomcat/embed/tomcat-embed-websocket
    contrib/java/org/springframework/boot/spring-boot-starter-tomcat
    contrib/java/io/micrometer/micrometer-core
    alice/paskills/dialogovo/src/test/proto
)

IDEA_EXCLUDE_DIRS(
    dialogovo
    test-results
    src/dbtest/test-results
    src/test/test-results
    src/pgtest/test-results
    src/yttest/test-results
)

END()

# расскомментировать для одновременной разработки
IF(YA_IDE_IDEA)
    RECURSE(
        ../common/instrumented-executor
        ../common/model/billing
        ../common/protoseq-logger
        ../common/solomon-utils
        ../common/tvm-spring-handler
        ../common/tvm-with-metrics
        ../common/rest-template-factory
        ../../kronstadt
    )
ENDIF()

RECURSE(
    ammo
    common
)


RECURSE_FOR_TESTS(
    src/test
    src/dbtest
    src/pgtest
    src/yttest
)
