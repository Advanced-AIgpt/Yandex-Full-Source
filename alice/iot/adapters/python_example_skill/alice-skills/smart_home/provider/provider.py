from typing import List
from smart_home.provider.instances import ColorHSV
from smart_home.provider.devices import Device, Bulb, Socket, Conditioner


class Provider:
    def __init__(self, devices: List[Device]) -> None:
        self.id_to_device = dict()
        for device in devices:
            self.id_to_device[device.id] = device


bulb1 = Bulb(id="1", on_off="on", color=ColorHSV(179, 57, 2), brightness=50)
bulb2 = Bulb(id="2", on_off="on", white_temperature=8000)
bulb3 = Bulb(id="3", on_off="off")
socket1 = Socket(id="4")
conditioner1 = Conditioner(id="5", on_off="on", temperature=28)
provider = Provider([bulb1, bulb2, bulb3, socket1, conditioner1])
