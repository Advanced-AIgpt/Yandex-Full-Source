from typing import Dict, List
from app.provider.capabilities import Capability
from app.provider.devices import Device
from app.translator.translator import DeviceCapability, get_smart_home_capability_type
from app.translator import translator_discovery, translator_query


def get_smart_home_capability(capability: Capability, capability_value) -> Dict:
    response = {
        DeviceCapability.TYPE: get_smart_home_capability_type(type(capability)),
        DeviceCapability.PARAMETERS: translator_discovery.get_smart_home_capability_parameters(capability),
        DeviceCapability.STATE: translator_query.get_smart_home_capability_state(type(capability), capability_value)
    }

    if not response[DeviceCapability.PARAMETERS]:
        del response[DeviceCapability.PARAMETERS]

    return response


def get_ui_device_capabilities(device: Device) -> List:
    capabilities = []
    for capability in device.capabilities:
        capabilities.append(
            get_smart_home_capability(device.capabilities[capability],
                                      translator_query.get_capability_value(capability, device))
        )
    return capabilities
