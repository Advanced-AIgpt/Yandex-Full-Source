
from common import TEST_OGG_PATH
from alice.library.python.decoder import Decoder, DecoderException
import os
import pytest


def decode_file(input_file, result_file, decoder=None, sample_rate=16000):
    if decoder is None:
        decoder = Decoder(sample_rate)

    with open(input_file, 'rb') as fi, open(result_file, 'wb') as fo:
        while True:
            data = fi.read(1024)
            if data:
                decoder.write(data)
            else:
                decoder.close()

            while True:
                data = decoder.read()
                if data is None:
                    break
                fo.write(data)

            if decoder.eof():
                break


def test_strange_behavior():
    """ Do something and expect no exceptions
    """

    dc = Decoder(16000)
    dc.close()
    dc.close()

    dc = Decoder(16000)
    with open(TEST_OGG_PATH, 'rb') as f:
        data = f.read(1024)
        dc.write(data)
    dc.close()
    dc.close()


def test_unrecognizable_input_format():
    input = b"AbAbAbAbAb"
    dc = Decoder(16000)

    with pytest.raises(DecoderException) as exc:
        dc.write(input)
        dc.close()
        assert dc.read() is None
    dc.close()
    dc.close()


def test_decode_8KHz():
    result_path = "py-test123-8KHz.pcm"
    decode_file(TEST_OGG_PATH, result_path, sample_rate=8000)
    assert os.path.getsize(result_path) == 82208


def test_decode_16KHz():
    result_path = "py-test123-16KHz.pcm"
    decode_file(TEST_OGG_PATH, result_path, sample_rate=16000)
    assert os.path.getsize(result_path) == 164448
