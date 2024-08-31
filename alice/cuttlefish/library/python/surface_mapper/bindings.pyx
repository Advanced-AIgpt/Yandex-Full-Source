from util.generic.string cimport TString
from util.generic.vector cimport TVector


cdef extern from 'alice/cuttlefish/library/surface_mapper/mapper.h' namespace 'NVoice::NSurfaceMapper':
    cdef cppclass TSurfaceInfo:
        TString GetSurface()
        TString GetVendorStr()
        TString GetPlatformStr()
        TString GetTypeStr()
        TVector[TString] ListAllPlatformsStr()
        TVector[TString] ListAllTypesStr()
        TVector[TString] ListPlatformsStr()
        TVector[TString] ListTypesStr()

    cdef cppclass TMapper:
        TMapper() except +
        const TSurfaceInfo& MapForPythonBinding(const TString& appId)


class Coder:
    _CODING = 'utf-8'

    @classmethod
    def encode(cls, value: str):
        return value.encode(cls._CODING)

    @classmethod
    def decode(cls, value: bytes):
        return value.decode(cls._CODING)


class PrettyPrintable:
    def _pretty_members(self):
        return ', '.join((f'{key}={value}' for key, value in vars(self).items()))

    def __str__(self):
        return f'{self.__class__.__name__}({self._pretty_members()})'


class SurfaceInfo(PrettyPrintable):
    def __init__(
        self,
        surface,
        vendor,
        main_platform,
        main_type,
        all_platforms,
        all_types,
        platforms,
        types,
    ):
        self.surface = surface
        self.vendor = vendor
        self.main_platform = main_platform
        self.main_type = main_type
        self.all_platforms = all_platforms
        self.all_types = all_types
        self.platforms = platforms
        self.types = types


def decode_kwargs(**kwargs):
    decoded_kwargs = dict()
    for key, value in kwargs.items():
        if isinstance(value, bytes):
            decoded_kwargs[key] = Coder.decode(value)
        else:
            assert isinstance(value, list)
            decoded_kwargs[key] = [Coder.decode(subvalue) for subvalue in value]
    return decoded_kwargs


def make_surface_info(**kwargs):
    return SurfaceInfo(**decode_kwargs(**kwargs))


cdef class MapperImpl:
    cdef TMapper* _impl

    def __cinit__(self):
        self._impl = new TMapper()

    def __dealloc__(self):
        del self._impl

    def map(self, app_id: str):
        cdef TSurfaceInfo raw = self._impl.MapForPythonBinding(Coder.encode(app_id))
        return make_surface_info(
            surface=raw.GetSurface(),
            vendor=raw.GetVendorStr(),
            main_platform=raw.GetPlatformStr(),
            main_type=raw.GetTypeStr(),
            all_platforms=raw.ListAllPlatformsStr(),
            all_types=raw.ListAllTypesStr(),
            platforms=raw.ListPlatformsStr(),
            types=raw.ListTypesStr(),
        )

    def try_map(self, app_id: str):
        val = self.map(app_id)
        return val if val.surface else None
