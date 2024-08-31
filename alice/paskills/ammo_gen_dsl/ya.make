JAVA_LIBRARY(ammo_gen_dsl)

IF(JDK_VERSION == "")
    JDK_VERSION(11)
ENDIF()

OWNER(
    g:paskills
)

WITH_KOTLIN()

PEERDIR(
    contrib/java/com/google/protobuf/protobuf-java
)


JAVA_SRCS(PACKAGE_PREFIX ru.yandex.alice.paskills.ammo SRCDIR src/main/kotlin **/*)

LINT(extended)

JAVAC_FLAGS(
    -parameters
)

END()

RECURSE(example)

RECURSE_FOR_TESTS(src/test)
