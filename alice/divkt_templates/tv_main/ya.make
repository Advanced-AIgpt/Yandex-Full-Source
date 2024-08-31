JAVA_LIBRARY(tv-main-templates)

OWNER(g:smarttv)

WITH_KOTLIN()
JDK_VERSION(17)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/dialogovo/ya.make.dependency_management.inc)

PEERDIR(
    contrib/java/com/google/protobuf/protobuf-java-util
    divkit/public/json-builder/kotlin
    alice/library/java/divkit-utils
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.divkttemplates.tvmain SRCDIR src/main/java **/*)
#JAVA_SRCS(SRCDIR src/main/resources **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

KOTLINC_FLAGS(
    -java-parameters
)

JAVAC_FLAGS(
    -parameters
)


END()

RECURSE_FOR_TESTS(
    src/test
)
