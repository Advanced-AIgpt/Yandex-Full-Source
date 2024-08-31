You have to use PY3TEST() and add the following line into your ya.make ```INCLUDE(${ARCADIA_ROOT}/alice/joker/library/python/for_tests.inc)``` which adds peerdir/depends.
