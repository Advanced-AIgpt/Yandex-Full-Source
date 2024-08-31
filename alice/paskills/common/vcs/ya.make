JAVA_PROGRAM(vcs_util)

JDK_VERSION(11)

OWNER(g:paskills)


PEERDIR(
    library/java/svnversion
    library/java/annotations
)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.common.vcs SRCDIR src/main/java **/*)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JAVAC_FLAGS(
    -parameters
)

END()

RECURSE_FOR_TESTS(src/test)
