from util.system.types cimport ui64
from util.digest.multi cimport MultiHash


def get_multi_hash_ui64(ui64 a, ui64 b):
    return MultiHash(a, b)
