# coding: utf-8

import base64
import logging

from lxml import etree
from urllib.parse import urlparse, unquote_plus


logger = logging.getLogger(__name__)


def _append_address_node(node, name, lat, lon):
    if lat is None or lon is None:
        return
    node_method = etree.SubElement(node, name)
    element = etree.SubElement(node_method, 'address')
    element.set('lat', str(float(lat)))
    element.set('lon', str(float(lon)))
    element.set('title', '')
    element.set('subtitle', '')


def _ignore_command():
    xml = etree.Element('yari')
    etree.SubElement(xml, 'ignore')
    return xml


def _route_reset_command(data, text, description, commands):
    if 'route_reset' in commands:
        xml = etree.Element('yari')
        node = etree.SubElement(xml, 'route_reset')
        node.set('description', description)
        return xml

    return _ignore_command()


def _show_me_command(data, text, description, commands):
    if 'show_me' in commands:
        xml = etree.Element('yari')
        node = etree.SubElement(xml, 'show_me')
        node.set('description', description)
        return xml

    return _ignore_command()


def _route_view_command(data, text, description, commands):
    if 'route_view' in commands:
        xml = etree.Element('yari')
        node = etree.SubElement(xml, 'route_view')
        node.set('description', description)
        return xml

    return _ignore_command()


def _parking_route_command(data, text, description, commands):
    if 'parking' in commands:
        xml = etree.Element('yari')
        node = etree.SubElement(xml, 'parking_route')
        node.set('description', description)
        return xml

    return _map_search_command(data, text, description, commands)


def _traffic_switch_layers_command(data, text, description, commands):
    xml = etree.Element('yari')
    node = etree.SubElement(xml, 'traffic')
    node.set('description', description)
    node.text = 'on' if data.get('traffic_on', '0') == '1' else 'off'
    return xml


def _parking_command(data, text, description, commands):
    if 'parking' in commands:
        xml = etree.Element('yari')
        node = etree.SubElement(xml, 'parking')
        node.set('description', description)
        node.text = 'on' if data.get('carparks_enabled', '0') == '1' else 'off'
        return xml

    # Search parkings on the map, it's better than ignore them
    return _map_search_command(data, text, description, commands)


def _add_point_command(data, text, description, commands):
    xml = etree.Element('yari')
    node = etree.SubElement(xml, 'add_point')
    node.set('description', description)
    node.set('category', data['category'])
    if data.get('where', None):
        node.set('where', data['where'])
    node.text = data.get('comment', '')
    return xml


def _confirmation_command(data, text, description, commands):
    xml = etree.Element('yari')
    confirmation = etree.SubElement(xml, 'confirmation')
    confirmation.text = 'Yes' if data['confirmed'] == '1' else 'No'
    return xml


def _route_command(data, text, description, commands):
    xml = etree.Element('yari')
    route_node = etree.SubElement(xml, 'route')
    route_node.set('description', description)

    _append_address_node(route_node, 'from', data.get('lat_from', None), data.get('lon_from', None))
    _append_address_node(route_node, 'to', data.get('lat_to', None), data.get('lon_to', None))
    _append_address_node(route_node, 'via', data.get('lat_via_0', None), data.get('lon_via_0', None))

    return xml


def _map_search_command(data, text, description, commands):
    xml = etree.Element('yari')
    n_search = etree.SubElement(xml, 'map_search')
    n_search.set('description', description)

    search_node = etree.SubElement(n_search, 'text')
    search_node.text = data.get('text', text)

    return xml


def parse_speech_kit_protocol(url):
    parsed_url = urlparse(url)

    payload = {}
    for query_item in parsed_url.query.split('&'):
        kv = query_item.split('=')
        if len(kv) == 2:
            payload[unquote_plus(kv[0])] = unquote_plus(kv[1])
    return parsed_url.netloc + parsed_url.path, payload


def build_command(data):
    return {
        'app': 'navi',
        'cmd': 'exec',
        'data': base64.b64encode(data).decode(),
    }


def dont_understand_command(text):
    xml = etree.Element('yari')
    node = etree.SubElement(xml, 'dont_understand')
    node.text = text
    return build_command(etree.tostring(xml, encoding='UTF-8'))


class NaviAPIResponse(object):
    _HANDLERS = {
        'traffic': _traffic_switch_layers_command,
        'show_ui/map': _parking_command,
        'add_point': _add_point_command,
        'external_confirmation': _confirmation_command,
        'carparks_route': _parking_route_command,
        'show_user_position': _show_me_command,
        'build_route_on_map': _route_command,
        'show_route_overview': _route_view_command,
        'clear_route': _route_reset_command,
        'map_search': _map_search_command,
    }

    def __init__(self, url, text, lang):
        self._name, self._data = parse_speech_kit_protocol(url)
        self._text = text
        self._lang = lang

    def build_xml(self, commands):
        if self._name not in self._HANDLERS:
            logger.error('Action %s not implemented yet', self._name)
            return _ignore_command()
        return self._HANDLERS[self._name](self._data, self._text, self._lang, commands)

    def build_commands(self, commands):
        xml = self.build_xml(commands)
        return build_command(etree.tostring(xml, encoding='UTF-8'))
