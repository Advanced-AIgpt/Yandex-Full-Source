import logging
from typing import Dict, List
from smart_home.provider.devices import Device
from smart_home.provider.instances import ColorRGB, ColorHSV
from smart_home.translator.translator import get_smart_home_error_code, ActionResult, DeviceCapability, Instance


logging.basicConfig(format="%(levelname)s PID %(process)d TID %(thread)d [%(asctime)s]: %(message)s\n",
                    level=logging.INFO, filename="/smart_home/smart_home_api.log")


def int_to_rgb(capability_value: int) -> ColorRGB:
    new_b = capability_value % 256
    capability_value >>= 8
    new_g = capability_value % 256
    capability_value >>= 8
    new_r = capability_value
    return ColorRGB(new_r, new_g, new_b)


def apply_action_get_smart_home_result(device: Device, action: Dict) -> Dict:
    capability_type = action[DeviceCapability.TYPE]
    capability_instance = action[DeviceCapability.STATE][DeviceCapability.INSTANCE]
    capability_value = action[DeviceCapability.STATE][DeviceCapability.VALUE]

    result = None
    if capability_type == DeviceCapability.ON_OFF:
        new_capability_value = "on" if capability_value else "off"
        logging.info("Changing %r to %r",  device.on_off, new_capability_value)
        result = device.set_on_off(new_capability_value)

    if capability_type == DeviceCapability.COLOR_SETTING:
        if capability_instance == Instance.HSV:
            color = capability_value
            new_capability_value = ColorHSV(color["h"], color["s"], color["v"])
            logging.info("Changing color %r to %r", device.color, new_capability_value)
            result = device.set_color(new_capability_value)
        if capability_instance == Instance.RGB:
            new_capability_value = int_to_rgb(capability_value)
            logging.info("Changing color %r to %r", device.color, new_capability_value)
            result = device.set_color(new_capability_value)
        if capability_instance == Instance.COLOR_TEMPERATURE:
            logging.info("Changing temperature of color %r to %r", device.white_temperature, capability_value)
            result = device.set_white_temperature(capability_value)

    if capability_type == DeviceCapability.RANGE:
        relative = False
        if DeviceCapability.RELATIVE in action[DeviceCapability.STATE]:
            relative = action[DeviceCapability.STATE][DeviceCapability.RELATIVE]

        if capability_instance == Instance.BRIGHTNESS:
            new_capability_value = (device.brightness if relative else 0) + capability_value
            logging.info("Changing brightness %r to %r", device.brightness, new_capability_value)
            result = device.set_brightness(new_capability_value)

        if capability_instance == Instance.TEMPERATURE:
            new_capability_value = (device.temperature if relative else 0) + capability_value
            logging.info("Changing brightness %r to %r", device.temperature, new_capability_value)
            result = device.set_temperature(new_capability_value)

    if capability_type == DeviceCapability.TOGGLE:
        if capability_instance == Instance.BACKLIGHT:
            new_capability_value = "on" if capability_value else "off"
            logging.info("Changing backlight from %r to %r", device.backlight, new_capability_value)
            result = device.set_backlight(new_capability_value)

    if capability_type == DeviceCapability.MODE:
        if capability_instance == Instance.FAN_SPEED:
            logging.info("Changing fan_speed from %r to %r", device.fan_speed, capability_value)
            result = device.set_fan_speed(capability_value)
        if capability_instance == Instance.SWING:
            logging.info("Changing fan_speed from %r to %r", device.swing, capability_value)
            result = device.set_swing(capability_value)
        if capability_instance == Instance.THERMOSTAT:
            logging.info("Changing thermostat from %r to %r", device.thermostat, capability_value)
            result = device.set_thermostat(capability_value)

    if get_smart_home_error_code(type(result)) == ActionResult.DONE:
        return {
            ActionResult.STATUS: ActionResult.DONE
        }

    logging.warning("Can't change something on %r device", device.id)
    return {
        ActionResult.STATUS: ActionResult.ERROR,
        ActionResult.CODE: get_smart_home_error_code(type(result)),
        ActionResult.MESSAGE: result.message
    }


def get_smart_home_capability(action: Dict, action_result: Dict) -> Dict:
    return {
        DeviceCapability.TYPE: action[DeviceCapability.TYPE],
        DeviceCapability.STATE: {
            DeviceCapability.INSTANCE: action[DeviceCapability.STATE][DeviceCapability.INSTANCE],
            ActionResult.RESULT: action_result
        }
    }


def get_smart_home_device_capabilities(device: Device, actions: List) -> List:
    capabilities = []
    for action in actions:
        action_result = apply_action_get_smart_home_result(device, action)
        capabilities.append(get_smart_home_capability(action, action_result))
    return capabilities
