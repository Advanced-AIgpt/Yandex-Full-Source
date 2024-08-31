# Java: вспомогательные команды

## ya maven-import
Для импорта библиотек из maven репозиториев в аркадию используется команда `ya maven-import groupId:artifactId:version` (которая взаимодействует с maven). В результате выполнения команды нужная библиотека со всеми зависимостями появится в [contrib/java](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/java) в виде модуля `JAVA_LIBRARY`, описанного в `groupId/artifactId/version/ya.make`(закоммитить нужно самостоятельно). После этого можно зависеть от этого модуля, используя `PEERDIR`.

Команда `ya maven-import` по умолчанию ищет артефакты в репозиториях из [contrib/java/MAVEN_REPOS](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/java/MAVEN_REPOS). Произвольный репозиторий можно указать через ключ `--remote-repository, -r`. Все артефакты сначала выкачиваются из maven репозитория на локальную машину, а затем заливаются в Sandbox как ресурсы. В ya.make листах будут написаны только id ресурсов в специальном макросе `EXTERNAL_JAR()`, сами артефакты в аркадию не попадают.

Некоторые фичи maven-сборки при `maven-import` не поддерживаются или поддерживаются частично:
* Игнорируется `<exclusions>`
* Игнорируются все `optional` и `test` зависимости
* Если у артефакта есть classifier, отличный от sources, в `contrib/java` он прикрепляется к artifactId. В частности, если артефакт платформозависим, и это реализовано в maven с помощью classifier, специальной поддержки в `maven-import` для этого нет.

{% cut "Почему использование maven-библиотек реализовано именно так" %}

Одна из причин, почему нельзя просто указать в `ya.make` зависимость от `junit:junit:4.12`, указать maven-репозитории, и система сборки, используя maven, сама бы все принесла - это диапазоны версий в maven. Например, библиотека A может зависеть от библиотеки B версии `[1.0,)`. Тогда в зависимости от доступности удаленных репозиториев, от наличия в репозиториях более свежих версий библиотеки B, maven может выбирать разные версии. Это нарушает свойство **воспроизводимости сборки**.

Аналогичный подход используется в bazel и buck (как в системах, рассчитанных на сравнимый масштаб и на зависимость по исходникам). Например, в bazel в `WORKSPACE` можно указать `maven_jar(name, artifact, repository, sha1)`, который затем можно использовать в своих `BUILD` как зависимости. `maven_jar` приносит указанный артефакт //без транзитивных зависимостей//, все зависимости нужно указывать вручную через тот же `maven_jar`. Предлагается с помощью специальных утилит ([generate_workspace](https://docs.bazel.build/versions/master/external.html#transitive-dependencies), [bazel-deps](https://github.com/pgr0ss/bazel-deps)) генерировать большие записи в `WORKSPACE`/`BUILD` и использовать их. Это аналогично `maven-import`. Обсуждение такого решения [здесь](https://github.com/bazelbuild/bazel/issues/89). Похожая ситуация и в buck. Обсуждения [здесь](https://github.com/facebook/buck/issues/64) и [здесь](https://github.com/facebook/buck/issues/902).

{% endcut %}

{% cut "Если maven-import падает с ошибками" %}

Например, если падает вот так
https://paste.yandex-team.ru/949193
то нужно поднять недостающие зависимости командой
```
ya make --checkout -j0 devtools/maven-import
```

{% endcut %}

Подробнее о правилах заноса и обновления библиотек можно почитать в [arcadia/contrib/README.md](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/README.md).

Чтобы понимать, какая библиотека откуда приехала, почему выбралась версия библиотеки X, а не Y, почему библиотека A стоит в classpath раньше, чем B и т.д., есть 3 полезные команды.

## ya java classpath
Команда позволяет узнать, какие библиотеки попали в classpath данного модуля после применения правил `DEPENDENCY_MANAGEMENT`, `EXCLUDE` и разрешения конфликтов версий. Например:
```
~/arcadia/iceberg/misc$ ya java classpath
iceberg/misc/iceberg-misc.jar
iceberg/bolts/iceberg-bolts.jar
contrib/java/junit/junit/4.12/junit-4.12.jar
contrib/java/org/hamcrest/hamcrest-core/1.3/hamcrest-core-1.3.jar
contrib/java/org/openjdk/jmh/jmh-generator-annprocess/1.11.2/jmh-generator-annprocess-1.11.2.jar
contrib/java/org/openjdk/jmh/jmh-core/1.11.2/jmh-core-1.11.2.jar
contrib/java/net/sf/jopt-simple/jopt-simple/4.6/jopt-simple-4.6.jar
contrib/java/org/apache/commons/commons-math3/3.2/commons-math3-3.2.jar
contrib/java/com/google/code/findbugs/jsr305/3.0.0/findbugs-jsr305-3.0.0.jar
iceberg/misc-bender-annotations/iceberg-misc-bender-annotations.jar
iceberg/misc-signal/iceberg-misc-signal.jar
devtools/jtest/devtools-jtest.jar
contrib/java/log4j/log4j/1.2.17/log4j-1.2.17.jar
contrib/java/org/apache/logging/log4j/log4j-api/2.5/log4j-api-2.5.jar
contrib/java/org/apache/logging/log4j/log4j-core/2.5/log4j-core-2.5.jar
contrib/java/commons-logging/commons-logging/1.1.1/commons-logging-1.1.1.jar
contrib/java/org/slf4j/slf4j-api/1.7.12/slf4j-api-1.7.12.jar
contrib/java/joda-time/joda-time/2.5/joda-time-2.5.jar
contrib/java/commons-lang/commons-lang/2.4/commons-lang-2.4.jar
contrib/java/org/ow2/asm/asm-all/5.0.3/asm-all-5.0.3.jar
contrib/java/javax/servlet/javax.servlet-api/3.0.1/javax.servlet-api-3.0.1.jar
contrib/java/javax/annotation/jsr250-api/1.0/jsr250-api-1.0.jar
contrib/java/org/productivity/syslog4j/0.9.46/syslog4j-0.9.46.jar
contrib/java/org/easymock/easymock/3.4/easymock-3.4.jar
contrib/java/org/objenesis/objenesis/2.4/objenesis-2.4.jar
```

