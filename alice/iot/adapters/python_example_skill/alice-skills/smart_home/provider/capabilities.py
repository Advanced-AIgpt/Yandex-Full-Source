from dataclasses import dataclass
from typing import List
from smart_home.provider.instances import Range


@dataclass
class Capability:
    retrievable: bool = True


@dataclass
class OnOffCapability(Capability):
    pass


@dataclass
class BacklightCapability(OnOffCapability):
    pass


@dataclass
class ColorCapability:
    pass


@dataclass
class ColorRGBCapability(ColorCapability):
    pass


@dataclass
class ColorHSVCapability(ColorCapability):
    pass


@dataclass
class WhiteTemperatureCapability:
    range: Range = None


@dataclass
class LightCapability(Capability):
    color_capability: ColorCapability = None
    white_temperature_capability: WhiteTemperatureCapability = None


@dataclass
class RangeCapability(Capability):
    range: Range = None


@dataclass
class BrightnessCapability(RangeCapability):
    pass


@dataclass
class TemperatureCapability(RangeCapability):
    unit: str = None


@dataclass
class ModeCapability(Capability):
    modes: List = None


@dataclass
class SwingCapability(ModeCapability):
    pass


@dataclass
class FanSpeedCapability(ModeCapability):
    pass


@dataclass
class ThermostatCapability(ModeCapability):
    pass
