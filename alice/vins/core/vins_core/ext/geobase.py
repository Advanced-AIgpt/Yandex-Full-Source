# coding: utf-8
from __future__ import unicode_literals

# from geobase5 import Lookup
from geobase6 import Lookup

from vins_core.utils.data import get_resource_full_path

from library.python.func import memoize


# read the doc https://doc.yandex-team.ru/lib/libgeobase5/concepts/interfaces-python.xml
@memoize()
def get_geobase():
    return Lookup(get_resource_full_path("last/GEODATA5BIN_STABLE").encode("utf-8"))
