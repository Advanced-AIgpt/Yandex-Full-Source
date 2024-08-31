# coding: utf-8

import json
import os
import subprocess
import uuid

import click
from google.protobuf import json_format

from library.python import resource
import yt.wrapper as yt
from yt.wrapper import yson

from search.scraper_over_yt.mapper.lib.proto import voice_output_pb2


PRODUCTION_FLAGS = json.dumps(json.loads(resource.find('flags_production.json')))


def prepare_reducer(key, values):
    values = list(values)
    values.sort(key=lambda t: -t['session_sequence'])
    yield {'session_requests': values, 'session_id': key['session_id']}


def run_reduce(reducer, input_table, output_table, reduce_by):
    with yt.TempTable() as sorted_table:
        yt.run_sort(input_table, sorted_table, sort_by=reduce_by)
        yt.run_reduce(reducer, sorted_table, output_table, reduce_by=reduce_by)


def unpack_mapper(row):
    proto_content = yson.yson_types.get_bytes(row['FetchedResult'])
    voice_output = voice_output_pb2.TVoiceOutput.FromString(proto_content)
    voice_output_dict = json_format.MessageToDict(voice_output, including_default_value_fields=True)
    voice_output_dict['Error'] = row['Error']
    yield voice_output_dict


@click.command()
@click.option('--input', required=True, help='Input table path')
@click.option('--output', required=True, help='Output table path')
@click.option('--yt-proxy', required=True, envvar='YT_PROXY', help='YT cluster proxy, envvar: YT_PROXY')
@click.option('--yt-token', required=True, envvar='YT_TOKEN', help='Yt token, envvar: YT_TOKEN')
@click.option('--mr-account', default='alice-dev', show_default=True, help='Yt quota account')
@click.option('--oauth-token', envvar='ALICE_OAUTH_TOKEN',
              help='OAuth token for scenarios with authorization, envvar: ALICE_OAUTH_TOKEN')
@click.option('--experiments', default='', help='Additional experiments. Format: exp1=val,exp2=val2')
@click.option('--uniproxy-fetcher-mode', default='auto',
              type=click.Choice(['voice', 'text', 'auto']),
              show_default=True,
              help='Fetcher type for uniproxy requests.')
@click.option('--uniproxy-url', default='wss://beta.uniproxy.alice.yandex.net/alice-uniproxy-hamster/uni.ws',
              show_default=True, help='Uniproxy url')
@click.option('--megamind-url', default='http://vins.hamster.alice.yandex.net/speechkit/app/pa/',
              show_default=True, help='Megamind url')
@click.option('--asr-chunk-size', default='-1', show_default=True,
              help='Asr chunk size')
@click.option('--asr-chunk-delay-ms', default='0', show_default=True,
              help='Asr chunk delay between chunks in milliseconds')
@click.option('--flags', default=PRODUCTION_FLAGS, show_default=True,
              help='Downloader feature flags, json-list format')
@click.option('--no-prepare', default=False, is_flag=True)
@click.option('--prepare-only', default=False, is_flag=True)
def main(input, output, yt_proxy, yt_token, mr_account, oauth_token, experiments, uniproxy_fetcher_mode,
         uniproxy_url, megamind_url, asr_chunk_size, asr_chunk_delay_ms, flags, no_prepare, prepare_only):
    yt.config['proxy']['url'] = yt_proxy
    yt.config['token'] = yt_token
    tmp_tables_dir = '//home/%s/%s/tmp' % (mr_account, os.environ['USER'])
    yt.config['remote_temp_tables_directory'] = tmp_tables_dir

    with os.popen('ya dump root') as f:
        arc_root = f.read()

    fetcher_path = os.path.join(arc_root, 'search/scraper_over_yt/mapper/mapper')
    binary_path = os.path.join(arc_root, 'alice/acceptance/modules/request_generator/scrapper/bin/downloader_inner')

    with yt.TempTable() as reduced_for_downloader:
        if no_prepare:
            reduced_for_downloader = input
        else:
            if prepare_only:
                reduced_for_downloader = output
            run_reduce(prepare_reducer, input, reduced_for_downloader, ['session_id'])

        if prepare_only:
            return

        downloaded_table = tmp_tables_dir + '/' + str(uuid.uuid4())
        fetcher_cmd = [
            fetcher_path, 'deep_uniproxy',
            '--input-path', reduced_for_downloader,
            '--output-path', downloaded_table,
            '--uniproxy-url', uniproxy_url,
            '--vins-url', megamind_url,
            '--asr-chunk-size', asr_chunk_size,
            '--asr-chunk-delay-ms', asr_chunk_delay_ms,
            '--binary-path', binary_path,
            '--binary-params', json.dumps({
                'process_id': 'dummy_downloader',
                'oauth_token': oauth_token,
                'experiments': experiments,
                'fetcher_mode': uniproxy_fetcher_mode,
                'retry_profile': 'music_video_search',
                # 'downloader_flags': json.loads(flags),
                'downloader_flags': flags,
            }),
            '--flags', flags,
        ]
        print(' '.join(fetcher_cmd))
        with subprocess.Popen(fetcher_cmd, stdout=subprocess.PIPE) as p:
            print(p.stdout.read())
            p.wait()

        error_suffix = '.error'
        yt.run_map(unpack_mapper, downloaded_table, output)
        yt.run_map(unpack_mapper, downloaded_table + error_suffix, output + error_suffix)
        yt.remove(downloaded_table)
        yt.remove(downloaded_table + error_suffix)


if __name__ == '__main__':
    main()
