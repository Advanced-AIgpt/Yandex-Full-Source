from dataclasses import dataclass


@dataclass
class Color:
    pass


@dataclass
class ColorHSV(Color):
    h: int
    s: int
    v: int


@dataclass
class ColorRGB(Color):
    r: int
    g: int
    b: int


@dataclass
class Range:
    min: int
    max: int
    precision: int = 1
