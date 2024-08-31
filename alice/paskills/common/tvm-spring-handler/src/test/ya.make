OWNER(
    g:paskills
)

JUNIT5(tvm-spring-handler-test)

JDK_VERSION(11)

SIZE(SMALL)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.common.tvm.spring.handler SRCDIR java **/*)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

PEERDIR(
    alice/paskills/common/tvm-spring-handler
    contrib/java/org/springframework/boot/spring-boot-starter-log4j2
    contrib/java/org/springframework/boot/spring-boot-starter-jetty
    contrib/java/org/springframework/boot/spring-boot-starter-test
    contrib/java/org/junit/jupiter/junit-jupiter
    contrib/java/org/mockito/mockito-core
)


LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

EXCLUDE(
    contrib/java/ch/qos/logback
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
    contrib/java/org/apache/tomcat/embed/tomcat-embed-websocket
    contrib/java/org/springframework/boot/spring-boot-starter-tomcat
)

JAVAC_FLAGS(
    -parameters
)

END()
