import json
import logging
import os
import re
from retry import retry
from typing import Dict, List, Optional
import yalibrary.upload.uploader as uploader
from yalibrary.yandex.sandbox import fetcher as sandbox_fetcher
import yatest.common as yc

from alice.library.python.eventlog_wrapper.eventlog_wrapper import ApphostEventlogWrapper, parse_eventlog
from alice.megamind.mit.library.util import (
    EVENTLOG_FILE_NAME,
    create_file_dir_if_not_exist,
    is_generator_mode,
)


logger = logging.getLogger(__name__)

StubData = Dict[str, Dict[str, ApphostEventlogWrapper]]


def _data_inc_path(module_path: str) -> str:
    return yc.source_path(os.path.join(module_path, 'eventlog_data.inc'))


def _load_eventlog_from_string(raw_data_str: str) -> StubData:
    stubs: StubData = dict()
    raw_data = json.loads(raw_data_str)
    assert isinstance(raw_data, dict)
    for test_name, eventlogs in raw_data.items():
        assert isinstance(eventlogs, dict)
        current_test_data: Dict[str, ApphostEventlogWrapper] = dict()
        for request_id, eventlog in eventlogs.items():
            assert isinstance(eventlog, list)
            current_test_data[request_id] = parse_eventlog(eventlog)
        stubs[test_name] = current_test_data
    return stubs


@retry(tries=5, delay=1)
def _try_download_resource(resource_id, dst_path):
    logger.info(f'Downloading resource {resource_id} into {dst_path}')
    sandbox_fetcher.download_resource(resource_id, dst_path, methods=['skynet', 'http_tgz'])


def _get_resource_id_from_data_inc_file(data_inc_path: str) -> Optional[int]:
    with open(data_inc_path, 'r') as f:
        regexp = re.compile('\\d+')
        match = regexp.search(f.read())
        if match:
            return int(match.group(0))
    return None


def _load_eventlog(eventlog_path: str, module_path: str) -> StubData:
    if is_generator_mode():
        # To be able to recananize one test in module (and leave stubs for other tests as is)
        # loading from sandbox manually
        # because eventlog_data.inc INCLUDEd in common.inc only for runner mode
        # this method would not work in runner mode because network is forbidden in CI
        data_inc_path = _data_inc_path(module_path)
        if not os.path.exists(data_inc_path):
            return dict()
        resource_id = _get_resource_id_from_data_inc_file(data_inc_path)
        if not resource_id:
            return dict()
        temp_file_path = yc.output_path(os.path.join(module_path, f'{EVENTLOG_FILE_NAME}_temp_for_generator'))
        _try_download_resource(resource_id, temp_file_path)
        if not os.path.exists(temp_file_path):
            logger.error(f'Downloading resource {resource_id} failed, no file {temp_file_path}')
            return dict()
        with open(temp_file_path, 'r') as f:
            return _load_eventlog_from_string(f.read())

    with open(eventlog_path, 'r') as f:
        return _load_eventlog_from_string(f.read())


def _dump_eventlog(stubs: StubData, module_path: str):
    temp_file_path = yc.output_path(os.path.join(module_path, EVENTLOG_FILE_NAME))
    logger.debug(f'Writing eventlog to temp file: {temp_file_path}')
    raw_data: Dict[str, Dict[str, List]] = dict()
    for test_name, test_data in stubs.items():
        raw_data[test_name] = dict()
        for request_id, eventlog_wrapper in test_data.items():
            raw_data[test_name][request_id] = [r.raw_event for r in eventlog_wrapper.records]

    create_file_dir_if_not_exist(temp_file_path)
    with open(temp_file_path, 'w') as f:
        f.write(json.dumps(raw_data, indent=2, ensure_ascii=False, sort_keys=True))
    resource_id = uploader.do(
        [temp_file_path],
        resource_type='OTHER_RESOURCE',
        resource_description=f'Eventlog json file for MIT test module: {module_path}',
        resource_owner='BASS',
        ttl='inf',
        sandbox_url='https://sandbox.yandex-team.ru'
    )
    logger.info(f'Eventlog sandbox resource id: {resource_id}')
    data_inc_path = _data_inc_path(module_path)
    logger.debug(f'Writing eventlog_data.inc file to path {data_inc_path}')
    create_file_dir_if_not_exist(data_inc_path)
    with open(data_inc_path, 'w') as f:
        f.write(f'DATA(\n    sbr://{resource_id}\n)\n')


class StubberRepository(object):

    def __init__(self, eventlog_path: str, module_path: str):
        self._eventlog_path: str = eventlog_path
        self._module_path: str = module_path
        self._data_has_changed = False
        self._stubs: StubData = dict()

    def __enter__(self):
        self._stubs = _load_eventlog(self._eventlog_path, self._module_path)
        return self

    def __exit__(self, _type, _value, _traceback):
        if self._data_has_changed:
            _dump_eventlog(self._stubs, self._module_path)

    def get_eventlog(self, test_name: str) -> Dict[str, ApphostEventlogWrapper]:
        if is_generator_mode():
            return dict()
        return self._stubs[test_name]

    def save_eventlog(self, test_name: str, request_id: str, eventlog_wrapper: ApphostEventlogWrapper):
        self._data_has_changed = True
        if test_name not in self._stubs:
            self._stubs[test_name] = dict()
        self._stubs[test_name][request_id] = eventlog_wrapper
