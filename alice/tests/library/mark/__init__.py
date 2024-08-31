import json

import alice.tests.library.region as region
import alice.tests.library.vault as vault


class MarkObject(object):
    _base_marks = set([
        'surface',
        'text',
        'voice',
        'iot',
        'oauth',
        'no_oauth',
        'experiments',
        'environment_state',
        'region',
        'supported_features',
        'unsupported_features',
        'permissions',
        'device_state',
        'app',
        'version',
        'version_lt',
        'version_le',
        'version_ge',
        'version_gt',
        'predefined_contacts',
        'locale',
    ])

    def __init__(self, additional_marks=[]):
        self._marks = MarkObject._base_marks.union(additional_marks)
        for name in self._marks:
            setattr(self, name, name)

    @property
    def names(self):
        return self._marks


Mark = MarkObject()


def _iter_markers(request, name):
    for node in request.node.listchain():
        for marker in node.own_markers:
            if marker.name == name:
                yield marker


def get_closest_marker(node, names):
    for marker in node.iter_markers():
        if marker.name in names:
            return marker


def _get_data(request, mark_name, default_value):
    def _to_dict(array):
        return {_: default_value for _ in array if _}

    data = getattr(request.cls, mark_name, {})
    if not isinstance(data, dict):
        data = _to_dict(data)

    for marker in _iter_markers(request, mark_name):
        data.update(_to_dict(marker.args))
        data.update(marker.kwargs)

    return data


def get_oauth_token(request):
    marker = get_closest_marker(request.node, [Mark.oauth, Mark.no_oauth])
    if not marker or marker.name == Mark.no_oauth:
        return None

    assert marker.args, f'Marker "{Mark.oauth}" must have username value'
    username = marker.args[0]
    if username.startswith('FAKE_'):
        return username
    return vault.get_oauth_token(username=username, secret_uuid=marker.kwargs.get('secret_uuid'))


def get_region(request):
    marker = request.node.get_closest_marker(Mark.region)
    if not marker:
        return region.Moscow

    if marker.args:
        test_region = marker.args[0]
        test_region.user_defined_region_id = marker.kwargs.get('user_defined_region_id')
        return test_region

    return region.Region(**marker.kwargs)


def get_device_state(request):
    device_state = getattr(request.cls, Mark.device_state, {})
    if isinstance(device_state, (str, bytes, )):
        device_state = json.loads(device_state)

    for marker in _iter_markers(request, Mark.device_state):
        if marker.args:
            device_state.update(marker.args[0])
        device_state.update(marker.kwargs)

    return device_state


def get_experiments(request):
    return _get_data(request, Mark.experiments, default_value='1')


def get_supported_features(request):
    return _get_data(request, Mark.supported_features, default_value=True)


def get_unsupported_features(request):
    return _get_data(request, Mark.unsupported_features, default_value=True)


def get_application(request):
    marker = request.node.get_closest_marker(Mark.app)
    return marker.kwargs if marker else {}


def get_environment_state(request):
    marker = request.node.get_closest_marker(Mark.environment_state)
    return marker.args[0] if marker else None


def get_iot(request):
    marker = request.node.get_closest_marker(Mark.iot)
    return marker.args[0] if marker else None


def get_predefined_contacts(request):
    marker = request.node.get_closest_marker(Mark.predefined_contacts)
    return json.dumps(marker.args[0]) if marker else None


def get_permissions(request):
    permissions = _get_data(request, Mark.permissions, default_value=True)
    return [{'name': k, 'granted': v} for k, v in permissions.items()]


def get_version_marker(node):
    for marker in node.iter_markers():
        if marker.name.startswith(Mark.version):
            return marker


def get_input_marker(request):
    return get_closest_marker(request.node, [Mark.voice, Mark.text])


def is_voice_input(request):
    marker = get_input_marker(request)
    return marker and marker.name == Mark.voice
