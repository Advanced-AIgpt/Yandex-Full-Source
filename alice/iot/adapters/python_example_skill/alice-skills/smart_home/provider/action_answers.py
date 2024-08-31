from dataclasses import dataclass


@dataclass
class ActionAnswer:
    message: str = ""


@dataclass
class Ok(ActionAnswer):
    message = "OK"


@dataclass
class WrongType(ActionAnswer):
    pass


@dataclass
class WrongValue(ActionAnswer):
    pass


@dataclass
class WrongState(ActionAnswer):
    pass


@dataclass
class DeviceNotFound(ActionAnswer):
    pass


@dataclass
class UnknownError(ActionAnswer):
    pass
