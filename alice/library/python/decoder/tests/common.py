import os.path
import yatest.common
from alice.library.python.decoder import Decoder


DECODER_ROOT = "alice/library/python/decoder"
TEST_APP_PATH = yatest.common.binary_path(os.path.join(DECODER_ROOT, "tests/cpp_test_app/cpp_test_app"))
TEST_OGG_PATH = yatest.common.source_path(os.path.join(DECODER_ROOT, "tests/test123-opus.ogg"))
TEST_WEBM_PATH = yatest.common.source_path(os.path.join(DECODER_ROOT, "tests/bear-vp9-opus.webm"))
