# Справочник по опциям ya make

`ya make [OPTION]... [TARGET]...`

## Опции
{% cut "Основные опции" %}
```
    -q, --quiet         Checkout silently (for svn)
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
    --report-config=REPORT_CONFIG_PATH
                        Set path to TestEnvironment report config
    -k, --keep-going    Build as much as possible
    -v, --verbose       Be verbose
    -T                  Do not rewrite output information (ninja/make)
    --do-not-output-stderrs
                        Do not output any stderrs
    -h, --help          Print help
    -j=BUILD_THREADS, --threads=BUILD_THREADS
                        Build threads count (default: 32)
    --clear             Clear temporary data
    --no-src-links      Do not create any symlink in source directory
    --ttl=TTL           Resource TTL in days (pass 'inf' - to mark resource not removable) (default: 14)
    -o=OUTPUT_ROOT, --output=OUTPUT_ROOT
                        Directory with build results
    --no-output-for=SUPPRESS_OUTPUTS
                        Do not symlink/copy output for files with given suffix, they may still be save in cache as result
    --checkout          Checkout missing dirs
```
{% endcut %}

{% cut "Опции запуска тестов" %}
```
    --coverage          Collect coverage information (alias for "--gcov --java-coverage --python-coverage --coverage-report")
    --gcov              Collect gcov coverage information (automatically increases tests timeout at 1.5 times)
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
    --coverage-report   Build coverage report (use with --output)
    --upload-coverage   Upload collected coverage to the YT
    --enable-java-contrib-coverage
                        Add sources and classes from contib/java into jacoco report
    --enable-contrib-coverage
                        Build contrib with coverage options and insert coverage.extractor tests for contrib binaries
    --nlg-coverage      Collect Alice's NLG coverage information
    -t, --run-tests     Run tests (-t runs only SMALL tests, -tt runs SMALL and MEDIUM tests, -ttt runs SMALL, MEDIUM and FAT tests)
    -A, --run-all-tests Run test suites of all sizes
    --add-peerdirs-tests=PEERDIRS_TEST_TYPE
                        Peerdirs test types (none, gen, all) (default: none)
    --test-threads=TEST_THREADS
                        Restriction on concurrent tests (no limit by default) (default: 0)
    --pdb               On error debug in pdb
    --gdb               Run c++ unittests in gdb
    -X, --last-failed-tests
                        restart tests which failed in last run for chosen target
    --fail-fast         Fail after the first test failure
    -P, --show-passed-tests
                        Show passed tests
    --show-skipped-tests
                        Show skipped tests
    -F=TESTS_FILTERS, --test-filter=TESTS_FILTERS
                        Run only tests that match <tests-filter>
    --test-size=TEST_SIZE_FILTERS
                        Run only specified set of tests
    --test-type=TEST_TYPE_FILTERS
                        Run only specified types of tests
    --test-tag=TEST_TAGS_FILTER
                        Run tests that have specified tag
    --test-filename=TEST_FILES_FILTER
                        Run only tests with specified filenames (pytest only)
    --test-env=TEST_ENV Pass env varable key[=value] to tests. Gets value from system env if not set
    --cache-tests       Use cache for tests
    --retest            No cache for tests
    -L, --list-tests    List tests
    -Z, --canonize-tests
                        Canonize selected tests
    --canonize-via-skynet
                        use skynet to upload big canonical data
    --canonize-via-http use http to upload big canonical data
    --junit=JUNIT_PATH  Path to junit report to be generated
    --jstyle-runner-path=JSTYLE_RUNNER_PATH
                        Path to custom runner for java style tests
    --inline-diff       Don't extract diffs to files. Disable truncation of the comments printed on the terminal
    --tests-retries=TESTS_RETRIES
                        Run every test specified number of times (default: 1)
    --no-random-ports   Use requested ports
    --dont-merge-split-tests
                        Don't merge split tests testing_out_stuff dir (with macro FORK_*TESTS)
    --test-stderr       Output test stderr to console online
    --test-stdout       Output test stdout to console online
    --test-log-level=TEST_LOG_LEVEL
                        Specifies logging level for output test logs ("critical", "error", "warning", "info", "debug")
    --test-traceback=TEST_TRACEBACK
                        Test traceback style for pytests ("long", "short", "line", "native", "no") (default: short)
    --allure=ALLURE_REPORT
                        Path to allure report to be generated
    --canon-diff=TEST_DIFF
                        Show test canonical data diff, allowed values are r<revision>, rev1:rev2, HEAD, PREV
    --test-disable-timeout
                        Turn off timeout for tests (only for local runs, incompatible with --cache-tests, --dist)
    --show-metrics      Show metrics on console (You need to add "-P" option to see metrics for the passed tests)
    -R=PROPERTIES, --system-property=PROPERTIES
                        Set system property (name=val)
    --system-properties-file=PROPERTIES_FILES
                        Load system properties from file
    --jvm-args=JVM_ARGS Add jvm args for jvm launch
    --test-keep-symlinks
                        Don't delete symlinks from test output
    --backup-test-results
                        Backup test results on the distbuild
    --no-test-outputs   Don't save testing_out_stuff
    --test-debug        Test debug mode
    --test-failure-code=TEST_FAIL_EXIT_CODE
                        Exit code when tests fail (default: 10)
    --fuzz-opts=FUZZ_OPTS
                        Space separated string of fuzzing options (default: )
    --fuzz-case=FUZZ_CASE_FILENAME
                        Specify path to the file with data for fuzzing
    --fuzz-minimization-only
                        Allows to run minimization without fuzzing
    --fuzzing           Extend test's corpus
    --profile-pytest    Profile pytest calls
    --test-param=TEST_PARAMS
                        Arbitrary parameters to be passed to tests (name=val)
    --keep-full-test-logs
                        Don't truncate logs on distbuild
    --strip-skipped-test-deps
                        Strip skipped test's deps
    --test-size-timeout=TEST_SIZE_TIMEOUTS
                        Set test timeout for each size (small=60, medium=600, large=3600)
    --disable-test-graceful-shutdown
                        Test node will be killed immediately after the timeout
    --run-tagged-tests-on-yt
                        Run tests marked with ya:yt tag on the YT
    --run-tagged-tests-on-sandbox
                        Run tests marked with ya:force_sandbox tag on the Sandbox
    --private-ram-drive Creates a private ram drive for all test nodes requesting one
    --test-tool-bin=TEST_TOOL_BIN
                        Path to test_tool binary
    --test-tool3-bin=TEST_TOOL3_BIN
                        Path to test_tool3 binary
    --profile-test-tool=PROFILE_TEST_TOOL
                        Profile specified test_tool calls
```
{% endcut %}

