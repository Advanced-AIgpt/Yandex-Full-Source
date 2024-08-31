from library.python import sanitizers


def fix_timeout_for_sanitizer(timeout):
    multiplier = 1
    if sanitizers.asan_is_on():
        # https://github.com/google/sanitizers/wiki/AddressSanitizer
        multiplier = 4
    elif sanitizers.msan_is_on():
        # via https://github.com/google/sanitizers/wiki/MemorySanitizer
        multiplier = 3
    elif sanitizers.tsan_is_on():
        # via https://clang.llvm.org/docs/ThreadSanitizer.html
        multiplier = 15

    return timeout * multiplier


def assert_numbers_are_almost_equal(a, b, max_diff):
    assert abs(a - b) <= max_diff, f"{a=}, {b=}, {max_diff=}, {abs(a - b)=}"
