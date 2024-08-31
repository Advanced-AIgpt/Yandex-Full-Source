# Особенности сборки для мобильных

## Предварительная настройка

1. [Установить arc, ya и примонтировать аркадию](https://docs.yandex-team.ru/devtools/intro/quick-start-guide)
2. В [ya.conf](https://docs.yandex-team.ru/yatool/commands/gen_config) добавить следующие настройки:
```
use_clonefile=true  # нужно только на mac os, чинит проблему с exit code -9. https://clubs.at.yandex-team.ru/arcadia/24117

# включить распределенный yt cache для ускорения локальной сборки
yt_store = true
yt_proxy = "hahn.yt.yandex.net"
yt_dir = "//home/maps_mobile_build_cache"
```

## Сборка TestApp
Тестовое приложение карт для iOS и Android.

### Android 
#### Подготовка
##### Для mac os

1. Установить [Android Studio](https://developer.android.google.cn/studio?hl=id)
2. Выполнить в консоли:

```bash
brew install openjdk@11
echo 'export JAVA_HOME=/usr/local/opt/openjdk@11/libexec/openjdk.jdk/Contents/Home/' >> ~/.profile
yes | ~/Library/Android/sdk/tools/bin/sdkmanager --licenses
echo 'export ANDROID_HOME="~/Library/Android/sdk"' >> ~/.profile
echo 'export PATH="$PATH:$HOME/Library/Android/sdk/platform-tools/"' >> ~/.profile  # добавляет adb в PATH
. ~/.profile
```
##### Для Linux без Android Studio
1. Установить openjdk-11: `sudo apt-get install openjdk-11-jdk`. Либо удостовериться, что на машине уже установлен jdk версии 1.11.*: `java -version`
2. [Скачать Command Line Tools под linux](https://developer.android.google.cn/studio), раздел "Command line tools only".
3. Выполнить в консоли:
```bash
mkdir -p ~/Android/cmdline-tools
cd ~/Android/cmdline-tools
unzip <ПУТЬ ДО СКАЧАННОГО .zip АРХИВА>
mv cmdline-tools tools
cd ../
echo "export ANDROID_HOME=$(pwd)" >> ~/.bashrc
. ~/.bashrc
yes | $ANDROID_HOME/cmdline-tools/tools/bin/sdkmanager --licenses

```

#### Сборка из Android Studio

1. Открыть в android студии *maps/mobile/apps/test_app/android/build.gradle*.

2. Собрать maps.mobile.aar: [Справа сверху кнопка Gradle](https://wiki.yandex-team.ru/maps/dev/core/mobile/mapkit2/ya-make/.files/snimokjekrana2020-01-31v18.50.28.png), запустить android/tasks/yamake/yaMakeBuildRelease

3. Запустить сборку ▶️.

Если возникает ошибка Failed to find Build Tools revision:
Нужно нажать на синий текст ошибки и установить build tools через интерфейс Android Studio.

При возникновении непонятных ошибок (в том числе и отсутствие yaMakeBuildRelease в п.2) рекомендуется попробовать собрать из консоли.

#### Сборка из консоли

1. cd *{путь до Аркадии}/maps/mobile/apps/test_app/android*

2. Собрать maps.mobile.aar:
```
./gradlew yaMakeBuildRelease
```
3. Собрать тестапп:
```bash
./gradlew assembleRelease
```

4. Установить тестапп:
```
adb install -r ./build/intermediates/apk/release/com.yandex.maps.testapp.apk
```

apk по умолчанию собирается под одну архитектуру, выбрать нужную можно изменив значение переменной targetArch в build.gradle приложения (или прописав targetArch=\<arch\> в local.properties) и пересобрать maps.mobile.aar и тестапп.

##### Конфигурации сборки maps.mobile.aar

- `yaMakeBuildRelease`: собирает релизную сборку со всеми символами

- `yaMakeBuildReleaseWithStacktraces`: собирает релизную сборку со всеми символами и выводом имен символов в стэктрейсах

- `yaMakeBuildReleaseWithDebugInfo`: собирает релизную сборку со всеми символами и дебажной информацией. Размер библиотеки получится около 1.5ГБ

- `yaMakeBuildDebug`: собирает дебажную сборку мапкита

##### Конфигурации сборки тестаппа

Команда сборки тестаппа имеет вид: `./gradlew assemble<конфигурация>`, где:

* конфигурация: `Release` - релизная сборка, `Debug` - отладочная сборка

### iOS
#### Подготовка
##### Получение сертификата
**Этот шаг нужен, если планируется запуск приложения на реальных устройствах. Для запуска на эмуляторе сертификат не нужен.**
Сертификат ios разработчика выдаёт СИБ, для получения сертификата нужно создать задачу на в очереди [APPMARKET](https://st.yandex-team.ru/APPMARKET) с названием *"Доступ в Apple Developer Program"*.

Описание:
```
выдать доступ с "Ролью Developer" в appstoreconnect
Для работы над проектом ***** нужен доступ в Enterprise консоль Apple Store с доступом к сертификатам
Пользователь:
email: login@yandex-team.ru
```

##### Добавление сертификата iOS Distribution: Yandex LLC в цепочку ключей на локальной машине для подписи приложения
```bash
security create-keychain -p 1 mobile-signing
security unlock-keychain -p 1 mobile-signing
ya vault get version sec-01fhtkpq0jn3tfsf4cg5w0952e -o certificate.p12 > certificate.p12
security import certificate.p12 -k mobile-signing -P `ya vault get version sec-01fhtkpq0jn3tfsf4cg5w0952e -o certificate_pass` -T `which codesign`
rm certificate.p12
security set-key-partition-list -S 'apple-tool:,apple:' -k 1 mobile-signing
security set-keychain-settings -u mobile-signing
```

После перезагрузки ноутбука нужно повторно вызвать `security unlock-keychain -p 1 mobile-signing`

##### Настройка окружения
Установить [Xcode версии 12.5 и выше](https://apps.apple.com/ru/app/xcode/id497799835?l=en&mt=12).

Выполнить в консоли (одно из двух):

* Простой вариант, но после вызова `pod install` будет меняться Gemfile.lock, коммить который НЕ нужно
```bash
brew install cocoapods
```

* Сложный, но идейно правильный вариант
```bash
brew install gpg
gpg --recv-keys <актуальные ключи с https://rvm.io/>
curl -sSL https://get.rvm.io | bash -s stable
. ~/.profile
rvm install 2.7.4
cd $ARCADIA_ROOT/maps/mobile/apps/test_app/ios
gem install -g
```

#### Сборка из консоли
```bash
cd $ARCADIA_ROOT/maps/mobile/apps/test_app/ios
pod install
./build-mapkit.sh all # вместо all можно указать конкретную архитектуру - arm64 для реальных девайсов, x86_64 для эмулятора на Intel и m1sim для эмулятора на Apple Silicon
open TestApp.xcworkspace/
```
Выбрать схему StaticTestApp в верхней части окна XCode рядом с кнопкой Play. Нажать на кнопку Play

Если `pod install` завершается с ошибкой, можно попробовать вызвать `pod repo update`, а затем снова  `pod install`.

{% note warning %}

Для обновления подов (например после `arc pull`) нужно так же использовать `pod install`, НЕ `pod update`.

{% endnote %}

#### Сборка через XCode
**Сборка мапкита через xcode временно не работает.** https://st.yandex-team.ru/MAPSMOBCORE-13245

### Darwin
```bash
ya package maps/mobile/apps/test_app/bundle/darwin/pkg.json
```

### Linux
```bash
ya package maps/mobile/apps/test_app/bundle/linux/pkg.json
```

## Сборка и тестирование отдельных компонентов мапкита
Сборка происходит при помощи ya make с обязательным указанием флага --maps-mobile. Он автоматически добавляет таргетный флаг MAPSMOBI_BUILD_TARGET=yes в конфигурацию.

MAPSMOBI_BUILD_TARGET используется для кастомизации [сборочной конфигурации](https://cs.yandex-team.ru/#!MAPSMOBI_BUILD_TARGET,%5Ebuild%2F,if,arcadia,,5000), а также для [определения стартовых таргетов](https://cs.yandex-team.ru/#!MAPSMOBI_BUILD_TARGET,%5Eautocheck%2F.*ya.make%24,if,arcadia,,5000) в автосборке.

### Платформы
Карты собираются под следующий набор платформ:

**Linux**

- `ya make --maps-mobile --target-platform=default-linux-x86_64`

**Darwin**

- `ya make --maps-mobile --target-platform=xcode_12_5-darwin-x86_64`

**iOS**

- `ya make --maps-mobile --target-platform=xcode_12_5-ios-arm64`

- `ya make --maps-mobile --target-platform=xcode_12_5-ios-x86_64` (эмулятор)

**Android**

- `ya make --maps-mobile --target-platform=default-android-armv8a`

- `ya make --maps-mobile --target-platform=default-android-armv7a`

- `ya make --maps-mobile --target-platform=default-android-x86_64` (эмулятор)

- `ya make --maps-mobile --target-platform=default-android-i686` (эмулятор)


Тулчейны xcode_12* на текущий момент собраны только для mac, на хостах linux и windows они недоступны.

### Особенности сборки под iOS
Тестовый артефакт собирается в dummy iOS приложение. На данный момент поддержан только врапперпод boost тесты. С++/Objective C часть описана [тут](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/boosttest_ios_wrapper/library/ya.make?rev=7870385), инетрефейсная - [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/boosttest_ios_wrapper/boosttest_ios_wrapper/ya.make). Точка входа такого приложения уходит в бустовый мэйн. В процссе сборки используются оригинальные эпловые инструменты, которые у нас есть только под mac. Сбока и запуск тестов возможны только на данной операционной системе.

### Особеннности сборки под андроид
Андроидные тесты также собираются в dummy приложение, которое тестируется на эмуляторе.

### Релизные артефакты для мобильных платформ
Артефакты представляют собой архивы с нативным кодом под несколько архитектур. ya make за один запуск собрать такие не может, поэтому используется ya package. Пакеты описаны [тут](https://a.yandex-team.ru/arc_vcs/maps/mobile/bundle/packages).

### Запуск тестов

Тесты модуля запускаются с помощью команды:
```
ya make --maps-mobile --target-platform=<платформа> -A
```
Пример:
```
ya make --maps-mobile --target-platform=default-android-i686 -A
```

ya make пока не умеет автоматически запускать тесты на устройствах, только на эмуляторах, поэтому для запуска теста на iOS и Android необходимо сделать дополнительные шаги.

**N.B.**

Тесты под Android требуют поддержки KVM, нужно запускать их либо на "железном" хосте, либо на Mac, либо на linux ноутбуке. На QYP машинах тесты не будут работать.

#### Запуск тестов на Android устройстве

1. Подключить устройство.

2. Установить утилиту adb. Идёт в составе Android SDK или можно скачать отдельно.

3. Собрать тесты под нужную архитектуру:
```
ya make --maps-mobile --target-platform=default-android-armv8a arcadia/maps/mobile/libs/mapkit/common/tests.
```
4. Установить собранный apk с unit-test'ами:
```
adb install arcadia/maps/mobile/libs/mapkit/common/tests/com.yandex.test.unittests.apk
```
5. Очистить лог, запустить тесты, получить информацию из лога
```
adb logcat -c && adb shell am start com.yandex.test.unittests/.RunTestsActivity && adb logcat | grep -E '(yandex.maps)|(unit_tests)|(DEBUG *)'
```

#### Запуск тестов на iOS устройстве
1. Подключить устройство.

2. Залогиниться в Xcode с Apple ID.

3. Собрать тесты под нужную архитектуру c флагом BOOSTTEST_IS_FAT_OBJECT
```
ya make --maps-mobile --target-platform=xcode_12_5-ios-x86_64 -DBOOSTTEST_IS_FAT_OBJECT=yes tests
```
4. Скопировать шаблон Xcode проекта рядом с tests.a файлом:
```
cp -r ~/arcadia/maps/mobile/tools/ios-tests-template tests
```
5. Открыть Xcode workspace:
```
open tests/ios-tests-template/YMUnitTests.xcworkspace
```
6. В левом верхнем углу выбрать устройство для запуска тестов и нажать CMD + U

### О CI

#### OLD CI

https://ci.yandex-team.ru/tests/?path=maps%2Fmobile
Используется для запуска юнит тестов. Тесты запускаются до и после комита под 4 пары ос + архитектура: linux/x86_64, darwin/x86_64, iOS-симулятор/x86_64 и Android/i686. **Все тесты размечены как large(fat) и их нужно запускать вручную кнопкой.** Тесты в CI сгруппированы в рамках бандла и запускаются вместе. [См. пример в runtime](https://a.yandex-team.ru/arc/trunk/arcadia/maps/mobile/libs/runtime/tests/ya.make).

#### NEW CI

https://a.yandex-team.ru/projects/maps-core-mobile-arcadia-ci/ci/
Используется для прекоммитных проверок, автосборок приложений и релизов. Про CI можно почитать [тут](https://a.yandex-team.ru/arc_vcs/maps/mobile/ci/README.md). Про релизы - [тут](https://a.yandex-team.ru/arc_vcs/maps/mobile/docs/internal/releases.md).


#### Автосборка с помощью ya.make

Код подключается к автосборке следующим образом:

**Мобильные компоненты** - то что в `maps/mobile/{apps,libs}` собирается под ios/android/darwin/linux с флагом `--maps-mobile` и отдельно подключено к автосборке..

**Утилиты**- то что в `maps/mobile/tools` - под платформу по умолчанию без дополнительных флагов, подключен от корня.

### FAQ

**Где скачать test-app?**

[Android](https://beta.m.soft.yandex.ru/description?app=ymapstestapp&platform_shortcut=android&branch=trunk)
[iOS](https://beta.m.soft.yandex.ru/description?app=ymapstestapp&platform_shortcut=iphoneos&branch=trunk)

**Как сделать чтобы testapp ходил в testing?**

Перейти во вкладку `Experiments`, активировать `TESTING` (в iOS до переключателя нужно проскроллить страницу до самого низа) и перезагрузить приложение

**Как переключиться на Datatesting?**

Datatesting включается в testing testapp экспериментом *MAPS_CONFIG:experimental_datatesting,1*.

**Как портировать изменения из git в svn или arc?**

Например, вот так можно перенести комит с HEAD в svn или arc.
```
bash$ cd ~/mapsmobi && git diff --no-prefix HEAD~1..HEAD > /tmp/changes.diff
bash$ cd ~/arcadia/maps/mobile && patch -p0 < /tmp/changes.diff
```

*Если пути в git и в svn/arc не совпадают, то можно подправить патч вручную или использовать другое значение параметра *patch -p*.

**Как открыть сгенерировать проект для IDE**
```
ya ide clion --make-args='-DMAPSMOBI_BUILD_TARGET=yes --target-platform=xcode_12_5-darwin-x86_64'
```

**Почему тесты не запускаются *Number of suites skipped by size: 1* **

Из-за технических особенностей автосборки все тесты под Аndroid, iOS и Darwin размечены как large.

Тесты под эти платформы нужно запускать флагом *-A* или *-ttt*.

**Почему build стадия для darwin/iOS кода прошла успешно, но компиляция сломалась во время запуска тестов?**

Для улучшения производительности собираемость iOS и Darwin сборок проверяется на Linux хостах toolchain'ом default-darwin-x84_64/default-ios-*. Этот компилятор почти идентичен поставляемому с Xcode, но он "менее придирчив" к warning'ам в коде.

**Как закомитить в релизную ветку?**

https://a.yandex-team.ru/arc_vcs/maps/mobile/docs/internal/releases.md#cherri-pik-v-reliznuyu-vetku

**Где посмотреть когда соберётся test-app для beta.m.ya.ru?**

[Под android](https://a.yandex-team.ru/projects/maps-core-mobile-arcadia-ci/ci/actions/launches?dir=maps%2Fmobile&id=publish-android-apps)
[Под iOS](https://a.yandex-team.ru/projects/maps-core-mobile-arcadia-ci/ci/actions/launches?dir=maps%2Fmobile&id=publish-ios-apps)

**Если ставить дебажный TestApp на эмулятор, студия пожалуется на insufficient storage**

Это фиксится следующим образом:

   1) Tools -> AVD Manager -> выбрать эмулятор -> Edit -> Show Advanced Settings

   2) Раздуваем все, что хотим раздуть. Мне оказалось достаточно 4096 RAM, 1024 VM heap, 8192 Internal Storage. Возможно, можно обойтись меньшими значениями.

**Как символизировать крэши под android?**

   А) Воспользоваться [утилитой](https://wiki.yandex-team.ru/maps/dev/core/mobile/mapkit2/Simvolizacija-krjeshejj-v-android/)

   B) Воспользоваться сендбокс [таской](https://sandbox.yandex-team.ru/template/MAPS_CORE_MOBILE_ANDROID_SYMBOLIZER_STABLE/view)

Для запуска нужно перейти по ссылке и нажать на кнопку "Create new task"

Параметры задачи:

* `Mapkit version` - версия мапкита, которая будет использована для символизации стектрейса.
* `Stacktrace to symbolize` - стектрейс, который нужно символизировать.
* `Mapkit architecture` - архитектура мапкита.

**Как добавить платформенную Android-зависимость к MapKit?**

1. Скачать ресурс Sandbox, указанный вот в [этом](https://a.yandex-team.ru/arc/trunk/arcadia/build/external_resources/mapsmobi_maven_repo/ya.make) файле. Это ресурс с локальным Maven-репозиторием.

2. Добавить в локальный Maven-репозиторий нужные пакеты.

3. Заархивировать репозиторий и загрузить в Sandbox при помощи ya upload.

4. Поменять id ресурса в файле из п.1.

5. Указать зависимость от пакета [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/maps/mobile/tools/ya_make_helpers/aar.inc?rev=r6987790#L35).

### Полезные ссылки

- [Про утилиту ya](https://docs.yandex-team.ru/yatool/).

- [Про релизный цикл mapkit](https://wiki.yandex-team.ru/jandekskarty/testing/releasecycle/).

- [https://st.yandex-team.ru/MAPSMOBCORE-10323](https://st.yandex-team.ru/MAPSMOBCORE-10323).

- Legacy: репозиторий *git+ssh://git.yandex.ru/maps/mapsmobi.git*, [инструкция по настройке окружения](https://wiki.yandex-team.ru/jandekskarty/development/fordevelopers/mobile/infra/newdev/) и [сборка gradle build](https://wiki.yandex-team.ru/maps/dev/core/mobile/gradle-build/).
