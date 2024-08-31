JUNIT5()

JDK_VERSION(11)

OWNER(
    vitvlkv
    g:megamind
    pazus
)


JAVA_SRCS(SRCDIR ${ARCADIA_ROOT}/alice/megamind/example_apps/java/src/test/java **/*)


INCLUDE(${ARCADIA_ROOT}/contrib/java/org/junit/junit-bom/5.8.2/ya.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/contrib/java/org/springframework/boot/spring-boot-dependencies/2.5.3/ya.dependency_management.inc)
INCLUDE(${ARCADIA_ROOT}/contrib/java/org/apache/logging/log4j/log4j-bom/2.17.0/ya.dependency_management.inc)
DEPENDENCY_MANAGEMENT(
    # testing
    contrib/java/com/squareup/okhttp3/mockwebserver/3.12.0 # 3.12.0
    contrib/java/org/skyscreamer/jsonassert/1.5.0
)

PEERDIR(
    alice/megamind/example_apps/java
    contrib/java/org/springframework/boot/spring-boot-starter-test
    contrib/java/org/junit/jupiter/junit-jupiter
)

EXCLUDE(
    contrib/java/junit/junit
    contrib/java/org/apache/logging/log4j/log4j-to-slf4j
)

NO_LINT()

END()
