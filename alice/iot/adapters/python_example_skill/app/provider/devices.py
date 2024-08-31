from dataclasses import dataclass, field
import logging
import uuid
from typing import Dict
from app.provider.instances import Range, ColorHSV
from app.provider import action_answers
from app.provider.capabilities import OnOffCapability, LightCapability, BacklightCapability, BrightnessCapability, \
    ColorHSVCapability, WhiteTemperatureCapability, FanSpeedCapability, SwingCapability, TemperatureCapability, \
    ThermostatCapability

logging.basicConfig(format="%(filename)s[line:%(lineno)d]# %(levelname)-8s [%(asctime)s]  %(message)s",
                    level=logging.DEBUG, filename="/app/app.log")


class Value:
    LOW = "low"
    MEDIUM = "medium"
    HIGH = "high"
    AUTO = "auto"
    COOL = "cool"
    HEAT = "heat"
    VERTICAL = "vertical"
    STATIONARY = "stationary"


@dataclass
class Device:
    id: str = field(default_factory=lambda: str(uuid.uuid4()))
    name: str = field(init=False)
    capabilities: Dict = field(init=False)


@dataclass
class OnOffDevice:
    on_off: str = "off"

    def set_on_off(self, on_off):
        if on_off not in {"on", "off"}:
            return action_answers.WrongType('There are only two possible modes: "on" and "off", but you try ' +
                                            str(on_off))

        try:
            self.on_off = on_off
            return action_answers.Ok()
        except Exception as e:
            return action_answers.UnknownError(str(e))


@dataclass
class Bulb(Device, OnOffDevice):
    name: str = "Лампа Обыкновенная"
    color: ColorHSV = None
    white_temperature: int = None
    brightness: int = 50

    is_white: bool = field(default=True, init=False)
    capabilities: Dict = field(default_factory=lambda: {
        OnOffCapability: OnOffCapability(),
        LightCapability: LightCapability(
            color_capability=ColorHSVCapability(),
            white_temperature_capability=WhiteTemperatureCapability(
                range=Range(2000, 9000)
            )
        ),
        BrightnessCapability: BrightnessCapability(range=Range(0, 100, 10))
    }, init=False)

    def __post_init__(self):
        if self.color is not None:
            self.is_white = False
        elif self.white_temperature is not None:
            self.is_white = True
        else:
            self.white_temperature = 5000
            self.is_white = True

    def set_color(self, color):
        if self.on_off == "off":
            return action_answers.WrongState("You try to change color of turned off bulb")
        if type(color) is not ColorHSV:
            return action_answers.WrongType("Color type should be ColorHSV, but you have " + str(type(color)))
        if not (0 <= color.h <= 360 and 0 <= color.s <= 100 and 0 <= color.v <= 100):
            return action_answers.WrongValue("h from 0 to 360, s from 0 to 100, v from 0 to 100, but you have " +
                                             str(color))

        try:
            self.color = color
            self.is_white = False
            return action_answers.Ok()
        except Exception as e:
            return action_answers.UnknownError(str(e))

    def set_white_temperature(self, white_temperature):
        if self.on_off == "off":
            return action_answers.WrongState("You try to change temperature of color of turned off bulb")
        if type(white_temperature) is not int:
            return action_answers.WrongType("Temperature of color type should be int, but you have " +
                                            str(type(white_temperature)))
        if not (self.capabilities[LightCapability].white_temperature_capability.range.min <=
                white_temperature <= self.capabilities[LightCapability].white_temperature_capability.range.max):
            return action_answers.WrongValue("Temperature of color should be from " +
                                             str(self.capabilities[LightCapability].white_temperature_capability.range.min) + "to" +
                                             str(self.capabilities[LightCapability].white_temperature_capability.range.max) +
                                             "but you have " + str(white_temperature))

        try:
            self.white_temperature = white_temperature
            self.is_white = True
            return action_answers.Ok()
        except Exception as e:
            return action_answers.UnknownError(str(e))

    def set_brightness(self, brightness):
        if self.on_off == "off":
            return action_answers.WrongState("You try to change brightness of color of turned off bulb")
        if type(brightness) is not int:
            return action_answers.WrongType("Brightness should be int, but you have " + str(type(brightness)))
        if not (self.capabilities[BrightnessCapability].range.min <= brightness <= self.capabilities[BrightnessCapability].range.max):
            return action_answers.WrongValue("Brightness should be from " +
                                             str(self.capabilities[BrightnessCapability].range.min) + "to" +
                                             str(self.capabilities[BrightnessCapability].range.max) +
                                             "but you have " + str(brightness))
        try:
            self.brightness = brightness
            return action_answers.Ok()
        except Exception as e:
            return action_answers.UnknownError(str(e))


