JAVA_PROGRAM(bilenko_skill)

JDK_VERSION(11)

OWNER(
    g:paskills
)

#INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)


WITH_KOTLIN()

PEERDIR(
    contrib/java/org/jetbrains/kotlinx/kotlinx-coroutines-reactor

    contrib/java/org/springframework/boot/spring-boot-starter-webflux
    contrib/java/org/springframework/boot/spring-boot-starter-jetty
    contrib/java/org/springframework/boot/spring-boot-starter-log4j2

    contrib/java/com/fasterxml/jackson/module/jackson-module-kotlin

    # for log4j2 async logging
    contrib/java/com/lmax/disruptor
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.skills.bilenko SRCDIR src/main/kotlin **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    #FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
)

JAVAC_FLAGS(
    -parameters
)

EXCLUDE(
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
    contrib/java/org/apache/tomcat/embed/tomcat-embed-websocket
    contrib/java/org/springframework/boot/spring-boot-starter-tomcat
)

END()

RECURSE_FOR_TESTS(src/test)
