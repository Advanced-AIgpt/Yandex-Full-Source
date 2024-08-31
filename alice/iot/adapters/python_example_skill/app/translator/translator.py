from app.provider.capabilities import Capability, OnOffCapability, LightCapability, BacklightCapability, \
    BrightnessCapability, ColorHSVCapability, ColorRGBCapability, WhiteTemperatureCapability, FanSpeedCapability, \
    SwingCapability, TemperatureCapability, ThermostatCapability, RangeCapability, ModeCapability
from app.provider.action_answers import Ok, WrongValue, WrongType, WrongState, UnknownError, DeviceNotFound, \
    ActionAnswer
from app.provider.devices import Device, Bulb, Socket, Conditioner


class ActionResult:
    RESULT = "action_result"

    STATUS = "status"
    CODE = "error_code"

    ERROR = "ERROR"
    DONE = "DONE"

    MESSAGE = "error_message"
        
    INVALID_ACTION = "INVALID_ACTION"
    INVALID_VALUE = "INVALID_VALUE"
    INTERNAL_ERROR = "INTERNAL_ERROR"
    NOT_SUPPORTED_IN_CURRENT_MODE = "NOT_SUPPORTED_IN_CURRENT_MODE"
    DEVICE_NOT_FOUND = "DEVICE_NOT_FOUND"


class DeviceInfo:
    ID = "id"
    CAPABILITIES = "capabilities"
    NAME = "name"
    TYPE = "type"

    LIGHT = "devices.types.light"
    SOCKET = "devices.types.socket"
    CONDITIONER = "devices.types.thermostat.ac"


class Parameters:
    INSTANCE = "instance"
    VALUE = "value"
    MODES = "modes"
    COLOR = "color_model"
    COLOR_TEMPERATURE = "temperature_k"
    UNIT = "unit"
    RANGE = "range"
    TEMPERATURE = "temperature"


class DeviceCapability:
    TYPE = "type"
    RETRIEVABLE = "retrievable"
    PARAMETERS = "parameters"

    STATE = "state"
    INSTANCE = "instance"
    VALUE = "value"

    RELATIVE = "relative"
    
    ON_OFF = "devices.capabilities.on_off"
    COLOR_SETTING = "devices.capabilities.color_setting"
    MODE = "devices.capabilities.mode"
    RANGE = "devices.capabilities.range"
    TOGGLE = "devices.capabilities.toggle"


class Instance:
    ON_OFF = "on"
    HSV = "hsv"
    RGB = "rgb"
    COLOR_TEMPERATURE = "temperature_k"
    BRIGHTNESS = "brightness"
    BACKLIGHT = "backlight"
    TEMPERATURE = "temperature"
    FAN_SPEED = "fan_speed"
    SWING = "swing"
    THERMOSTAT = "thermostat"


to_smart_home_device_type = {
    Bulb: DeviceInfo.LIGHT,
    Socket: DeviceInfo.SOCKET,
    Conditioner: DeviceInfo.CONDITIONER
}


to_smart_home_capability_instance = {
    OnOffCapability: Instance.ON_OFF,
    WhiteTemperatureCapability: Instance.COLOR_TEMPERATURE,
    ColorHSVCapability: Instance.HSV,
    ColorRGBCapability: Instance.RGB,
    BrightnessCapability: Instance.BRIGHTNESS,
    TemperatureCapability: Instance.TEMPERATURE,
    BacklightCapability: Instance.BACKLIGHT,
    FanSpeedCapability: Instance.FAN_SPEED,
    SwingCapability: Instance.SWING,
    ThermostatCapability: Instance.THERMOSTAT
}


to_smart_home_error_code = {
    Ok: ActionResult.DONE,
    WrongState: ActionResult.NOT_SUPPORTED_IN_CURRENT_MODE,
    WrongValue: ActionResult.INVALID_VALUE,
    WrongType: ActionResult.INVALID_ACTION,
    UnknownError: ActionResult.INTERNAL_ERROR,
    DeviceNotFound: ActionResult.DEVICE_NOT_FOUND,
    None: ActionResult.INTERNAL_ERROR
}


def get_smart_home_device_type(device: type(Device)) -> str:
    return to_smart_home_device_type.get(device)


def get_smart_home_error_code(answer: type(ActionAnswer)) -> str:
    return to_smart_home_error_code.get(answer)


def get_smart_home_capability_type(capability: type(Capability)) -> str:
    if capability is OnOffCapability:
        return DeviceCapability.ON_OFF
    if issubclass(capability, OnOffCapability):
        return DeviceCapability.TOGGLE
    if issubclass(capability, RangeCapability):
        return DeviceCapability.RANGE
    if issubclass(capability, ModeCapability):
        return DeviceCapability.MODE
    if issubclass(capability, LightCapability):
        return DeviceCapability.COLOR_SETTING


def get_smart_home_capability_instance(capability: type(Capability)) -> str:
    return to_smart_home_capability_instance.get(capability)