@dataclass
class Socket(Device, OnOffDevice):
    name: str = "Розетка С Подсветочкой"
    on_off: str = "on"
    backlight: str = "off"
    capabilities: Dict = field(default_factory=lambda: {
        OnOffCapability: OnOffCapability(),
        BacklightCapability: BacklightCapability()
    }, init=False)

    def set_backlight(self, backlight):
        if backlight not in {"on", "off"}:
            return action_answers.WrongType('There are only two possible modes: "on" and "off", but you try ' +
                                            str(backlight))

        try:
            self.backlight = backlight
            return action_answers.Ok()
        except Exception as e:
            return action_answers.UnknownError(str(e))


@dataclass
class Conditioner(Device, OnOffDevice):
    name: str = "Кондиционер Обыкновенный"
    temperature: int = 25
    speed: int = field(default=50, init=False)
    fan_speed: str = Value.MEDIUM
    swing: str = Value.STATIONARY
    thermostat: str = Value.AUTO
    capabilities: Dict = field(default_factory=lambda: {
        OnOffCapability: OnOffCapability(),
        TemperatureCapability: TemperatureCapability(range=Range(-10, 40), unit="C"),
        FanSpeedCapability: FanSpeedCapability(modes=[Value.LOW, Value.MEDIUM, Value.HIGH]),
        SwingCapability: SwingCapability(modes=[Value.VERTICAL, Value.STATIONARY]),
        ThermostatCapability: ThermostatCapability(modes=[Value.AUTO, Value.COOL, Value.HEAT])
    }, init=False)

    fan_speed_mode_to_speed: Dict = field(default_factory=lambda: {Value.LOW: 30, Value.MEDIUM: 50, Value.HIGH: 100},
                                          init=False)
    thermostat_mode_to_temperature: Dict = field(default_factory=lambda: {Value.COOL: 20, Value.HEAT: 30}, init=False)

    def set_temperature(self, temperature):
        if self.on_off == "off":
            return action_answers.WrongState("You try to change temperature of turned off conditioner")
        if type(temperature) is not int:
            return action_answers.WrongType("Temperature should be int, but you have " +
                                            str(type(temperature)))
        if not (self.capabilities[TemperatureCapability].range.min <=
                temperature <= self.capabilities[TemperatureCapability].range.max):
            return action_answers.WrongValue("Temperature should be from " +
                                             str(self.capabilities[TemperatureCapability].range.min) + "to" +
                                             str(self.capabilities[TemperatureCapability].range.max) +
                                             "but you have " + str(temperature))

        try:
            self.temperature = temperature
            return action_answers.Ok()
        except Exception as e:
            return action_answers.UnknownError(str(e))

    def set_fan_speed(self, fan_speed):
        if self.on_off == "off":
            return action_answers.WrongState("You try to change fan speed of turned off conditioner")
        if type(fan_speed) is not str:
            return action_answers.WrongType("Fan speed mode should be str, but you have " +
                                            str(type(fan_speed)))
        if fan_speed not in self.capabilities[FanSpeedCapability].modes:
            return action_answers.WrongType("There are possible modes:" +
                                            ', '.join(self.capabilities[FanSpeedCapability].modes) +
                                            ", but you try " + str(fan_speed))

        try:
            self.fan_speed = fan_speed
            self.speed = self.fan_speed_mode_to_speed[fan_speed]
            return action_answers.Ok()
        except Exception as e:
            return action_answers.UnknownError(str(e))

    def set_swing(self, swing):
        if self.on_off == "off":
            return action_answers.WrongState("You try to change swing of turned off conditioner")
        if type(swing) is not str:
            return action_answers.WrongType("Swing mode should be str, but you have " +
                                            str(type(swing)))
        if swing not in self.capabilities[SwingCapability].modes:
            return action_answers.WrongType("There are possible modes:" +
                                            ', '.join(self.capabilities[SwingCapability].modes) +
                                            ", but you try " + str(swing))

        try:
            self.swing = swing
            return action_answers.Ok()
        except Exception as e:
            return action_answers.UnknownError(str(e))

    def set_thermostat(self, thermostat):
        if self.on_off == "off":
            return action_answers.WrongState("You try to change thermostat of turned off conditioner")
        if type(thermostat) is not str:
            return action_answers.WrongType("Thermostat mode should be str, but you have " +
                                            str(type(thermostat)))
        if thermostat not in self.capabilities[ThermostatCapability].modes:
            return action_answers.WrongType("There are possible modes:" +
                                            ', '.join(self.capabilities[SwingCapability].modes) +
                                            ", but you try " + str(thermostat))

        try:
            self.thermostat = thermostat
            if thermostat != Value.AUTO:
                self.temperature = self.thermostat_mode_to_temperature[thermostat]
            return action_answers.Ok()
        except Exception as e:
            return action_answers.UnknownError(str(e))
