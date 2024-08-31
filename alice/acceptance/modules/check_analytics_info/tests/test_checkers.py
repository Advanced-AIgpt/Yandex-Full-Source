import json
import os

import pytest

from alice.acceptance.modules.check_analytics_info.lib.checkers import (
    MegamindAnalyticsInfoChecker,
    MetaChecker,
    TunnellerChecker
)
from alice.acceptance.modules.check_analytics_info.lib.utils import collect_dump_data, generate_file_name
from library.python import resource

DATA_DIR = 'data'


@pytest.mark.parametrize('cls_checker', [MegamindAnalyticsInfoChecker, MetaChecker, TunnellerChecker])
def test_checker(cls_checker):
    for test_dir in (path.split(os.sep)[1] for path in resource.resfs_files(DATA_DIR)):
        checker = cls_checker()
        cur_dir = os.path.join(DATA_DIR, test_dir)
        input_table = json.loads(resource.resfs_read(os.path.join(cur_dir, 'input_table.json')))
        actual = collect_dump_data(checker, input_table)
        expected = json.loads(
            resource.resfs_read(os.path.join(cur_dir, '{}.json'.format(generate_file_name(checker)))))
        assert expected == json.loads(json.dumps(actual))