## ya java test-classpath
Команда, позволяющая узнать classpath, с которым будет запускаться данный Java тест. Например:
```
~/arc/arcadia/iceberg/misc/ut$ ya java test-classpath 
iceberg/misc/ut:
	iceberg/misc/ut/misc-ut.jar
	iceberg/misc/testlib/iceberg-misc-testlib.jar
	devtools/jtest/devtools-jtest.jar
	contrib/java/com/google/code/gson/gson/2.8.6/gson-gson-2.8.6.jar
	contrib/java/com/beust/jcommander/1.72/jcommander-1.72.jar
	iceberg/misc/iceberg-misc.jar
	iceberg/bolts/iceberg-bolts.jar
	contrib/java/com/google/code/findbugs/jsr305/3.0.2/findbugs-jsr305-3.0.2.jar
	iceberg/misc-bender-annotations/iceberg-misc-bender-annotations.jar
	iceberg/misc-signal/iceberg-misc-signal.jar
	contrib/java/log4j/log4j/1.2.17/log4j-1.2.17.jar
	contrib/java/org/apache/logging/log4j/log4j-api/2.11.0/log4j-api-2.11.0.jar
	contrib/java/org/apache/logging/log4j/log4j-core/2.11.0/log4j-core-2.11.0.jar
	contrib/java/commons-logging/commons-logging/1.1.1/commons-logging-1.1.1.jar
	contrib/java/org/slf4j/slf4j-api/1.7.25/slf4j-api-1.7.25.jar
	contrib/java/joda-time/joda-time/2.10.1/joda-time-2.10.1.jar
	contrib/java/org/apache/commons/commons-lang3/3.5/commons-lang3-3.5.jar
	contrib/java/org/ow2/asm/asm-all/5.0.3/asm-all-5.0.3.jar
	contrib/java/javax/servlet/javax.servlet-api/3.0.1/javax.servlet-api-3.0.1.jar
	contrib/java/org/productivity/syslog4j/0.9.46/syslog4j-0.9.46.jar
	contrib/java/org/objenesis/objenesis/2.4/objenesis-2.4.jar
	contrib/java/com/sun/activation/javax.activation/1.2.0/javax.activation-1.2.0.jar
	contrib/java/javax/annotation/javax.annotation-api/1.3.1/annotation-javax.annotation-api-1.3.1.jar
	iceberg/bolts/testlib/iceberg-bolts-testlib.jar
	contrib/java/junit/junit/4.12/junit-4.12.jar
	contrib/java/org/hamcrest/hamcrest-core/1.3/hamcrest-core-1.3.jar
	contrib/java/org/openjdk/jmh/jmh-generator-annprocess/1.19/jmh-generator-annprocess-1.19.jar
	contrib/java/org/openjdk/jmh/jmh-core/1.19/jmh-core-1.19.jar
	contrib/java/net/sf/jopt-simple/jopt-simple/4.6/jopt-simple-4.6.jar
	contrib/java/org/apache/commons/commons-math3/3.2/commons-math3-3.2.jar
	contrib/java/org/easymock/easymock/3.4/easymock-3.4.jar
	devtools/junit-runner/devtools-junit-runner.jar
```

## ya java dependency-tree
Команда печатает дерево зависимостей данного модуля с объяснением выбора конкретных версий библиотек в тех ситуациях, когда имел место конфликт. Пример использования:
```
~/arcadia/iceberg/misc$ ya java dependency-tree
iceberg/misc
|-->iceberg/bolts
|   |-->contrib/java/junit/junit/4.12
|   |   |-->contrib/java/org/hamcrest/hamcrest-core/1.3
|   |-->contrib/java/org/openjdk/jmh/jmh-generator-annprocess/1.11.2
|   |   |-->contrib/java/org/openjdk/jmh/jmh-core/1.11.2
|   |   |   |-->contrib/java/net/sf/jopt-simple/jopt-simple/4.6
|   |   |   |-->contrib/java/org/apache/commons/commons-math3/3.2
|   |-->contrib/java/com/google/code/findbugs/jsr305/3.0.0
|-->iceberg/misc-bender-annotations
|-->iceberg/misc-signal
|-->devtools/jtest
|-->contrib/java/log4j/log4j/1.2.17
|-->contrib/java/org/apache/logging/log4j/log4j-api/2.5
|-->contrib/java/org/apache/logging/log4j/log4j-core/2.5
|   |-->contrib/java/org/apache/logging/log4j/log4j-api/2.5 (*)
|-->contrib/java/commons-logging/commons-logging/1.1.1
|-->contrib/java/org/slf4j/slf4j-api/1.7.12
|-->contrib/java/joda-time/joda-time/2.5
|-->contrib/java/commons-lang/commons-lang/2.4
|-->contrib/java/org/ow2/asm/asm-all/5.0.3
|-->contrib/java/javax/servlet/javax.servlet-api/3.0.1
|-->contrib/java/javax/annotation/jsr250-api/1.0
|-->contrib/java/org/productivity/syslog4j/0.9.46
|-->contrib/java/org/easymock/easymock/3.4
|   |-->contrib/java/org/objenesis/objenesis/2.2 (omitted because of confict with 2.4)
|-->contrib/java/junit/junit/4.12 (*)
|-->contrib/java/org/objenesis/objenesis/2.4
```
