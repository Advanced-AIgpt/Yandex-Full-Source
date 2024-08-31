import re


app_version_pattern = re.compile("^(?P<board>[^@]*)@(?P<platform>[^@]*)@(?P<firmware_version>[^@]*)(@(?P<build>[^@]*))?$")


def get_diagonal(stat_dict, manufacturer, model):
    try:
        diagonal = stat_dict[manufacturer.upper()][model]['diagonal']
    except:
        diagonal = None

    return diagonal


def get_resolution(stat_dict, manufacturer, model):
    try:
        resolution = stat_dict[manufacturer.upper()][model]['resolution']
    except:
        resolution = None

    return resolution


def parse_app_version(app_version):
    if not isinstance(app_version, str):
        return {}

    match = re.match(app_version_pattern, app_version)

    if not match:
        return {}

    return dict(map(lambda x: (x[0], x[1] or None), match.groupdict().items()))
