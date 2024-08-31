JAVA_PROGRAM(tvm_with_metrics)

IF(JDK_VERSION == "")
    JDK_VERSION(11)
ENDIF()

OWNER(
    g:paskills
)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

PEERDIR(
    library/java/monlib/metrics
    library/java/tvmauth
    alice/paskills/common/solomon-utils
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.common.tvm.solomon SRCDIR src/main/java **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JAVAC_FLAGS(
    -parameters
)

IDEA_EXCLUDE_DIRS(
    tvm_with_metrics
    test-results
    src/test/test-results
)

END()
