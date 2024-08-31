JAVA_PROGRAM(pgconverter)

JDK_VERSION(11)

OWNER(
    g:paskills
)

WITH_KOTLIN()

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)
INCLUDE(${KOTLIN_BOM_FILE})

PEERDIR(
    contrib/java/org/postgresql/postgresql
    contrib/java/com/fasterxml/jackson/core/jackson-databind
    contrib/java/com/fasterxml/jackson/module/jackson-module-kotlin
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.common.pgconverter SRCDIR src/main/java **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
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

