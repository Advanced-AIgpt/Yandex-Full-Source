import pytest
from alice.uniproxy.tools.wsproxy_admin import RequestArguments, _build_url, _update_url_to_admin


EPS = 1e-9


def test_request_arguments(argparser):

    req_arg = RequestArguments(argparser.parse_args(
        ['--service', 'wsproxy-vla,wsproxy-man,wsproxy-sas', '--apply-to-all', 'disconnect', '--ratio', '1.0']))
    assert req_arg.action == 'disconnect_clients'
    assert req_arg.services == ['wsproxy-vla', 'wsproxy-man', 'wsproxy-sas']
    assert req_arg.uuid is None
    assert req_arg.session_id is None
    assert req_arg.device_id is None
    assert req_arg.surface is None
    assert abs(req_arg.ratio - 1.0) < EPS

    req_arg = RequestArguments(argparser.parse_args(
        ['--service', 'wsproxy-vla', 'disconnect', '--uuid', '1234']))
    assert req_arg.action == 'disconnect_clients_with_filter'
    assert req_arg.uuid == '1234'

    req_arg = RequestArguments(argparser.parse_args(
        ['--service', 'wsproxy-vla', 'disconnect', '--ratio', '0.58']))
    assert req_arg.action == 'disconnect_clients'
    assert abs(req_arg.ratio - 0.58) < EPS

    req_arg = RequestArguments(
        argparser.parse_args(['--service', 'wsproxy-vla', 'suspend']))
    assert req_arg.action == 'suspend_accepting'

    req_arg = RequestArguments(
        argparser.parse_args(['--service', 'wsproxy-vla', 'resume']))
    assert req_arg.action == 'resume_accepting'

    req_arg = RequestArguments(
        argparser.parse_args(['--service', 'wsproxy-vla', 'shutdown']))
    assert req_arg.action == 'shutdown'

    req_arg = RequestArguments(argparser.parse_args(
        ['--host', 'a,b,c', 'disconnect', '--ratio', '1.0']))
    assert req_arg.action == 'disconnect_clients'
    assert req_arg.uuid is None
    assert req_arg.session_id is None
    assert req_arg.device_id is None
    assert req_arg.surface is None
    assert abs(req_arg.ratio - 1.0) < EPS

    req_arg = RequestArguments(argparser.parse_args(
        ['--host', 'a', 'disconnect', '--ratio', '1.0']))
    assert req_arg.action == 'disconnect_clients'

    req_arg = RequestArguments(
        argparser.parse_args(['--host', 'a', 'suspend']))
    assert req_arg.action == 'suspend_accepting'

    req_arg = RequestArguments(argparser.parse_args(['--host', 'a', 'resume']))
    assert req_arg.action == 'resume_accepting'

    req_arg = RequestArguments(
        argparser.parse_args(['--host', 'a', 'shutdown']))
    assert req_arg.action == 'shutdown'

    with pytest.raises(ValueError):
        req_arg = RequestArguments(argparser.parse_args(
            ['--service', 'wsproxy-vla', 'disconnect', '--uuid', '1234', '--surface', 'abc']))

    with pytest.raises(ValueError):
        req_arg = RequestArguments(argparser.parse_args(
            ['disconnect', '--uuid', '1234']))

    with pytest.raises(ValueError):
        req_arg = RequestArguments(argparser.parse_args(
            ['--service', 'wsproxy-vla', '--host', 'localhost', 'disconnect', '--uuid', '1234']))

    req_arg = RequestArguments(argparser.parse_args(
        ['--host', 'localhost', 'disconnect', '--uuid', '1234']))
    assert req_arg.action == 'disconnect_clients_with_filter'
    assert req_arg.uuid == '1234'

    with pytest.raises(ValueError):
        req_arg = RequestArguments(argparser.parse_args(
            ['--host', 'localhost', 'disconnect']))

    with pytest.raises(ValueError):
        req_arg = RequestArguments(argparser.parse_args(
            ['--host', 'localhost', 'disconnect', '--uuid', '1234', '--ratio', '0.7']))

    with pytest.raises(ValueError):
        req_arg = RequestArguments(argparser.parse_args(
            ['--host', 'localhost', 'disconnect', '--session-id', '1234', '--ratio', '0.7']))

    with pytest.raises(ValueError):
        req_arg = RequestArguments(argparser.parse_args(
            ['--host', 'localhost', 'disconnect', '--device-id', '1234', '--ratio', '0.7']))

    req_arg = RequestArguments(argparser.parse_args(
        ['--host', 'localhost', 'disconnect', '--surface', '1234', '--ratio', '0.7']))
    assert req_arg.action == 'disconnect_clients_with_filter'
    assert req_arg.surface == '1234'
    assert abs(req_arg.ratio - 0.7) < EPS

    with pytest.raises(ValueError):
        req_arg = RequestArguments(argparser.parse_args(
            ['--host', 'localhost', 'disconnect', '--ratio', '12']))

    with pytest.raises(ValueError):
        req_arg = RequestArguments(argparser.parse_args(
            ['--host', 'localhost', 'disconnect', '--ratio', '-1']))


