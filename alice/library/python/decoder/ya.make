LIBRARY(decoder)
OWNER(g:voicetech-infra)
USE_PYTHON3()

PEERDIR(
    contrib/libs/ffmpeg-3/libavcodec
    contrib/libs/ffmpeg-3/libavformat
    contrib/libs/ffmpeg-3/libavutil
    contrib/libs/ffmpeg-3/libavfilter
)

ADDINCL(
    contrib/libs/ffmpeg-3
)

PY_SRCS(
    CYTHON_C
    __init__.py
    decoder.pyx
)

SRCS(
    stream_decoder.cpp
    stream_decoder.hpp
    stream_decoder.h
)

END()

RECURSE_FOR_TESTS(
    tests
    tests/cpp_test_app
    tests/py_test_app
)
