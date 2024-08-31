OWNER(
    g:paskills
)

JUNIT5()

JDK_VERSION(11)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.common.vcs SRCDIR java **/*)


INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)


PEERDIR(
    alice/paskills/common/vcs
    contrib/java/org/junit/jupiter/junit-jupiter
)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

EXCLUDE(


)

JAVAC_FLAGS(
    -parameters
)

END()
