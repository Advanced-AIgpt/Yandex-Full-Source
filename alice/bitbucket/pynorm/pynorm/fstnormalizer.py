#! /usr/bin/env python

import ctypes
import sys
import os
import argparse

import alice.bitbucket.pynorm.normalize.ctypes as libnormalizer


def encode_string(string):
    if sys.version_info > (3, 0):
        return bytes(string, 'utf-8')
    return string

def decode_string(string):
    if sys.version_info > (3, 0):
        return string.decode('utf-8')
    return string

class FSTNormalizer(object):

    def __init__(self, normalizer_data_path, fst_black_list=None):
        self.black_list = None
        if fst_black_list:
            c_style_black_list = (ctypes.c_char_p * (len(fst_black_list) + 1))(*(fst_black_list + [None]))
            self.black_list = ctypes.cast(c_style_black_list, ctypes.POINTER(ctypes.c_char_p))

        self._libnormalize = libnormalizer.Normalizer()
        self._libnormalize.norm_run_with_blacklist.argtypes = (ctypes.c_void_p,
                                                               ctypes.c_char_p,
                                                               ctypes.POINTER(ctypes.c_char_p))
        self._libnormalize.norm_run_with_blacklist.restype = ctypes.POINTER(ctypes.c_char)
        self._libnormalize.norm_data_read.argtypes = (ctypes.c_char_p,)
        self._libnormalize.norm_data_read.restype = ctypes.c_void_p
        self._libnormalize.norm_data_free.argtypes = (ctypes.c_void_p,)

        self.normalizer_data = self._libnormalize.norm_data_read(encode_string(normalizer_data_path))
        assert self.normalizer_data, "Error! Could not load FSTs!"

    def __del__(self):
        if hasattr(self, 'normalizer_data') and self.normalizer_data:
            self._libnormalize.norm_data_free(self.normalizer_data)
            self.normalizer_data = None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self._libnormalize.norm_data_free(self.normalizer_data)
        self.normalizer_data = None

    def normalize(self, text):
        result_ptr = self._libnormalize.norm_run_with_blacklist(self.normalizer_data,
                                                                encode_string(text),
                                                                self.black_list)
        if result_ptr:
            normalized_text = ctypes.string_at(result_ptr)
            self._libnormalize.free(result_ptr)
        else:
            normalized_text = ""
        return normalized_text


if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        description="Normalizer wrapper.\n"
                    "Reads the sentences linewise from stdin, normalize them and writes to stdout.\n"
    )
    parser.add_argument(
        "normalizer_data_path",
        help="path to normalizer data (FSTs etc)"
    )
    options = parser.parse_args()

    normalizer = FSTNormalizer(options.normalizer_data_path)
    for line in sys.stdin:
        result = normalizer.normalize(line.strip())
        if len(result) == 0:
            result = "\n"
        print(decode_string(result))
