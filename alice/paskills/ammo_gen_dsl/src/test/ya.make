OWNER(
    g:paskills
)


JUNIT5()

JDK_VERSION(11)
WITH_KOTLIN()
SIZE(SMALL)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.ammo SRCDIR kotlin **/*)

# is 5.5.2 is set evetyhing is ok, explicit peerdir to junit-jupiter-engine is not needed
# if 5.6.1 is set and no peerdir to junit-jupiter-engine then run fails with
    # Exception in thread "main" org.junit.platform.commons.PreconditionViolationException:
    # Cannot create Launcher without at least one TestEngine;
    # consider adding an engine implementation JAR to the classpath
# if 5.6.1 and peerdir to junit-jupiter-engine is set no suites/tests are found (build green)
INCLUDE(${ARCADIA_ROOT}/contrib/java/org/junit/junit-bom/5.8.1/ya.dependency_management.inc)

PEERDIR(
    alice/paskills/ammo_gen_dsl
    contrib/java/org/junit/jupiter/junit-jupiter
)

LINT(extended)

JAVA_DEPENDENCIES_CONFIGURATION(
    #FORBID_DIRECT_PEERDIRS
    FORBID_DEFAULT_VERSIONS
    FORBID_CONFLICT_DM_RECENT
)

JAVAC_FLAGS(
    -parameters
)

END()