def test_build_url(argparser):
    req_arg = RequestArguments(argparser.parse_args(
        ['--service', 'wsproxy-vla', 'disconnect', '--ratio', '0.75']))
    assert _build_url('http://abc/admin',
                      req_arg) == 'http://abc/admin?action=disconnect_clients&ratio=0.75'

    req_arg = RequestArguments(
        argparser.parse_args(['--service', 'wsproxy-vla', 'disconnect', '--uuid', '1234']))
    assert _build_url(
        'http://abc/admin', req_arg) == 'http://abc/admin?action=disconnect_clients_with_filter&filter_type=Uuid&filter_value=1234'

    req_arg = RequestArguments(argparser.parse_args(
        ['--service', 'wsproxy-vla', 'disconnect', '--device-id', '1234']))
    assert _build_url(
        'http://abc/admin', req_arg) == 'http://abc/admin?action=disconnect_clients_with_filter&filter_type=DeviceId&filter_value=1234'

    req_arg = RequestArguments(
        argparser.parse_args(['--service', 'wsproxy-vla', 'disconnect', '--session-id', '1234']))
    assert _build_url(
        'http://abc/admin', req_arg) == 'http://abc/admin?action=disconnect_clients_with_filter&filter_type=SessionId&filter_value=1234'

    req_arg = RequestArguments(
        argparser.parse_args(['--service', 'wsproxy-vla', 'disconnect', '--surface', '1234', '--ratio', '1.0']))
    assert _build_url(
        'http://abc/admin', req_arg) == 'http://abc/admin?action=disconnect_clients_with_filter&ratio=1.0&filter_type=Surface&filter_value=1234'

    req_arg = RequestArguments(
        argparser.parse_args(['--service', 'wsproxy-vla', 'disconnect', '--session-id', '1234']))
    assert _build_url(
        'http://abc/admin', req_arg) == 'http://abc/admin?action=disconnect_clients_with_filter&filter_type=SessionId&filter_value=1234'

    req_arg = RequestArguments(
        argparser.parse_args(['--service', 'wsproxy-vla', 'suspend']))
    assert _build_url('http://abc/admin',
                      req_arg) == 'http://abc/admin?action=suspend_accepting'

    req_arg = RequestArguments(
        argparser.parse_args(['--service', 'wsproxy-vla', 'resume']))
    assert _build_url('http://abc/admin',
                      req_arg) == 'http://abc/admin?action=resume_accepting'

    req_arg = RequestArguments(
        argparser.parse_args(['--service', 'wsproxy-vla', 'shutdown']))
    assert _build_url('http://abc/admin',
                      req_arg) == 'http://abc/admin?action=shutdown'


def test_update_url_to_admin():
    assert _update_url_to_admin('abc') == 'http://abc/admin'
