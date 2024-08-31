from library.python.ctypes import StaticLibrary

from .syms import syms


def Normalizer():
    return StaticLibrary('normalizer', syms)
