JAVA_PROGRAM(solomon-utils)

IF(JDK_VERSION == "")
    JDK_VERSION(11)
ENDIF()

OWNER(
    g:paskills
)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

PEERDIR(
    library/java/monlib/metrics
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.common.solomon.utils SRCDIR src/main/java **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JAVAC_FLAGS(
    -parameters
)


END()

#RECURSE_FOR_TESTS(src/test)
