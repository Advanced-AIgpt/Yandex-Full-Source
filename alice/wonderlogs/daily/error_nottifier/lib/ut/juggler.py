import json
import requests_mock

from alice.wonderlogs.daily.error_nottifier.lib.juggler import Status, send_raw_event, JUGGLER_EVENTS_URI


def test_status():
    ok = Status.OK
    warn = Status.WARN
    crit = Status.CRIT

    assert warn.worse(ok)
    assert crit.worse(ok)
    assert crit.worse(warn)

    assert not ok.worse(ok)
    assert not ok.worse(warn)
    assert not ok.worse(crit)

    assert not warn.worse(warn)
    assert not warn.worse(crit)

    assert not crit.worse(crit)

    assert 'OK' == str(Status(Status.OK))
    assert 'WARN' == str(Status(Status.WARN))
    assert 'CRIT' == str(Status(Status.CRIT))


def test_send_raw_event():
    with requests_mock.Mocker() as m:
        success = '{"accepted_events": 1, "events": [{"code": 200}], "success": true}'
        m.post(JUGGLER_EVENTS_URI, text=success)

        host = 'host'
        service = 'service'
        status = 'OK'
        tags = ['abc', 'cde']
        source = 'source'
        description = 'description'

        response = send_raw_event(host, service, status, tags, source, description)

        assert response.text == success
        assert m.called
        assert m.call_count == 1

        request = m.request_history[0]
        assert request.method == 'POST'
        assert request.url == JUGGLER_EVENTS_URI

        data = json.loads(request.text)
        assert data['source'] == source

        event = data['events'][0]
        assert event['description'] == description
        assert event['host'] == host
        assert event['service'] == service
        assert event['status'] == status
        assert event['tags'] == tags
