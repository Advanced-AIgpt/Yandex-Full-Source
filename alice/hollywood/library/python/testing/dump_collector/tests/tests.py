import logging
from pathlib import Path


import pytest
from alice.hollywood.library.python.testing.dump_collector import DumpCollector
from alice.tests.library.service import AppHost
from google.protobuf import text_format
from library.python import resource


logger = logging.getLogger(__name__)


@pytest.fixture
def apphost():
    return AppHost(port=0)


@pytest.fixture
def eventlog_file(apphost, eventlog_resource_filename):
    eventlog_path = Path(apphost.local_app_host_dir, 'ALICE', f'eventlog-{apphost.port}')
    eventlog_path.parent.mkdir(parents=True, exist_ok=True)
    logger.info(f'eventlog_filepath = {eventlog_path}')

    eventlog_json_lines = resource.find(eventlog_resource_filename)
    with eventlog_path.open('wb') as out_f:
        out_f.write(eventlog_json_lines)
    return len(eventlog_json_lines)


@pytest.mark.parametrize('scenario_name, request_id, scenario_stages, eventlog_resource_filename', [
    ('HollywoodMusic', 'd81edcbe-e909-55d3-ab82-57dabbadoo00', {'run', 'continue'}, 'eventlog-run_contiue'),
    ('ZeroTesting', 'ac9a02eb-3d70-5575-ac52-31dabbadoo00', {'run'}, 'eventlog-run'),
    ('HollywoodMusic', '14f9a554-8987-5faa-9666-53dabbadoo01', {'run', 'apply'}, 'eventlog-run_apply'),
    ('HollywoodMusic', 'b532437e-2cbb-573a-97d1-0edabbadoo01', {'run', 'commit'}, 'eventlog-run_commit'),
    ('Video', '37b103c2-40e8-53c5-92fa-7fdabbadoo00', {'run'}, 'eventlog-app_host_copy_run'),
])
def test_extract_requests_and_responses_from_eventlog(
    apphost, eventlog_file, scenario_name, request_id, scenario_stages,
):
    assert eventlog_file

    dumps_collector = DumpCollector(apphost)
    requests, responses, sources_dump = dumps_collector.extract_requests_and_responses_from_eventlog(
        scenario_name, request_id, flush_eventlog=False,
    )

    assert set(requests.keys()) == scenario_stages
    assert set(responses.keys()) == scenario_stages

    result = []
    for scenario_stage, request in requests.items():
        result.append(f'Content of {scenario_stage} request:')
        result.append(text_format.MessageToString(request, as_utf8=True))
        result.append('\n')

    for scenario_stage, response in responses.items():
        result.append(f'Content of {scenario_stage} response:')
        result.append(text_format.MessageToString(response, as_utf8=True))
        result.append('\n')

    for node_name, r in sources_dump.http_requests.items():
        result.append(f'Context of {node_name} http request:')
        result.append(f'Path: {r.path}')
        result.append(f'Headers count: {len(r.headers)}')
        result.append(f'Content: {r.content}')
        result.append('\n')

    for node_name, r in sources_dump.http_responses.items():
        result.append(f'Context of {node_name} http response:')
        result.append(f'Status code: {r.status_code}')
        result.append(f'Headers count: {len(r.headers)}')
        result.append(f'Content: {r.content}')
        result.append('\n')

    grpc_requests_nodes = []
    grpc_requests_answers_count = 0
    for node_name, r in sources_dump.grpc_requests.items():
        grpc_requests_nodes.append(node_name)
        grpc_requests_answers_count += len(r.answers)
    result.append(f'Grpc requests count: {len(grpc_requests_nodes)}')
    result.append(f'Grpc requests answers total count: {grpc_requests_answers_count}')

    grpc_responses_nodes = []
    grpc_responses_answers_count = 0
    for node_name, r in sources_dump.grpc_responses.items():
        grpc_responses_nodes.append(node_name)
        grpc_responses_answers_count += len(r.answers)
    result.append(f'Grpc responses count: {len(grpc_responses_nodes)}')
    result.append(f'Grpc responses answers total count: {grpc_responses_answers_count}')

    return '\n'.join(result)
