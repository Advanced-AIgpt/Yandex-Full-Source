def _iter_markers(request, mark_name):
    for node in request.node.listchain():
        for marker in node.own_markers:
            if marker.name == mark_name:
                yield marker


def get_mock(request):
    data = {}
    for marker in _iter_markers(request, 'mock'):
        data.update(marker.args[0])
    return data


def get_graph_name(request):
    marker = request.node.get_closest_marker('graph_name')
    if marker:
        return marker.args[0]
    raise Exception('Please use @pytest.mark.graph_name in test!')
