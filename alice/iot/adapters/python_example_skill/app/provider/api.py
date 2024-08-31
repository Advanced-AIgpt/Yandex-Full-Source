import logging
from typing import List, Dict
from app.translator import translator, translator_action, translator_discovery, translator_query, translator_ui
from app.translator.translator import DeviceInfo, ActionResult
from app.provider.action_answers import DeviceNotFound
from app.provider.provider import provider


logging.basicConfig(format="%(levelname)s PDevice.ID %(process)d TDevice.ID %(thread)d [%(asctime)s]: %(message)s\n",
                    level=logging.INFO, filename="/app/app.log")


def get_error_response(device_id: str) -> Dict:
    result = DeviceNotFound("Device" + device_id + "not found")
    return {
        DeviceInfo.ID: device_id,
        ActionResult.STATUS: ActionResult.ERROR,
        ActionResult.CODE: translator.get_smart_home_error_code(result),
        ActionResult.MESSAGE: result.message
    }


def discovery_devices() -> List:
    devices_response = []
    for device_id in provider.id_to_device:
        device = provider.id_to_device[device_id]
        logging.info("Discovering device: %r", device.id)

        device_response = {
            DeviceInfo.ID: device.id,
            DeviceInfo.NAME: device.name,
            DeviceInfo.TYPE: translator.get_smart_home_device_type(type(device)),
            DeviceInfo.CAPABILITIES: translator_discovery.get_smart_home_device_capabilities(device)
        }

        devices_response.append(device_response)
    return devices_response


def query_devices(request_devices: List) -> List:
    devices_response = []
    for request_device in request_devices:
        if request_device[DeviceInfo.ID] not in provider.id_to_device:
            logging.warning("Device: %r not found", request_device[DeviceInfo.ID])
            devices_response.append(get_error_response(request_device[DeviceInfo.ID]))
            continue

        device = provider.id_to_device[request_device[DeviceInfo.ID]]
        logging.info("Querying device: %r", device.id)

        device_response = {
            DeviceInfo.ID: device.id,
            DeviceInfo.CAPABILITIES: translator_query.get_smart_home_device_capabilities(device)
        }

        devices_response.append(device_response)
    return devices_response


def action_devices(request_devices: List) -> List:
    devices_response = []
    for request_device in request_devices:
        if request_device[DeviceInfo.ID] not in provider.id_to_device:
            logging.warning("Device: %r not found", request_device[DeviceInfo.ID])
            devices_response.append(get_error_response(request_device[DeviceInfo.ID]))
            continue

        device = provider.id_to_device[request_device[DeviceInfo.ID]]
        logging.info("Changing device %r", device.id)

        device_response = {
            DeviceInfo.ID: device.id,
            DeviceInfo.CAPABILITIES:
                translator_action.get_smart_home_device_capabilities(device, request_device[DeviceInfo.CAPABILITIES])
        }

        devices_response.append(device_response)
    return devices_response


def ui_devices() -> List:
    devices_response = []
    for device_id in provider.id_to_device:
        device = provider.id_to_device[device_id]

        device_response = {
            DeviceInfo.ID: device.id,
            DeviceInfo.NAME: device.name,
            DeviceInfo.TYPE: translator.get_smart_home_device_type(type(device)),
            DeviceInfo.CAPABILITIES: translator_ui.get_ui_device_capabilities(device)
        }

        devices_response.append(device_response)
    return devices_response
