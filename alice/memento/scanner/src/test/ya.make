OWNER(
    g:paskills
)

JUNIT5(memento-scanner-test)

SIZE(SMALL)
JDK_VERSION(17)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.memento.scanner SRCDIR java **/*)
# JAVA_SRCS(SRCDIR resources **/*)

INCLUDE(${ARCADIA_ROOT}/alice/paskills/common/ya.make.dependency_management.inc)

TEST_CWD(alice/memento)

PEERDIR(
    alice/memento/scanner
    contrib/java/org/junit/jupiter/junit-jupiter
)


LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

END()
