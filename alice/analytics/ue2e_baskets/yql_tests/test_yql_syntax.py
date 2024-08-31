import glob
import os.path
import pytest

import yql.library.fastcheck.python as fastcheck
import yatest.common

YQL_PATH = 'alice/analytics/ue2e_baskets'
YQL_EXTENSION = 'yql'
SKIP_CHECK_LIST = ('basket_stratification_and_sampling.yql',)

yql_filenames = glob.glob(yatest.common.source_path(os.path.join(YQL_PATH, '*.'+YQL_EXTENSION)))


@pytest.mark.parametrize('yql_filename', yql_filenames, ids=os.path.basename)
def test_basic(yql_filename):
    with open(yql_filename) as yql_file:
        script = yql_file.read()
        errors = []
        valid = fastcheck.check_program(script, cluster_mapping={'hahn': 'yt'}, errors=errors, syntax_version=1)
        # yql fastcheck not working on nirvanized queries, quick fix is skipping them
        if os.path.basename(yql_filename) in SKIP_CHECK_LIST:
            valid = True
        assert valid, ('filename= ' + yql_filename + ' \n') + ''.join((str(e) for e in errors))
