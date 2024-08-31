from typing import Dict, List
from smart_home.provider.capabilities import Capability, OnOffCapability, RangeCapability, ModeCapability, LightCapability, \
    BrightnessCapability, TemperatureCapability, ColorHSVCapability, ColorRGBCapability
from smart_home.provider.devices import Device
from smart_home.translator.translator import DeviceCapability, Parameters, Instance, \
    get_smart_home_capability_type, get_smart_home_capability_instance


class Range:
    MIN = "min"
    MAX = "max"
    PRECISION = "precision"


class Unit:
    PERCENT = "unit.percent"
    CELSIUS = "unit.temperature.celsius"
    KELVIN = "unit.temperature.kelvin"


def get_smart_home_capability_unit(capability: type(Capability), unit: str = None) -> str:
    if capability is BrightnessCapability:
        return Unit.PERCENT
    if capability is TemperatureCapability:
        return Unit.CELSIUS if unit == "C" else Unit.KELVIN


def get_smart_home_capability_parameters(capability: Capability) -> Dict:
    parameters = dict()

    if type(capability) is OnOffCapability:
        return parameters

    if type(capability) is LightCapability:
        if capability.color_capability is not None:
            if type(capability.color_capability) is ColorHSVCapability:
                parameters[Parameters.COLOR] = Instance.HSV
            if type(capability.color_capability) is ColorRGBCapability:
                parameters[Parameters.COLOR] = Instance.RGB
        if capability.white_temperature_capability is not None:
            parameters[Parameters.COLOR_TEMPERATURE] = {
                Range.MIN: capability.white_temperature_capability.range.min,
                Range.MAX: capability.white_temperature_capability.range.max
            }
        return parameters

    parameters[Parameters.INSTANCE] = get_smart_home_capability_instance(type(capability))
    if issubclass(type(capability), OnOffCapability):
        return parameters

    if issubclass(type(capability), ModeCapability):
        parameters[Parameters.MODES] = [{Parameters.VALUE: mode} for mode in capability.modes]
        return parameters

    if issubclass(type(capability), RangeCapability):
        parameters[Parameters.RANGE] = {
            Range.MIN: capability.range.min,
            Range.MAX: capability.range.max,
            Range.PRECISION: capability.range.precision
        }

        parameters[Parameters.UNIT] = get_smart_home_capability_unit(
            type(capability),
            None if not hasattr(capability, "unit") else capability.unit
        )
        return parameters
    return parameters


def get_smart_home_capability(capability: Capability) -> Dict:
    response = {
        DeviceCapability.TYPE: get_smart_home_capability_type(type(capability)),
        DeviceCapability.RETRIEVABLE: capability.retrievable,
        DeviceCapability.PARAMETERS: get_smart_home_capability_parameters(capability)
    }

    if not response[DeviceCapability.PARAMETERS]:
        del response[DeviceCapability.PARAMETERS]

    return response


def get_smart_home_device_capabilities(device: Device) -> List:
    capabilities = []
    for capability in device.capabilities:
        capabilities.append(get_smart_home_capability(device.capabilities[capability]))
    return capabilities
