import yatest.common
import yatest.common.network
import yatest.common.runtime
import yatest.common.process
from time import sleep
import pytest


@pytest.mark.parametrize("cfg_path,bucket_path,index_dir_path", [
    ('alice/boltalka/extsearch/base/nlgsearch/search.cfg', 'alice/boltalka/extsearch/tests/bucket.tskv', 'index')
])
def test_nlgsearch_server(cfg_path, bucket_path, index_dir_path):
    with yatest.common.network.PortManager() as port_manager:
        nlgsearch_path = yatest.common.binary_path('alice/boltalka/extsearch/base/nlgsearch/nlgsearch')
        cfg_path = yatest.common.source_path(cfg_path)
        bucket_path = yatest.common.source_path(bucket_path)
        index_dir_path = yatest.common.runtime.work_path(index_dir_path)
        query_basesearch_path = yatest.common.source_path('alice/boltalka/extsearch/scripts/query_basesearch.py')
        port = port_manager.get_port()
        process = yatest.common.process.execute([nlgsearch_path, '-p', str(port), '-d', cfg_path, '-V', 'IndexDir=' + index_dir_path], wait=False)
        timeout = 10
        while port_manager.is_port_free(port):
            assert process.running
            sleep(timeout)
        res = None
        with open(bucket_path) as f:
            res = yatest.common.canonical_py_execute(query_basesearch_path, ['--port', str(port), '--ranker', 'catboost', '--from-yt', '--ranker', 'catboost'], stdin=f)
        try:
            process.kill()
        except yatest.common.process.TimeoutError:
            pass
        return res