{% cut "Расширенные опции" %}
```
    --build=BUILD_TYPE  Build type (debug, release, profile, gprof, valgrind, valgrind-release, coverage, relwithdebinfo, minsizerel, debugnoasserts, fastdebug) https://wiki.yandex-team.ru/yatool/build-types (default: debug)
    -D=FLAGS            Set variables (name[=val], "yes" if val is omitted)
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
    -C=BUILD_TARGETS, --target=BUILD_TARGETS
                        Targets to build
    --stat              Show additional statistics
    --stat-dir=STATISTICS_OUT_DIR
                        Additional statistics output dir
    --mask-roots        Mask source and build root paths in stderr
    -S=CUSTOM_SOURCE_ROOT, --source-root=CUSTOM_SOURCE_ROOT
                        Custom source root (autodetected by default)
    -B=CUSTOM_BUILD_DIRECTORY, --build-dir=CUSTOM_BUILD_DIRECTORY
                        Custom build directory (autodetected by default)
    --misc-build-info-dir=MISC_BUILD_INFO_DIR
                        Directory for miscellaneous build files (build directory by default)
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
                        Run only "boost_test exectest fuzz go_test gtest java junit py2test py3test pytest testng unittest" test types for the last target platform
    --target-platform-flag=TARGET_PLATFORM_FLAG
                        Set build flag for the last target platform
    --target-platform-c-compiler=TARGET_PLATFORM_COMPILER
                        Specifies path to the custom compiler for the last target platform
    --target-platform-cxx-compiler=TARGET_PLATFORM_COMPILER
                        Specifies path to the custom compiler for the last target platform
    --universal-binaries
                        Generate multiplatform binaries
    --lipo              Generate multiplatform binaries with lipo
    --show-command=SHOW_COMMAND
                        Print command for selected build output
    --add-result=ADD_RESULT
                        Process selected build output as a result
    --add-protobuf-result
                        Process protobuf output as a result
    --add-flatbuf-result
                        Process flatbuf output as a result
    --show-timings      Print execution time for commands
    --show-extra-progress
                        Print extra progress info
    --replace-result    Build only --add-result targets
    --add-modules-to-results
                        Process all modules as results
    --no-local-executor Use Popen instead of local executor
    --force-build-depends
                        Build by DEPENDS anyway
    --ignore-recurses   Do not build by RECURSES
    --share-results     Share results with skynet
    -I=INSTALL_DIR, --install=INSTALL_DIR
                        Path to accumulate resulting binaries and libraries
    --html-display=HTML_DISPLAY
                        Alternative output in html format
    --teamcity          Generate additional info for teamcity
    --tools-cache-size=TOOLS_CACHE_SIZE
                        Max tool cache size (default: 30GiB)
    --cache-size=CACHE_SIZE
                        Max cache size (default: 300GiB)
    --pgo-add           Create PGO profile
    --pgo-use=PGO_USER_PATH
                        PGO profiles path
    --pic               Force PIC mode
    --report-only-stages
                        Dump raw build results to the output root
    --maps-mobile       Enable mapsmobi configuration preset
    --profile=PROFILE_TO_FILE
                        Write profile info to file
    --stages=STAGES_PROFILE
                        Write stages info to file
    -x=DEBUG_OPTIONS, --dev=DEBUG_OPTIONS
                        ymake debug options
    --vcs-file=VCS_FILE Provides VCS file
    --dump-files-path=DUMP_FILE_PATH
                        Put extra ymake dumps into specified directory
    --dev-conf=CONF_DEBUG_OPTIONS
                        Configure step debug options: list-files, print-commands, force-run, verbose-run
    --ymake-bin=YMAKE_BIN
                        Path to ymake binary
    --no-ymake-resource Do not use ymake binary as part of build commands
    --no-ya-bin-resource
                        Do not use ya-bin binary as part of build commands
    --do-not-use-local-conf
                        Do not use local configuration files
    --local-conf-path=LOCAL_CONF_PATH
                        Path to <local.ymake>
    --build-custom-json=CUSTOM_JSON
                        Build custom graph specified by file name
    --custom-context=CUSTOM_CONTEXT
                        Use custom context specified by file name (requires additionally passing --build-custom-json)
    -G, --dump-graph    Dump full build graph to stdout
    --dump-json-graph   Dump full build graph as json to stdout
    -M, --makefile      Generate Makefile
    --dump-distbuild-result=DUMP_DISTBUILD_RESULT
                        Dump result returned by distbuild (default: False)
    --build-time=BUILD_EXECUTION_TIME
                        Set maximum build execution time (in seconds)
    -E, --download-artifacts
                        Download build artifacts when using distributed build
    --dist              Run on distbuild
    --keep-temps        Do not remove temporary build roots. Print test's working directory to the stderr (use --test-stderr to make sure it's printed at the test start)
    --profile-to=PROFILE_TO
                        Run with cProfile
    --log-file=LOG_FILE Append verbose log into specified file
    --evlog-file=EVLOG_FILE
                        Dump event log into specified file
    --no-evlogs         Disable standard evlogs in YA_CACHE_DIR
    --evlog-dump-platform
                        Add platform in event message
    --cache-stat        Show cache statistics
    --gc                Remove all cache except uids from the current graph
    --gc-symlinks       Remove all symlink results except files from the current graph
    --symlinks-ttl=SYMLINKS_TTL
                        Results cache TTL (default: 168.0h)
    --yt-store          Use YT storage
    --yt-proxy=YT_PROXY YT storage proxy (default: hahn.yt.yandex.net)
    --yt-dir=YT_DIR     YT storage cypress directory pass (default: //home/devtools/cache)
    --yt-token=YT_TOKEN YT token
    --yt-token-path=YT_TOKEN_PATH
                        YT token path (default: /home/spreis/.yt/token)
    --yt-put            Upload to YT store
    --yt-create-tables  Create YT storage tables
    --yt-max-store-size=YT_MAX_CACHE_SIZE
                        YT storage max size
    --yt-store-filter=YT_CACHE_FILTER
                        YT store filter
    --yt-store-ttl=YT_STORE_TTL
                        YT store ttl in hours(0 for infinity) (default: 24)
    --yt-store-codec=YT_STORE_CODEC
                        YT store codec
    --yt-replace-result Build only targets that need to be uploaded to the YT store
    --yt-store-threads=YT_STORE_THREADS
                        YT store max threads (default: 3)
    --raw-params=RAW_PARAMS
                        Params dict as json encoded with base64
```
{% endcut %}

