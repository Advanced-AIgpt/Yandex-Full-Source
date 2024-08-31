TANDEM_STATE = {
    "NONE": "",
    "CONNECTING": "0",
    "CONNECTED": "1"
}


def get_channel_type(channel):
    if channel == 'ott':
        return 'ott'

    return 'other_vh'


def get_license(license, view_type):
    if view_type == 'vod':
        if license:
            return license.lower()
        else:
            return 'avod'

    return None


def get_tandem_state(tandem_device_connected):
    return TANDEM_STATE.get(tandem_device_connected, "")
