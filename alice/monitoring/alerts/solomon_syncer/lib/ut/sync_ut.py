import json
from unittest import mock, TestCase

import requests
from alice.monitoring.alerts.solomon_syncer.lib.sync import (get_alert_ids, create_alert, request, update_alert,  # noqa
                                                             sync_alerts, Alert, get_alert)


@mock.patch('requests.request')
def test_get_alert(mock_request):
    response = mock.Mock()
    response.content = json.dumps({
        'id': 'logbroker-megamind-messages-written-original',
        'version': 1
    })

    mock_request.return_value = response

    expected = Alert('logbroker-megamind-messages-written-original', 1)
    actual = get_alert('token', 'megamind', 'logbroker-megamind-messages-written-original')

    assert expected.id == actual.id
    assert expected.version == actual.version


@mock.patch('requests.request')
def test_get_alert_ids(mock_request):
    response1 = mock.Mock()
    response1.content = json.dumps({
        'items': [
            {
                'id': 'cofe-abt-main-readyness',
                'projectId': 'megamind'
            },
            {
                'id': 'alicelogs_lag',
                'projectId': 'megamind'
            },
            {
                'id': 'alicelogs_dropped_messages',
                'projectId': 'megamind'
            },
            {
                'id': 'always-error',
                'projectId': 'megamind'
            },
            {
                'id': 'bass_traffic_forecast_decompress_error',
                'projectId': 'megamind'
            }
        ],
        'nextPageToken': '5'
    })
    response2 = mock.Mock()
    response2.content = json.dumps({
        'items': [
            {
                'id': 'bass_traffic_forecast_old_resource_error',
                'projectId': 'megamind'
            },
            {
                'id': 'video_trailer_not_200',
                'projectId': 'megamind'
            },
            {
                'id': 'megamind-stack-engine-errors',
                'projectId': 'megamind'
            },
            {
                'id': 'source_reqwizard_fail',
                'projectId': 'megamind'
            },
            {
                'id': 'source_vins_fails',
                'projectId': 'megamind'
            }
        ],
    })

    mock_request.side_effect = [response1, response2]

    alerts = get_alert_ids('token', 'megamind')

    expected = ['cofe-abt-main-readyness', 'alicelogs_lag', 'alicelogs_dropped_messages', 'always-error',
                'bass_traffic_forecast_decompress_error', 'bass_traffic_forecast_old_resource_error',
                'video_trailer_not_200', 'megamind-stack-engine-errors', 'source_reqwizard_fail', 'source_vins_fails']

    assert expected == alerts


class TestHttpError(TestCase):
    @mock.patch('requests.request')
    def test_get_alert_ids_error(self, mock_request):
        response = mock.Mock()
        response.status_code = 500
        response.raise_for_status = mock.Mock(side_effect=requests.exceptions.HTTPError())

        mock_request.return_value = response

        with self.assertRaises(requests.exceptions.HTTPError):
            get_alert_ids('token', 'megamind')


@mock.patch('alice.monitoring.alerts.solomon_syncer.lib.sync.request')
def test_create_alert(mock_request):
    alert = {
        'id': 'test-alert-ran1s-delete',
    }

    create_alert('token', 'megamind', alert)

    mock_request.assert_called_once_with('token', 'projects/megamind/alerts', 'POST', data=alert)


class TestUpdateAlert(TestCase):
    @mock.patch('alice.monitoring.alerts.solomon_syncer.lib.sync.request')
    def test_update_alert(self, mock_request):
        response1 = mock.MagicMock()
        response1.return_value = {
            'id': 'logbroker-megamind-messages-written-original',
            'version': 1
        }

        response2 = mock.Mock()

        mock_request.side_effect = [response1, response2]

        alert = {
            'id': 'test-alert-ran1s-delete',
        }

        update_alert('token', 'megamind', alert)

        self.assertEquals(mock_request.mock_calls, [
            mock.call('token', 'projects/megamind/alerts/test-alert-ran1s-delete', 'GET'),
            mock.call('token', 'projects/megamind/alerts/test-alert-ran1s-delete', 'PUT', data=alert)
        ])


class TestSyncAlerts(TestCase):
    @mock.patch('alice.monitoring.alerts.solomon_syncer.lib.sync.get_alert_ids', return_value=['a', 'b', 'c'])
    @mock.patch('alice.monitoring.alerts.solomon_syncer.lib.sync.create_alert')
    @mock.patch('alice.monitoring.alerts.solomon_syncer.lib.sync.update_alert')
    def test_sync_alerts(self, mock_update_alert, mock_create_alert, mock_get_alert_ids):
        alerts = [{
            'id': 'a',
        }, {
            'id': 'b',
        }, {
            'id': 'c',
        }, {
            'id': 'd',
        }, {
            'id': 'e',
        }, {
            'id': 'f',
        }, {
            'id': 'g',
        }]

        sync_alerts('token', 'megamind', alerts)

        mock_get_alert_ids.assert_called_once_with('token', 'megamind')

        self.assertEquals(mock_create_alert.mock_calls, [
            mock.call('token', 'megamind', {'id': 'd'}),
            mock.call('token', 'megamind', {'id': 'e'}),
            mock.call('token', 'megamind', {'id': 'f'}),
            mock.call('token', 'megamind', {'id': 'g'})
        ])

        self.assertEquals(mock_update_alert.mock_calls, [
            mock.call('token', 'megamind', {'id': 'a'}),
            mock.call('token', 'megamind', {'id': 'b'}),
            mock.call('token', 'megamind', {'id': 'c'})
        ])
