import yatest.common
from nlgsearch_pytest_lib import local_canonical_file, get_file_hash

NLGSEARCH_SIMPLE_PATH = yatest.common.binary_path('alice/boltalka/extsearch/nlgsearch_simple/nlgsearch_simple')


def test_no_reranker():
    output_path = yatest.common.test_output_path('output.tsv')

    def run_no_reranker_nlgsearch(output_path, dssms, knn_indexes):
        base_dssm = dssms.split(',')[0]
        base_knn_index = knn_indexes.split(',')[0].split(':')[0]
        cmd = (
            NLGSEARCH_SIMPLE_PATH,
            '-i', yatest.common.work_path('input.tsv'),
            '-o', output_path,
            '--data', yatest.common.work_path('small_index'),
            '--dssms', dssms,
            '--base-dssm', base_dssm,
            '--knn-indexes', knn_indexes,
            '--base-knn-index', base_knn_index,
        )
        yatest.common.execute(cmd)

    run_no_reranker_nlgsearch(output_path, 'small_dssm', 'base:sns1400:dcl35000')
    output_hash = get_file_hash(output_path)

    run_no_reranker_nlgsearch(output_path, 'small_dssm,small_dssm_2', 'base:sns1400:dcl35000')
    assert output_hash == get_file_hash(output_path)
    run_no_reranker_nlgsearch(output_path, 'small_dssm', 'base:sns1400:dcl35000,assessors:sns1400:dcl35000')
    assert output_hash == get_file_hash(output_path)
    run_no_reranker_nlgsearch(output_path, 'small_dssm,small_dssm_2', 'base:sns1400:dcl35000,assessors:sns1400:dcl35000')
    assert output_hash == get_file_hash(output_path)

    return [local_canonical_file(output_path)]


def test_1dssm_1knn_reranker():
    output_path = yatest.common.test_output_path('output.tsv')
    cmd = (
        NLGSEARCH_SIMPLE_PATH,
        '-i', yatest.common.work_path('input.tsv'),
        '-o', output_path,
        '--data', yatest.common.work_path('small_index'),
        '--dssms', 'small_dssm',
        '--rerankers-to-load', 'catboost_1dssm:sf8',
        '--knn-indexes', 'base:sns1400:dcl35000',
    )
    yatest.common.execute(cmd)
    return [local_canonical_file(output_path)]


def test_1dssm_1knn_1factor_reranker():
    output_path = yatest.common.test_output_path('output.tsv')
    cmd = (
        NLGSEARCH_SIMPLE_PATH,
        '-i', yatest.common.work_path('input.tsv'),
        '-o', output_path,
        '--data', yatest.common.work_path('small_index'),
        '--dssms', 'small_dssm',
        '--factor-dssms', 'factor_dssm_0',
        '--rerankers-to-load', 'catboost:sf8',
        '--knn-indexes', 'base:sns1400:dcl35000',
    )
    yatest.common.execute(cmd)
    return [local_canonical_file(output_path)]


def test_2dssms_1knn_reranker():
    output_path = yatest.common.test_output_path('output.tsv')
    cmd = (
        NLGSEARCH_SIMPLE_PATH,
        '-i', yatest.common.work_path('input.tsv'),
        '-o', output_path,
        '--data', yatest.common.work_path('small_index'),
        '--dssms', 'small_dssm,small_dssm_2',
        '--base-dssm', 'small_dssm',
        '--rerankers-to-load', 'catboost:sf8',
        '--knn-indexes', 'base:sns1400:dcl35000',
    )
    yatest.common.execute(cmd)
    return [local_canonical_file(output_path)]


def test_1dssm_2knns_reranker():
    output_path = yatest.common.test_output_path('output.tsv')
    cmd = (
        NLGSEARCH_SIMPLE_PATH,
        '-i', yatest.common.work_path('input.tsv'),
        '-o', output_path,
        '--data', yatest.common.work_path('small_index'),
        '--dssms', 'small_dssm',
        '--rerankers-to-load', 'catboost_1dssm:sf8',
        '--knn-indexes', 'base:sns1400:dcl35000',
        '--base-knn-index', 'base',
    )
    yatest.common.execute(cmd)
    return [local_canonical_file(output_path)]


def test_1dssm_2knns_1factor_reranker():
    output_path = yatest.common.test_output_path('output.tsv')
    cmd = (
        NLGSEARCH_SIMPLE_PATH,
        '-i', yatest.common.work_path('input.tsv'),
        '-o', output_path,
        '--data', yatest.common.work_path('small_index'),
        '--dssms', 'small_dssm',
        '--factor-dssms', 'factor_dssm_0',
        '--rerankers-to-load', 'catboost:sf8',
        '--knn-indexes', 'base:sns1400:dcl35000',
        '--base-knn-index', 'base',
    )
    yatest.common.execute(cmd)
    return [local_canonical_file(output_path)]


def test_2dssms_2knns_reranker():
    output_path = yatest.common.test_output_path('output.tsv')
    cmd = (
        NLGSEARCH_SIMPLE_PATH,
        '-i', yatest.common.work_path('input.tsv'),
        '-o', output_path,
        '--data', yatest.common.work_path('small_index'),
        '--dssms', 'small_dssm,small_dssm_2',
        '--base-dssm', 'small_dssm',
        '--rerankers-to-load', 'catboost:sf8,assessors_catboost:sf8',
        '--reranker', 'catboost',
        '--knn-indexes', 'base:sns1400:dcl35000',
        '--base-knn-index', 'base',
    )
    yatest.common.execute(cmd)
    return [local_canonical_file(output_path)]


def test_2dssms_2knns_reranker_gcAssessorsExperiment():
    output_path = yatest.common.test_output_path('output.tsv')
    cmd = (
        NLGSEARCH_SIMPLE_PATH,
        '-i', yatest.common.work_path('input.tsv'),
        '-o', output_path,
        '--data', yatest.common.work_path('small_index'),
        '--dssms', 'small_dssm,small_dssm_2',
        '--base-dssm', 'small_dssm',
        '--rerankers-to-load', 'assessors_catboost:sf8',
        '--reranker', 'assessors_catboost',
        '--knn-indexes', 'base:sns1400:dcl35000,assessors:sns1400:dcl35000',
        '--base-knn-index', 'base',
    )
    yatest.common.execute(cmd)
    return [local_canonical_file(output_path)]
