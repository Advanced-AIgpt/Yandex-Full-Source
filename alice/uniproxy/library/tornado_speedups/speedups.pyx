import array
from util.generic.string cimport TStringBuf, TString


def pyx_websocket_mask(TStringBuf mask, TStringBuf data):
    cdef TString result
    result.resize(data.Size())

    cdef unsigned size = data.Size()
    cdef unsigned i = 0

    while i < size:
        result[i] = data.Data()[i] ^ mask.Data()[i % 4]
        i += 1
    return result
