# ya package

Основная задача хендлера - предоставление возможности хранения полной конфигурации сборки пакета в репозитории и его воспроизводимой сборки.

Команда пакетирования, позволяет собирать различные типы пакетов, описанных в специальных json-файлах, выкладывать их в Sandbox и публиковать в пакетных репозиториях.

Команда позволяет пакетировать код, собранный системой сборки ya make в различных фиксированных конфигурациях, данные из репозитория Аркадия, ресурсы из Sandbox и т.п.

Для пакетирования доступны следующие форматы, выбираемые ключом командной строки:

* `--tar` [tar-архив](tar.md) (по умолчанию)
* `--debian` [deb-пакет](deb.md)
* `--rpm` [rpm-пакет](rpm.md)
* `--docker` [docker-образ](docker.md)
* `--wheel` [Python wheel-пакет](wheel.md)
* `--aar` [aar - нативный пакет для Android](aar.md)
* `--npm` [npm - пакет для Node.js](npm.md)

Пакет можно собрать как локально, командой `ya package`, так и [специальной задачей в Sandbox](../sandbox/ya_package.md).

По умолчанию все программы указанные в package.json собираются в release, для настройки параметров сборки смотри [формат описания](json.md#build).

## С чего начать

Для того, чтобы собрать пакет, необходимо запустить команду указанием пути до json-описания и необходиого формата.
Пакет будет собран в текущей директории. Если формат пакетирования не указан, то будет собран tar-архив.

*Например*

```bash
[=] ya package devtools/dummy_arcadia/package/hello_world.json
   . . .
[=] tar -tvzf ./yandex-package-hello-world.85c7e374108166bfc1b2a47ca888830965a07708.tar.gz
drwxrwxr-x 0/0               0 2021-03-11 06:58 some_package_dir/
-rwxrwxr-x 0/0           19488 2021-03-11 06:58 some_package_dir/hello_world
```

Таким образом мы получили архив с исполняемым файлом в директории `some_package_dir` внутри.

Описание этого архива выглядит так:

{% code '/devtools/dummy_arcadia/package/hello_world.json' lang='json' %}

Собственно в описании написано что:

- Именем пакета будет `yandex-package-hello-world`
- Версией пакета будет текущая ревизия репозитория, которую система сборки определит сама.
- Для пакета надо собрать программу [devtools/dummy_arcadia/hello_world](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/dummy_arcadia/hello_world/ya.make).
- Сложить её в `some_package_dir`.

Подробнее про формат JSON-описания можно почитать на [отдельной странице](json.md).

{% note tip %}

По умолчанию `ya package` создаёт пакет в текущем каталоге, что будет приводить к лишним накладным расходом, если вы работаете c `arc`.
С помощью опций `-O/--package-output` вы можете переопределить каталог, куда `ya package` сохранит создаваемые пакеты.

{% endnote %}

## Концепция хорошего пакета
`ya package`, являясь надстройкой над системой сборки, стремится к тем же идеалам что и `ya make` - предоставляя герметичную воспроизводимую (не бинарно) сборку артефактов с минимально необходимым набором cli параметров для сборки пакета.

Внутри описания package.json указывается всё что относится к:
- Конфигурации сборки: целевые программы, сборочные флаги, платформы и прочее
- Зависимости из репозитория и ресурсы Sandbox
- Способы раскладки артефактов внутри пакета, права доступа, имена файлов/каталогов
- Метаданные относящиеся к пакету и его сборке
- Параметры пакетирования, которые неразрывно связаны с описываемым пакетом

Поэтому в большинстве случаев для сборки пакета конкретной версии, достаточно запустить `ya package <package-path>` поверх конкретной ревизии репозитория.
См. так же разделе [формат описания пакетов](json.md).

## Что ещё может ya package

Ключи для пакетирования в различных форматах были приведены выше. Что же ещё умеет команда `ya package`.

### Дополнительные параметры пакетирования { #cmdline-aux }
* `--custom-version` версия пакета, имеет более высокий приоритет над версией, указанной в файле описания
* `--strip` усечечение бинарных файлов от отладочной информации(```strip -g```)
* `--full-strip` полное учечение бинарных файлов (```strip```)
* `--create-dbg` дополнительно создает пакет с отладочной информацией (имеет смысл только в сочетании `--strip` or `--full-strip`)
* `--raw-package` позволяет не запаковываются полученные в ходе тестирования файлы в один tar-gz архив
* `--raw-package-path` путь, по которому надо сохранить сырые результаты пакетирования
* `--no-cleanup` не очищать временные директории: полезно при отладке
* `--overwrite-read-only-files` разрешение перезаписывать файлы с правами read-only при сборке пакета (такие ситуации случаются при сложных секциях `include`)
* `--dist` выполнять сборку на кластере распределенной сборки (это может сэкономить время, но и потратить квоту)
* `--yt-store` использовать распределённый сборочный кэш для ускорения локальной сборки

### Запуск тестов { #cmdline-tests }
Тесная интеграция в систему сборки позволяет не только собрать артефакты для пакетирования, но и протестировать их.
* `-A, --run-all-tests` прогонять все тесты при сборке целей из секции [build](json.md#build)
* `-t, --run-tests` прогонять только быстрые тесты при сборке целей из секции [build](json.md#build)
* `--ignore-fail-tests` игнорировать падения тестов при сборке пакетов (в противном случае, сборка пакета прекратится)

### Публикация и загрузка результатов пакетирования в Sandbox / MDS { #cmdline-upload }
Полученный пакет можно сразу опубликовать в пакетном репозитории или выложить в хранилище Sandbox / MDS
* `--publish-to=<repo_url>` опубликовать пакет в соотвествующем пакетном репозитории.
* `--ensure-package-published` проверить доступность пакета после публикации
* `--upload` загрузить полученный пакет в виде ресурса [Sandbox](https://sandbox.yandex-team.ru)
* `--upload-resource-type` тип загружаемого ресурса (по умолчанию YA_PACKAGE)
* `--upload-resource-attr` дополнительные атрибуты ресурса Sandbox
* `--owner` владелец (Owner) ресурса
* `--mds` загрузить полученные результат в mds
* `--ssh-key` путь к приватной части ключа для получения токена для заливки пакета в Sandbox
* `--token` OAuth-токен для авторизации в Сандбоксе (можно указывать напрямую без `--ssh-key`)
* `--user` имя пользователя для авторизации (по умолчанию – текущий пользователь)
* `--artifactory` опубликовать пакет в `artifactory`. [Подробнее](artifactory.md)


### Все ключи командной строки { #cmd-line-all }

Кроме указанных выше, поддерживается широкий набор параметров сборки и тестирования

{% cut "ya package --help" %}

```bash
Usage: ya package [OPTION]... [PACKAGE DESCRIPTION FILE NAME(S)]...

Build package using json package description in the release build type by default.
For more info see https://docs.yandex-team.ru/ya-make/usage/ya_package

Examples:
  ya package <path to json description>  Create tarball package from json description


Options:
  Bullet-proof options
    -d                  Debug build
    -r                  Release build
    --sanitize=SANITIZE Sanitizer type(address, memory, thread, undefined, leak)
    --sanitizer-flag=SANITIZER_FLAGS
                        Additional flag for sanitizer
    --lto               Build with LTO
    --thinlto           Build with ThinLTO
    --sanitize-coverage=SANITIZE_COVERAGE
                        Enable sanitize coverage
    --afl               Use AFL instead of libFuzzer
    --musl              Build with musl-libc
    --pch               Build with Precompiled Headers
    --hardening         Build with hardening
    --race              Build Go projects with race detector
    --cuda=CUDA_PLATFORM
                        Cuda platform(optional, required, disabled) (default: optional)
    -j=BUILD_THREADS, --threads=BUILD_THREADS
                        Build threads count (default: 50)
    --checkout          Checkout missing dirs
    --report-config=REPORT_CONFIG_PATH
                        Set path to TestEnvironment report config
    -o=OUTPUT_ROOT, --output=OUTPUT_ROOT
                        Directory with build results
    --no-output-for=SUPPRESS_OUTPUTS
                        Do not symlink/copy output for files with given suffix, they may still be save in cache as result
    -h, --help          Print help
    -v, --verbose       Be verbose
    --ttl=TTL           Resource TTL in days (pass 'inf' - to mark resource not removable) (default: 14)
  Testing options
   Run tests
    -t, --run-tests     Run tests (-t runs only SMALL tests, -tt runs SMALL and MEDIUM tests, -ttt runs SMALL, MEDIUM and FAT tests)
    -A, --run-all-tests Run test suites of all sizes
    --add-peerdirs-tests=PEERDIRS_TEST_TYPE
                        Peerdirs test types (none, gen, all) (default: none)
    --test-threads=TEST_THREADS
                        Restriction on concurrent tests (no limit by default) (default: 0)
    --fail-fast         Fail after the first test failure
    -L, --list-tests    List tests
   Filtering
    -X, --last-failed-tests
                        restart tests which failed in last run for chosen target
    -F=TESTS_FILTERS, --test-filter=TESTS_FILTERS
                        Run only tests that match <tests-filter> or chunks that match "[.*?] chunk"
    --test-size=TEST_SIZE_FILTERS
                        Run only specified set of tests
    --test-type=TEST_TYPE_FILTERS
                        Run only specified types of tests
    --style             Run only style tests and implies --strip-skipped-test-deps (classpath.clash clang_tidy flake8.py2 flake8.py3 gofmt govet java.style)
    --regular-tests     Run only regular tests (boost_test exectest fuzz g_benchmark go_bench go_test gtest java py2test py3test pytest unittest)
    --test-tag=TEST_TAGS_FILTER
                        Run tests that have specified tag
    --test-filename=TEST_FILES_FILTER
                        Run only tests with specified filenames (pytest only)
    --test-size-timeout=TEST_SIZE_TIMEOUTS
                        Set test timeout for each size (small=60, medium=600, large=3600)
   Console report
    -P, --show-passed-tests
                        Show passed tests
    --show-skipped-tests
                        Show skipped tests
    --inline-diff       Disable truncation of the comments and print diff to the terminal
    --show-metrics      Show metrics on console (You need to add "-P" option to see metrics for the passed tests)
   Linters
    --disable-flake8-migrations
                        Enable all flake8 checks
    --disable-jstyle-migrations
                        Enable all java style checks
   Canonization
    -Z, --canonize-tests
                        Canonize selected tests
    --canonize-via-skynet
                        use skynet to upload big canonical data
    --canonize-via-http use http to upload big canonical data
    --canon-diff=TEST_DIFF
                        Show test canonical data diff, allowed values are r<revision>, rev1:rev2, HEAD, PREV
   Debugging
    --pdb               On error debug in pdb
    --gdb               Run c++ unittests in gdb
    --dlv               Run go unittests in dlv
    --tests-retries=TESTS_RETRIES
                        Run every test specified number of times (default: 1)
    --no-random-ports   Use requested ports
    --test-stderr       Output test stderr to console online
    --test-stdout       Output test stdout to console online
    --test-disable-timeout
                        Turn off timeout for tests (only for local runs, incompatible with --cache-tests, --dist)
    --test-debug        Test debug mode (prints test pid after launch and implies --test-threads=1 --test-disable-timeout --retest --test-stderr)
    --disable-test-graceful-shutdown
                        Test node will be killed immediately after the timeout
    --test-binary-args=TEST_BINARY_ARGS
                        Throw args to test binary
   Runtime environment
    --test-param=TEST_PARAMS
                        Arbitrary parameters to be passed to tests (name=val)
    --private-ram-drive Creates a private ram drive for all test nodes requesting one
   Test uid calculation
    --cache-tests       Use cache for tests
    --retest            No cache for tests
   Test dependencies
    --strip-skipped-test-deps
                        Don't build skipped test's dependencies
    --build-only-test-deps
                        Build only targets required for requested tests
   File reports
    --allure=ALLURE_REPORT (deprecated)
                        Path to allure report to be generated
    --junit=JUNIT_PATH  Path to junit report to be generated
   Test outputs
    --no-test-outputs   Don't save testing_out_stuff
    --no-dir-outputs (deprecated)
                        Tar testing output dir in the intermediate machinery
    --keep-full-test-logs
                        Don't truncate logs on distbuild
    --test-keep-symlinks
                        Don't delete symlinks from test output
   Tests over YT
    --run-tagged-tests-on-yt
                        Run tests marked with ya:yt tag on the YT
    --ytexec-bin=YTEXEC_BIN
                        use local ytexec binary
    --ytexec-run        Use ytexec tool to run YT vanilla operations (should be used with "--run-tagged-tests-on-yt")
   Tests over Sandbox
    --run-tagged-tests-on-sandbox
                        Run tests marked with ya:force_sandbox tag on the Sandbox
   Coverage
    --coverage (deprecated)
                        Collect coverage information. (deprecated alias for "--gcov --java-coverage --python-coverage --coverage-report")
    --coverage-prefix-filter=COVERAGE_PREFIX_FILTER
                        Inspect only matched paths
    --coverage-exclude-regexp=COVERAGE_EXCLUDE_REGEXP
                        Exclude matched paths from coverage report
    --coverage-report-path=COVERAGE_REPORT_PATH
                        Path inside output dir where to store gcov cpp coverage report (use with --output)
    --python-coverage   Collect python coverage information
    --go-coverage       Collect go coverage information
    --java-coverage     Collect java coverage information
    --merge-coverage    Merge all resolved coverage files to one file
    --sancov            Collect sanitize coverage information (automatically increases tests timeout at 1.5 times)
    --clang-coverage    Clang's source based coverage (automatically increases tests timeout at 1.5 times)
    --fast-clang-coverage-merge
                        Merge profiles in the memory in test's runtime using fuse
    --coverage-report   Build HTML coverage report (use with --output)
    --enable-java-contrib-coverage
                        Add sources and classes from contib/java into jacoco report
    --enable-contrib-coverage
                        Build contrib with coverage options and insert coverage.extractor tests for contrib binaries
    --nlg-coverage      Collect Alice's NLG coverage information
   Fuzzing
    --fuzzing           Extend test's corpus. Implies --sanitizer-flag=-fsanitize=fuzzer
    --fuzz-opts=FUZZ_OPTS
                        Space separated string of fuzzing options (default: )
    --fuzz-case=FUZZ_CASE_FILENAME
                        Specify path to the file with data for fuzzing (conflicting with "--fuzzing")
    --fuzz-minimization-only
                        Allows to run minimization without fuzzing (should be used with "--fuzzing")
   Pytest specific
    --test-log-level=TEST_LOG_LEVEL
                        Specifies logging level for output test logs ("critical", "error", "warning", "info", "debug")
    --test-traceback=TEST_TRACEBACK
                        Test traceback style for pytests ("long", "short", "line", "native", "no") (default: short)
    --profile-pytest    Profile pytest calls
   Java tests specific
    --jstyle-runner-path=JSTYLE_RUNNER_PATH
                        Path to custom runner for java style tests
    -R=PROPERTIES, --system-property=PROPERTIES
                        Set system property (name=val)
    --system-properties-file=PROPERTIES_FILES
                        Load system properties from file
    --jvm-args=JVM_ARGS Add jvm args for jvm launch
   Developer options
    --test-tool-bin=TEST_TOOL_BIN
                        Path to test_tool binary
    --test-tool3-bin=TEST_TOOL3_BIN
                        Path to test_tool3 binary
    --profile-test-tool=PROFILE_TEST_TOOL
                        Profile specified test_tool handlers
    --split-factor=TESTING_SPLIT_FACTOR
                        Redefines SPLIT_FACTOR(X) (default: 0)
  Package options
   Common
    --no-cleanup        Do not clean the temporary directory
    --change-log=CHANGE_LOG
                        Change log text or path to the existing changelog file
    --publish-to=PUBLISH_TO
                        Publish package to the specified dist
    --strip             Strip binaries (only debug symbols: "strip -g")
    --full-strip        Strip binaries
    --key=KEY           The key to use for signing
    --ignore-fail-tests Create package, no matter tests failed or not
    --new               Use new ya package json format
    --old               Use old ya package json format
    --custom-version=CUSTOM_VERSION
                        Custom package version
    --tests-data-root=CUSTOM_TESTS_DATA_ROOT
                        Custom location for arcadia_tests_data dir, defaults to <source root>/../arcadia_tests_data
    --data-root=CUSTOM_DATA_ROOT
                        Custom location for data dir, defaults to <source root>/../data
    --upload            Upload created package to sandbox
    --nanny-release=NANNY_RELEASE
                        Notify nanny about new release
    --overwrite-read-only-files
                        Overwrite read-only files in package
    --dump-arcadia-inputs=DUMP_INPUTS
                        Only dump inputs, do not build package
    --ensure-package-published
                        Ensure that package is available in the repository
    -O=PACKAGE_OUTPUT, --package-output=PACKAGE_OUTPUT
                        Specifies directory for package output
   Tar
    --tar               Build tarball package
    --no-compression    Don't compress tar archive (for --tar only)
    --create-dbg        Create separate package with debug info (works only in case of --strip or --full-strip)
    --compression-filter=COMPRESSION_FILTER
                        Specifies compression filter (gzip/zstd)
    --compression-level=COMPRESSION_LEVEL
                        Specifies compression level (0-9 for gzip [6 is default], 0-22 for zstd [3 is default])
    --raw-package       Used with --tar to get package content without tarring
    --raw-package-path=RAW_PACKAGE_PATH
                        Custom path for raw-package (implies --raw-package)
    --codec=CODEC       Codec name for uc compression
    --codecs-list       Show available codecs for --uc
   Debian
    --debian            Build debian package
    --not-sign-debian   Do not sign debian package
    --debian-distribution=DEBIAN_DISTRIBUTION
                        Debian distribution (default: unstable)
    --debian-arch=DEBIAN_ARCH
                        Debian arch (passed to debuild as `-a`)
    --arch-all          Use "Architecture: all" in debian
    --force-dupload     dupload --force
    --build-debian-scripts
    -z=DEBIAN_COMPRESSION_LEVEL, --debian-compression=DEBIAN_COMPRESSION_LEVEL
                        deb-file compresson level (none, low, medium, high)
    -Z=DEBIAN_COMPRESSION_TYPE, --debian-compression-type=DEBIAN_COMPRESSION_TYPE
                        deb-file compression type used when building deb-file (allowed types: gzip, xz, bzip2, lzma, none)
    --dupload-max-attempts=DUPLOAD_MAX_ATTEMPTS
                        How many times try to run dupload if it fails (default: 1)
    --dupload-no-mail   dupload --no-mail
   Docker
    --docker            Build docker
    --docker-registry=DOCKER_REGISTRY
                        Docker registry (default: registry.yandex.net)
    --docker-repository=DOCKER_REPOSITORY
                        Specify private repository (default: )
    --docker-save-image Save docker image to archive
    --docker-push       Push docker image to registry
    --docker-network=DOCKER_BUILD_NETWORK
                        --network parameter for `docker build` command
    --docker-build-arg=DOCKER_BUILD_ARG
                        --build-arg parameter for `docker build` command, set it in the <key>=<value> form
   Aar
    --aar               Build aar package
   Rpm
    --rpm               Build rpm package
   Npm
    --npm               Build npm package
   Python wheel
    --wheel             Build wheel package
    --wheel-repo-access-key=WHEEL_ACCESS_KEY_PATH
                        Path to access key for wheel repository
    --wheel-repo-secret-key=WHEEL_SECRET_KEY_PATH
                        Path to secret key for wheel repository
    --wheel-python3     use python3 when building wheel package
  Advanced options
    --build=BUILD_TYPE  Build type (debug, release, profile, gprof, valgrind, valgrind-release, coverage, relwithdebinfo, minsizerel, debugnoasserts, fastdebug) https://wiki.yandex-team.ru/yatool/build-types (default: release)
    --link-threads=LINK_THREADS
                        Link threads count (default: 0)
    --host-build-type=HOST_BUILD_TYPE
                        Host platform build type (debug, release, profile, gprof, valgrind, valgrind-release, coverage, relwithdebinfo, minsizerel, debugnoasserts, fastdebug) https://wiki.yandex-team.ru/yatool/build-types (default: release)
    --host-platform=HOST_PLATFORM
                        Host platform
    --host-platform-flag=HOST_PLATFORM_FLAGS
                        Host platform flag
    --c-compiler=C_COMPILER
                        Specifies path to the custom compiler for the host and target platforms
    --cxx-compiler=CXX_COMPILER
                        Specifies path to the custom compiler for the host and target platforms
    --target-platform=TARGET_PLATFORMS
                        Target platform
    --target-platform-build-type=TARGET_PLATFORM_BUILD_TYPE
                        Set build type for the last target platform
    --target-platform-release
                        Set release build type for the last target platform
    --target-platform-debug
                        Set debug build type for the last target platform
    --target-platform-tests
                        Run tests for the last target platform
    --target-platform-test-size=TARGET_PLATFORM_TEST_SIZE
                        Run tests only with given size for the last target platform
    --target-platform-test-type=TARGET_PLATFORM_TEST_TYPE
                        Run tests only with given type for the last target platform
    --target-platform-regular-tests
                        Run only "boost_test exectest fuzz g_benchmark go_bench go_test gtest java py2test py3test pytest unittest" test types for the last target platform
    --target-platform-flag=TARGET_PLATFORM_FLAG
                        Set build flag for the last target platform
    --target-platform-c-compiler=TARGET_PLATFORM_COMPILER
                        Specifies path to the custom compiler for the last target platform
    --target-platform-cxx-compiler=TARGET_PLATFORM_COMPILER
                        Specifies path to the custom compiler for the last target platform
    --universal-binaries
                        Generate multiplatform binaries
    --lipo              Generate multiplatform binaries with lipo
    --rebuild           Rebuild all
    --strict-inputs     Enable strict mode
    --build-results-report=BUILD_RESULTS_REPORT_FILE
                        Dump build report to file in the --output-dir
    --build-results-report-tests-only
                        Report only test results in the report
    --build-report-type=BUILD_REPORT_TYPE
                        Build report type(canonical, human_readable) (default: canonical)
    --build-results-resource-id=BUILD_RESULTS_RESOURCE_ID
                        Id of sandbox resource id containing build results
    --use-links-in-report
                        Use links in report instead of local paths
    --report-skipped-suites
                        Report skipped suites
    --report-skipped-suites-only
                        Report only skipped suites
    --dump-raw-results  Dump raw build results to the output root
    --no-local-executor Use Popen instead of local executor
    --use-clonefile     Use clonefile instead of hardlink on macOS
    --force-use-copy-instead-hardlink-macos-arm64
                        Use copy instead hardlink when clonefile is unavailable
    --force-build-depends
                        Build by DEPENDS anyway
    --ignore-recurses   Do not build by RECURSES
    -S=CUSTOM_SOURCE_ROOT, --source-root=CUSTOM_SOURCE_ROOT
                        Custom source root (autodetected by default)
    -B=CUSTOM_BUILD_DIRECTORY, --build-dir=CUSTOM_BUILD_DIRECTORY
                        Custom build directory (autodetected by default)
    --html-display=HTML_DISPLAY
                        Alternative output in html format
    --tools-cache-size=TOOLS_CACHE_SIZE
                        Max tool cache size (default: 30GiB)
    --cache-size=CACHE_SIZE
                        Max cache size (default: 140GiB)
    -D=FLAGS            Set variables (name[=val], "yes" if val is omitted)
    --pgo-add           Create PGO profile
    --pgo-use=PGO_USER_PATH
                        PGO profiles path
    --share-results     Share results with skynet
  Developers options
    --log-file=LOG_FILE Append verbose log into specified file
    --evlog-file=EVLOG_FILE
                        Dump event log into specified file
    --no-evlogs         Disable standard evlogs in YA_CACHE_DIR
    --evlog-dump-platform
                        Add platform in event message
    --keep-temps        Do not remove temporary build roots. Print test's working directory to the stderr (use --test-stderr to make sure it's printed at the test start)
    --dist              Run on distbuild
    -G, --dump-graph    Dump full build graph to stdout
    --dump-json-graph   Dump full build graph as json to stdout
    -x=DEBUG_OPTIONS, --dev=DEBUG_OPTIONS
                        ymake debug options
    --vcs-file=VCS_FILE Provides VCS file
    --dump-files-path=DUMP_FILE_PATH
                        Put extra ymake dumps into specified directory
    --ymake-bin=YMAKE_BIN
                        Path to ymake binary
    --no-ymake-resource Do not use ymake binary as part of build commands
    --no-ya-bin-resource
                        Do not use ya-bin binary as part of build commands
    --cache-stat        Show cache statistics
    --gc                Remove all cache except uids from the current graph
    --gc-symlinks       Remove all symlink results except files from the current graph
    --symlinks-ttl=SYMLINKS_TTL
                        Results cache TTL (default: 168.0h)
    --yt-proxy=YT_PROXY YT storage proxy (default: hahn.yt.yandex.net)
    --yt-dir=YT_DIR     YT storage cypress directory pass (default: //home/devtools/cache)
    --yt-token=YT_TOKEN YT token
    --yt-token-path=YT_TOKEN_PATH
                        YT token path (default: /home/prettyboy/.yt/token)
    --yt-put            Upload to YT store
    --yt-max-store-size=YT_MAX_CACHE_SIZE
                        YT storage max size
    --yt-store-ttl=YT_STORE_TTL
                        YT store ttl in hours(0 for infinity) (default: 24)
    --yt-store          Use YT storage
    --no-yt-store       Disable YT storage
    --yt-create-tables  Create YT storage tables
    --yt-store-filter=YT_CACHE_FILTER
                        YT store filter
    --yt-store-codec=YT_STORE_CODEC
                        YT store codec
    --yt-store-threads=YT_STORE_THREADS
                        YT store max threads (default: 25)
    --yt-store-refresh-on-read
                        On read mark cache items as fresh (simulate LRU)
    --yt-replace-result Build only targets that need to be uploaded to the YT store
    --yt-replace-result-add-objects
                        Tune yt-replace-result option: add objects (.o) files to build results. Useless without --yt-replace-result
    --yt-replace-result-rm-binaries
                        Tune yt-replace-result option: remove all non-tool binaries from build results. Useless without --yt-replace-result
    --dump-distbuild-result=DUMP_DISTBUILD_RESULT
                        Dump result returned by distbuild (default: False)
    --build-time=BUILD_EXECUTION_TIME
                        Set maximum build execution time (in seconds)
    -E, --download-artifacts
                        Download build artifacts when using distributed build
    --distbuild-pool=DISTBUILD_POOL
                        Execution pool in DistBuild (default: )
  Legacy options
    --results-root=OUTPUT_ROOT
                        Alias for --output
  Sandbox upload options
    --upload-resource-type=RESOURCE_TYPE
                        Created resource type (default: YA_PACKAGE)
    --upload-resource-attr=RESOURCE_ATTRS
                        Resource attr, set it in the <name>=<value> form
    --owner=RESOURCE_OWNER
                        User name to own data saved to sandbox. Required in case of inf ttl of resources in mds.
    --sandbox-url=SANDBOX_URL
                        sandbox url to use for storing canonical file (default: https://sandbox.yandex-team.ru)
    --task-kill-timeout=TASK_KILL_TIMEOUT
                        Timeout in seconds for sandbox uploading task
    --sandbox           Upload to Sandbox
    --skynet            Should upload using skynet
    --http              Should upload using http
    --sandbox-mds       Should upload using MDS as storage
  MDS upload options
    --mds               Upload to MDS
    --mds-host=MDS_HOST MDS Host (default: storage.yandex-team.ru)
    --mds-port=MDS_PORT MDS Port (default: 80)
    --mds-namespace=MDS_NAMESPACE
                        MDS namespace (default: devtools)
    --mds-token=MDS_TOKEN
                        MDS Basic Auth token
  Authorization options
    --ssh-key=SSH_KEYS  Path to private ssh key to exchange for OAuth token
    --token=OAUTH_TOKEN oAuth token
    --user=USERNAME     Custom user name for authorization
    --ssh-key=SSH_KEYS  Path to private ssh key to exchange for OAuth token
```
{% endcut %}
