# coding: utf-8

import json

import vh

from alice.acceptance.cli.marker_tests.lib import common
from alice.acceptance.cli.marker_tests.lib import lazy_helpers
from alice.acceptance.cli.marker_tests.lib import op

from library.python import resource


def compare_and_assert(requests_vh_table, reference_requests, global_options):
    actual_requests = op.MR_TABLE_TO_JSON_OP(
        _inputs={
            'table': requests_vh_table,
        },
        _options={
            'yt-token': global_options.yt_token,
            'output-columns': ['VinsResponse', 'RequestId', 'SetraceUrl', 'SessionId', 'SessionSequence']
        },
    )
    reference_requests_data = vh.data_from_str(
        json.dumps(reference_requests).encode('utf-8'),
        name='marker tests context data',
        data_type='json',
    )
    (_, _, _, fail_flag) = lazy_helpers.compare_results(reference_requests_data, actual_requests)
    lazy_helpers.assert_test_result(fail_flag)


def build_common_subgraph(data_dir, global_options, config, use_linked_resources):
    all_requests = []
    if use_linked_resources:
        for test_json_request in common.concat_json_files_from_resources(resource.iterkeys('/marker_tests/data')):
            all_requests.extend(common.compose_context_request(test_json_request))
    else:
        for test_json_request in common.concat_json_files(common.all_json_files(data_dir)):
            all_requests.extend(common.compose_context_request(test_json_request))

    json_out = vh.data_from_str(
        json.dumps(all_requests).encode('utf-8'),
        name='marker tests data',
        data_type='json',
    )
    cache_sync_out = op.SINGLE_OPTION_TO_JSON(
        _options={'input': vh.OptionExpr('{"timestamp": "${global.cache_sync}"}')}
    )['output']
    input_table = op.JSON_TO_MR_TABLE_OP(
        _inputs={
            'json': json_out,
        },
        _options={
            'yt-token': global_options.yt_token,
            'mr-account': global_options.mr_account,
            'mr-default-cluster': config.yt.proxy,  # global_options.yt_proxy, https://paste.yandex-team.ru/899252
            'mr-output-ttl': global_options.mr_output_ttl,
        },
        _dynamic_options=cache_sync_out,
    )['new_table']
    return (all_requests, input_table)


def build_graph_with_scraper(data_dir, global_options, config, use_linked_resources, scraper_mode):
    all_requests, input_table = build_common_subgraph(data_dir, global_options, config, use_linked_resources)
    downloaded_results = op.DEEP_DOWNLOADER_OP(
        _inputs={
            'input': input_table,
        },
        _options={
            'VinsUrl': global_options.vins_url,
            'UniproxyUrl': global_options.uniproxy_url,
            'uniproxy-fetcher-mode': 'auto',
            'oauth': global_options.ya_plus_token,
            'yt-token': global_options.yt_token,
            'yql-token': global_options.yql_token,
            'arcanum_token': global_options.arcanum_token,
            'input_cluster': config.yt.proxy,
            'mr-account': global_options.mr_account,
            'mr-output-ttl': global_options.mr_output_ttl,
            'mr-output-path': global_options.mr_output_path,
            'yt-pool': global_options.yt_pool,
            'cache_sync': global_options.cache_sync,
            'AsrChunkSize': config.alice.asr_chunk_size,
            'scraper-timeout': config.alice.scraper_timeout,
            'ScraperOverYtPool': config.alice.scraper_pool,
            'experiments': common.prepare_experiments(config.alice.experiments),
            'Scraper-token': config.soy.token,
            'Scraper-contour': {'test': '6', 'soy_test': '2'}.get(scraper_mode),
        },
    )
    uniproxy_out = downloaded_results['output']
    compare_and_assert(uniproxy_out, all_requests, global_options)


def build_graph(data_dir, global_options, config, use_linked_resources, scraper_mode):
    build_graph_with_scraper(data_dir, global_options, config, use_linked_resources, scraper_mode)
