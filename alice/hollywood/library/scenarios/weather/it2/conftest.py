from datetime import datetime

import pytest
from alice.hollywood.library.python.testing.it2.stubber import StubberEndpoint, create_stubber_fixture
from alice.megamind.protos.scenarios.response_pb2 import TScenarioRunResponse
from alice.protos.api.renderer.api_pb2 import TDivRenderData, TRenderResponse
from weather.app_host.forecast_postproc.protos.forecast_pb2 import TForecast
from weather.app_host.geo_location.protos.request_pb2 import TGeoLocationRequest
from weather.app_host.protos.timespan_pb2 import TTimeSpan
from weather.app_host.v3_nowcast_alert_response.protos.v3_nowcast_alert_response_pb2 import TV3NowcastAlertResponse as TAlert, TAlertRequest
from weather.app_host.warnings.protos.warnings_pb2 import TWarnings


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['weather']


geosearch_stubber = create_stubber_fixture(
    'addrs.yandex.ru',
    17140,
    [
        StubberEndpoint('/yandsearch', ['GET']),
    ],
    stubs_subdir='geosearch_back',
)


reqwizard_stubber = create_stubber_fixture(
    'reqwizard.yandex.net',
    8891,
    [
        StubberEndpoint('/wizard', ['GET']),
    ],
    stubs_subdir='reqwizard_back',
)


weather_graph_stubber = create_stubber_fixture(
    host=('sas', 'weather-test-app-host_sas'),
    port=81,
    scheme='grpc',
    stubs_subdir='weather_back',
    type_to_proto={
        # request protobufs
        'TAlertRequest': TAlertRequest,
        'TGeoLocationRequest': TGeoLocationRequest,
        'TTimeSpan': TTimeSpan,

        # response protobufs
        'TAlert': TAlert,
        'TForecast': TForecast,
        'TWarnings': TWarnings,
    },
)


div_render_graph_stubber = create_stubber_fixture(
    ('sas', 'div-renderer-prestable.div-renderer'),
    10000,
    [
        StubberEndpoint('/render', ['POST']),
    ],
    stubs_subdir='div_render_back',
    type_to_proto={
        'mm_scenario_response': TScenarioRunResponse,
        'render_data': TDivRenderData,
        'render_result': TRenderResponse,
    },
    pseudo_grpc=True,
    header_filter_regexps=['content-length'],
)


@pytest.fixture(scope="function")
def srcrwr_params(geosearch_stubber, reqwizard_stubber, weather_graph_stubber, div_render_graph_stubber, apphost):
    return {
        'WEATHER_GEOMETASEARCH_PROXY': f'localhost:{geosearch_stubber.port}',
        'WEATHER_REQWIZARD_PROXY': f'localhost:{reqwizard_stubber.port}',
        'WEATHER_GRAPH': f'localhost:{weather_graph_stubber.port}',
        'RENDER_DIV2': f'localhost:{div_render_graph_stubber.port}',
    }


@pytest.fixture(scope="function")
def experiments(experiments):
    mock_time = datetime.utcnow().strftime("%Y-%m-%dT%H:00:00")
    experiments[f'mock_time={mock_time}'] = '1'  # for execution in generator mode
    return experiments
