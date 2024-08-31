from functools import partial

from . import surface
from .alice import EvoError, AsrRecognitionError
from .muzpult import Muzpult


__all__ = ['Muzpult', 'EvoError', 'AsrRecognitionError']


def _wraps_surface(surface_func):
    def wrapper(**device_state):
        surface_partial = partial(surface_func(), device_state=device_state)
        surface_partial.__name__ = surface_func.__name__
        return surface_partial
    wrapper.__name__ = surface_func.__name__
    return wrapper


# auto
@_wraps_surface
def automotive():
    return surface.Auto


@_wraps_surface
def uma():
    return surface.AutoNew


@_wraps_surface
def old_automotive():
    return surface.AutoOld


# navi
@_wraps_surface
def navi():
    return surface.Navigator


@_wraps_surface
def maps():
    return surface.Maps


@_wraps_surface
def navi_tr():
    return surface.NavigatorTr


# smart speaker
@_wraps_surface
def dexp():
    return surface.Dexp


@_wraps_surface
def loudspeaker():
    return surface.YandexMini


@_wraps_surface
def station_midi():
    return surface.YandexMidi


@_wraps_surface
def smart_display():
    return surface.SmartDisplay


@_wraps_surface
def station():
    return surface.Quasar


@_wraps_surface
def station_lite_beige():
    return surface.YandexMicroBeige


@_wraps_surface
def station_lite_green():
    return surface.YandexMicroGreen


@_wraps_surface
def station_lite_pink():
    return surface.YandexMicroPink


@_wraps_surface
def station_lite_purple():
    return surface.YandexMicroPurple


@_wraps_surface
def station_lite_red():
    return surface.YandexMicroRed


@_wraps_surface
def station_lite_yellow():
    return surface.YandexMicroYellow


@_wraps_surface
def station_pro():
    return surface.YandexMax


# other
@_wraps_surface
def aliceapp():
    return surface.Aliceapp


@_wraps_surface
def browser():
    return surface.Browser


@_wraps_surface
def browser_ios():
    return surface.BrowserIos


@_wraps_surface
def webtouch():
    return surface.Webtouch


@_wraps_surface
def webtouch_ios():
    return surface.WebtouchIos


@_wraps_surface
def launcher():
    return surface.Launcher


@_wraps_surface
def sdc():
    return surface.Sdc


@_wraps_surface
def searchapp():
    return surface.Searchapp


@_wraps_surface
def smart_tv():
    return surface.SmartTv


@_wraps_surface
def module_2():
    return surface.Module2


@_wraps_surface
def taximeter():
    return surface.Taximeter


@_wraps_surface
def watch():
    return surface.Watch


@_wraps_surface
def yabro_win():
    return surface.YabroWin


@_wraps_surface
def legatus():
    return surface.Legatus


all_station_lite = [
    station_lite_beige,
    station_lite_green,
    station_lite_pink,
    station_lite_purple,
    station_lite_red,
    station_lite_yellow,
]


# preset of all actual surfaces
# except for 'navi_tr' and 'old_automotive'
actual_surfaces = [
    automotive,
    launcher,
    loudspeaker,
    navi,
    searchapp,
    smart_tv,
    station,
    station_lite_beige,
    station_pro,
    watch,
    yabro_win,
]


yandex_smart_speakers = [
    loudspeaker,
    station,
    station_lite_beige,
    station_pro,
]


smart_speakers = [
    dexp,
    loudspeaker,
    station,
    station_lite_beige,
    station_pro,
]


auto = [
    automotive,
    old_automotive,
]


_smart_speakers = frozenset([
    surface.Dexp.preset.application['app_id'],
    surface.Quasar.preset.application['app_id'],
    surface.YandexMax.preset.application['app_id'],
    surface.YandexMicroBeige.preset.application['app_id'],
    surface.YandexMini.preset.application['app_id'],
])


_auto = frozenset([
    surface.Auto.preset.application['app_id'],
    surface.AutoOld.preset.application['app_id'],
])


def is_auto(alice):
    return alice.application.AppId in _auto


def is_smart_speaker(alice):
    return alice.application.AppId in _smart_speakers


def is_sdc(alice):
    return alice.application.AppId == surface.Sdc.preset.application['app_id']


def is_searchapp(alice):
    return alice.application.AppId == surface.Searchapp.preset.application['app_id']


def is_smart_tv(alice):
    return alice.application.AppId == surface.SmartTv.preset.application['app_id']


def is_launcher(alice):
    return alice.application.AppId == surface.Launcher.preset.application['app_id']


def is_navi(alice):
    return alice.application.AppId == surface.Navigator.preset.application['app_id']


def is_quasar(alice):
    return alice.application.AppId == surface.Quasar.preset.application['app_id']


def is_watch(alice):
    return alice.application.AppId == surface.Watch.preset.application['app_id']


def is_yabro_win(alice):
    return alice.application.AppId == surface.YabroWin.preset.application['app_id']


def is_legatus(alice):
    return alice.application.AppId == surface.Legatus.preset.application['app_id']
