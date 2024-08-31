JAVA_LIBRARY(routing-datasource)

OWNER(g:paskills)

JDK_VERSION(11)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

PEERDIR(
    library/java/annotations
    contrib/java/org/springframework/spring-jdbc
    contrib/java/org/springframework/spring-context
    contrib/java/org/springframework/spring-aop
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.library.routingdatasource SRCDIR  src/main/java **/*)

LINT(extended)

END()


