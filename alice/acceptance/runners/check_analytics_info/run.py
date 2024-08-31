import vh

from alice.acceptance.runners.common_operations import (
    build_arcadia_project_op,
    get_mr_table_op,
    ue2e_downloader_op,
    yql_2_op
)
from alice.acceptance.runners.config import global_options

check_analytics_info_op = vh.op(id='9d3bd383-2873-437d-9044-5a67abd7dd98')

MR_OUTPUT_TTL = 10

YQL_CONCAT_SUCC_AND_VALID_ERROR_RESPONSES = '''$good_error_responses = (
SELECT
    *
FROM
    {{input2}}
WHERE
    FetchedError LIKE '%Retries for tunneller validation are over.%'
);

$good_with_good_error_responses = (
SELECT
    *
FROM (
        SELECT
            *
        FROM
            {{input1}}
    UNION ALL
        SELECT
            *
        FROM
            $good_error_responses
)
);

INSERT INTO {{output1}}
    SELECT
        *
    FROM $good_with_good_error_responses;

$script = @@
from yql.typing import *

def assert_ratio(
    good: Optional[Uint64],
    bad: Optional[Uint64]) -> Bool:
    MAX_ERROR_SHARE = 0.005
    value = bad / (bad + good) < MAX_ERROR_SHARE
    assert value
    return value
@@;

$assert_ratio = Python3::assert_ratio($script);

$bad = (
SELECT
        *
    FROM
        {{input2}} AS errors
    LEFT ONLY JOIN
        $good_error_responses AS good_errors
    USING
        (RequestId)
);

$good_count = SELECT COUNT(*) FROM $good_with_good_error_responses;
$bad_count = SELECT COUNT(*) FROM $bad;

SELECT
    $assert_ratio(
        $good_count,
        $bad_count
    )
'''


def build_graph(config, _after=None):
    run_python_udf_bin = build_arcadia_project_op(
        _name='build run python udf',
        _options={
            'arcadia_url': 'arcadia:/arc/trunk/arcadia',
            'arcadia_revision': config.check_analytics_info.revision,
            'build_type': 'release',
            'targets': 'yql/tools/run_python_udf',
            'arts': 'yql/tools/run_python_udf/run_python_udf',
        },
    )

    check_analytics_info_so = build_arcadia_project_op(
        _name='build checker analytics_info',
        _options={
            'arcadia_url': 'arcadia:/arc/trunk/arcadia',
            'arcadia_revision': config.check_analytics_info.revision,
            'build_type': 'release',
            'targets': 'alice/acceptance/modules/check_analytics_info/so',
            'arts': 'alice/acceptance/modules/check_analytics_info/so/libcheck_analytics_info.so',
        },
    )

    input_table = get_mr_table_op(
        _name='get input table',
        _options={
            'cluster': config.yt.proxy,
            'creationMode': 'NO_CHECK',
            'table': config.check_analytics_info.input_table,
            'yt-token': vh.get_yt_token_secret(),
        },
    )

    downloader_test_table = ue2e_downloader_op(
        _name='test downloader',
        _inputs={
            'evaluation_set': input_table['outTable'],
        },
        _options={
            'uniproxy_url': config.check_analytics_info.uniproxy_url,
            'vins_url': config.check_analytics_info.test_vins_url,
            'oauth': global_options.ya_plus_token,
            'arcanum_token': config.alice_common.arcanum_robot_token,
            'yt-token': vh.get_yt_token_secret(),
            'yql-token': global_options.yql_token,
            'Scraper-token': global_options.soy_token,
            'input_cluster': config.yt.proxy,
            'ScraperOverYtPool': config.alice_common.scraper_over_yt_pool,
            'mr-account': vh.get_mr_account(),
            'experiments': config.check_analytics_info.experiments,
            'mr-output-ttl': MR_OUTPUT_TTL,
            'cache_sync': global_options.cache_sync,
            'retry-profile': 'no_retry',
        },
    )

    downloader_stable_table = ue2e_downloader_op(
        _name='stable downloader',
        _inputs={
            'evaluation_set': input_table['outTable'],
        },
        _options={
            'uniproxy_url': config.check_analytics_info.uniproxy_url,
            'vins_url': config.check_analytics_info.stable_vins_url,
            'oauth': global_options.ya_plus_token,
            'arcanum_token': config.alice_common.arcanum_robot_token,
            'yt-token': vh.get_yt_token_secret(),
            'yql-token': global_options.yql_token,
            'Scraper-token': global_options.soy_token,
            'input_cluster': config.yt.proxy,
            'ScraperOverYtPool': config.alice_common.scraper_over_yt_pool,
            'mr-account': vh.get_mr_account(),
            'experiments': config.check_analytics_info.experiments,
            'mr-output-ttl': MR_OUTPUT_TTL,
            'cache_sync': global_options.cache_sync,
            'retry-profile': 'no_retry',
        },
    )

    concat_test_table = yql_2_op(
        _name='concat test successful and valid error responses',
        _inputs={
            'input1': downloader_test_table['downloader_results'],
            'input2': downloader_test_table['downloader_errors'],
        },
        _options={
            'request': YQL_CONCAT_SUCC_AND_VALID_ERROR_RESPONSES,
            'py_version': 'Python2',
            'syntax_version': 'v1',
            'mr-default-cluster': config.yt.proxy,
            'mr-account': vh.get_mr_account(),
            'yt-token': vh.get_yt_token_secret(),
            'yql-token': global_options.yql_token,
            'mr-output-ttl': MR_OUTPUT_TTL,
        },
    )

    concat_stable_table = yql_2_op(
        _name='concat stable successful and valid error responses',
        _inputs={
            'input1': downloader_stable_table['downloader_results'],
            'input2': downloader_stable_table['downloader_errors'],
        },
        _options={
            'request': YQL_CONCAT_SUCC_AND_VALID_ERROR_RESPONSES,
            'py_version': 'Python2',
            'syntax_version': 'v1',
            'mr-default-cluster': config.yt.proxy,
            'mr-account': vh.get_mr_account(),
            'yt-token': vh.get_yt_token_secret(),
            'yql-token': global_options.yql_token,
            'mr-output-ttl': MR_OUTPUT_TTL,
        },
    )

    joined_test_and_stable_table = yql_2_op(
        _name='join test and stable responses',
        _inputs={
            'input1': concat_test_table['output1'],
            'input2': concat_stable_table['output1'],
        },
        _options={
            'request': '''INSERT INTO {{output1}}
    SELECT test.RequestId AS RequestId, test.VinsResponse, stable.VinsResponse
        FROM {{input1}} AS test
    FULL JOIN
        {{input2}} AS stable
    USING (RequestId)''',
            'py_version': 'Python2',
            'syntax_version': 'v1',
            'mr-default-cluster': config.yt.proxy,
            'mr-account': vh.get_mr_account(),
            'yt-token': vh.get_yt_token_secret(),
            'yql-token': global_options.yql_token,
            'mr-output-ttl': MR_OUTPUT_TTL,
        },
    )

    check_analytics_info_op(
        _name='check analytics info',
        _inputs={
            'input_table': joined_test_and_stable_table['output1'],
            'runner': run_python_udf_bin['ARCADIA_PROJECT'],
            'so': check_analytics_info_so['ARCADIA_PROJECT'],
        },
        _options={
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account(),
            'max-ram': 2000,
            'yql-token': global_options.yql_token,
        },
    )
