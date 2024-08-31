from typing import Any, Dict, List
from smart_home.provider.capabilities import Capability, OnOffCapability, LightCapability, BacklightCapability, \
    BrightnessCapability, ColorHSVCapability, ColorRGBCapability, WhiteTemperatureCapability, FanSpeedCapability, \
    SwingCapability, TemperatureCapability, ThermostatCapability
from smart_home.provider.devices import Device
from smart_home.provider.instances import ColorRGB, ColorHSV
from smart_home.translator.translator import DeviceCapability, \
    get_smart_home_capability_type, get_smart_home_capability_instance


def rgb_to_int(rgb: ColorRGB) -> int:
    value = (rgb.r << 16) + (rgb.g << 8) + rgb.b
    return value


def get_smart_home_capability_value(capability: type(Capability), capability_value: Any) -> Any:
    if issubclass(capability, OnOffCapability):
        return capability_value == "on"
    if capability is LightCapability:
        if type(capability_value) is ColorHSV:
            return {"h": capability_value.h, "s": capability_value.s, "v": capability_value.v}
        if type(capability_value) is ColorRGB:
            return rgb_to_int(capability_value)
    return capability_value


def get_capability_value(capability: type(Capability), device: Device) -> Any:
    if capability is LightCapability:
        if device.is_white:
            return device.white_temperature
        else:
            return device.color
    if capability is OnOffCapability:
        return device.on_off
    if capability is BrightnessCapability:
        return device.brightness
    if capability is BacklightCapability:
        return device.backlight
    if capability is TemperatureCapability:
        return device.temperature
    if capability is FanSpeedCapability:
        return device.fan_speed
    if capability is SwingCapability:
        return device.swing
    if capability is ThermostatCapability:
        return device.thermostat


def get_smart_home_capability_state(capability: type(Capability), capability_value: Any):
    instance = get_smart_home_capability_instance(capability)
    if capability is LightCapability:
        if type(capability_value) is ColorHSV:
            instance = get_smart_home_capability_instance(ColorHSVCapability)
        elif type(capability_value) is ColorRGB:
            instance = get_smart_home_capability_instance(ColorRGBCapability)
        else:
            instance = get_smart_home_capability_instance(WhiteTemperatureCapability)
    return {
        DeviceCapability.INSTANCE: instance,
        DeviceCapability.VALUE: get_smart_home_capability_value(capability, capability_value)
    }


def get_smart_home_capability(capability: type(Capability), capability_value: Any) -> Dict:
    return {
        DeviceCapability.TYPE: get_smart_home_capability_type(capability),
        DeviceCapability.STATE: get_smart_home_capability_state(capability, capability_value)
    }


def get_smart_home_device_capabilities(device: Device) -> List:
    capabilities = []
    for capability in device.capabilities:
        capabilities.append(
            get_smart_home_capability(capability, get_capability_value(capability, device))
        )
    return capabilities
