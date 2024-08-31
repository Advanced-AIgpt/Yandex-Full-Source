JAVA_PROGRAM(rest-template-factory)

OWNER(
    g:paskills
)

WITH_KOTLIN()
JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)
INCLUDE(${KOTLIN_BOM_FILE})

PEERDIR(
    contrib/java/org/springframework/boot/spring-boot-starter-web

    contrib/java/org/apache/httpcomponents/httpclient

    alice/paskills/common/instrumented-executor
    alice/paskills/common/protoseq-logger
    alice/paskills/common/tvm-spring-handler

    alice/kronstadt/core
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.common.resttemplate.factory SRCDIR src/main/java **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JAVAC_FLAGS(
    -parameters
)

KOTLINC_FLAGS(
    -Xjvm-default=all
    -java-parameters
)

END()

#RECURSE_FOR_TESTS(src/test)

