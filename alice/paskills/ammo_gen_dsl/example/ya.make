JAVA_PROGRAM(ammo_gen_example)

JDK_VERSION(11)

OWNER(
    g:paskills
)

WITH_KOTLIN()

PEERDIR(alice/paskills/ammo_gen_dsl)


JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.ammo.example SRCDIR src/main/kotlin **/*)

LINT(extended)

JAVAC_FLAGS(
    -parameters
)

END()