{% cut "Опции Java-сборки" %}
```
    --sonar             Analyze code with sonar.
    --sonar-project-filter=SONAR_PROJECT_FILTERS
                        Analyze only projects that match any filter
    -N=SONAR_PROPERTIES, --sonar-property=SONAR_PROPERTIES
                        Property for sonar analyzer(name[=val], "yes" if val is omitted")
    --sonar-java-args=SONAR_JAVA_ARGS
                        Java machine properties for sonar scanner run
    --get-deps=GET_DEPS Compile and collect all dependencies into specified directory
    -s, --sources       Make sources jars as well
    --maven-export      Export to maven repository
    --maven-no-recursive-deps
                        Not export recursive dependencies
    --maven-exclude-transitive-from-deps
                        Exclude transitive from dependencies
    --version=VERSION   Version of artifacts for exporting to maven
    --deploy            Deploy artifact to repository
    --repository-id=REPOSITORY_ID
                        Maven repository id
    --repository-url=REPOSITORY_URL
                        Maven repository url
    --settings=MAVEN_SETTINGS
                        Maven settings.xml file path
    --maven-out-dir=MAVEN_OUTPUT
                        Maven output directory( for .class files )
    --use-uncanonical-pom-name
                        Use uncanonical pom output filename( {artifact}.pom )
    -J=JAVAC_FLAGS, --javac-opts=JAVAC_FLAGS
                        Set common javac flags (name=val)
```
{% endcut %}

{% cut "Опции работы с Sandbox и MDS" %}
```
    --owner=RESOURCE_OWNER
                        User name to own data saved to sandbox
    --sandbox-url=SANDBOX_URL
                        sandbox url to use for storing canonical file (default: https://sandbox.yandex-team.ru)
    --task-kill-timeout=TASK_KILL_TIMEOUT
                        Timeout in seconds for sandbox uploading task
    --sandbox           Upload to Sandbox
    --mds               Upload to MDS
    --mds-host=MDS_HOST MDS Host (default: storage.yandex-team.ru)
    --mds-port=MDS_PORT MDS Port (default: 80)
    --mds-namespace=MDS_NAMESPACE
                        MDS namespace (default: devtools)
    --mds-token=MDS_TOKEN
                        MDS Basic Auth token
```
{% endcut %}

{% cut "Опции авторизации" %}
```
    --key=SSH_KEYS      Path to private ssh key to exchange for OAuth token
    --token=OAUTH_TOKEN oAuth token
    --user=USERNAME     Custom user name for authorization
```
{% endcut %}

## Примеры
```
  ya make -r               Build current directory in release mode
  ya make -t -j16 library  Build and test library with 16 threads
  ya make --checkout -j0   Checkout absent directories without build
```
