import click
import datetime
import google.protobuf.text_format
import time
import uuid
import yt.wrapper as yt
from alice.wonderlogs.daily.error_nottifier.lib.config_parser import generate_thresholds
from alice.wonderlogs.daily.error_nottifier.lib.juggler import send_raw_event
from alice.wonderlogs.daily.error_nottifier.lib.reporter import report
from alice.wonderlogs.protos.asr_prepared_pb2 import TAsrPrepared
from alice.wonderlogs.protos.error_threshold_config_pb2 import TErrorThresholdConfig
from alice.wonderlogs.protos.megamind_prepared_pb2 import TMegamindPrepared
from alice.wonderlogs.protos.uniproxy_prepared_pb2 import TUniproxyPrepared
from alice.wonderlogs.protos.wonderlogs_pb2 import TWonderlog
from functools import partial


def create_temp_table(tmp_dir, prefix):
    name = tmp_dir.rstrip('/') + '/' + prefix + str(uuid.uuid4())
    expiration_time = int(time.mktime((datetime.datetime.now() + datetime.timedelta(days=7)).timetuple())) * 1000
    yt.create('table', name, recursive=True, ignore_existing=True, attributes={'expiration_time': expiration_time})
    return name


def count_process_reason_reducer(key, input_row_iterator, row_count):
    process = key[b'process']
    reason = key[b'reason']
    count = 0
    for input_row in input_row_iterator:
        count += 1
    share = 1.0 * count / row_count
    yield {b'process': process, b'reason': reason, b'count': count, b'share': share}


@click.command()
@click.option('--yt-cluster')
@click.option('--yt-pool')
@click.option('--error-config-file')
@click.option('--uniproxy-prepared')
@click.option('--uniproxy-prepared-errors')
@click.option('--megamind-prepared')
@click.option('--megamind-prepared-errors')
@click.option('--asr-prepared')
@click.option('--asr-prepared-errors')
@click.option('--wonderlogs')
@click.option('--wonderlogs-errors')
@click.option('--tmp-dir')
def main(yt_cluster, yt_pool, error_config_file, uniproxy_prepared, uniproxy_prepared_errors, megamind_prepared,
         megamind_prepared_errors, asr_prepared, asr_prepared_errors, wonderlogs, wonderlogs_errors, tmp_dir):
    yt.config.set_proxy(yt_cluster)
    if yt_pool:
        yt.config['pool'] = yt_pool

    with open(error_config_file) as f:
        error_config = google.protobuf.text_format.Parse(f.read(),
                                                         TErrorThresholdConfig())

    uniproxy_prepared_data = (
        'uniproxy_prepared', uniproxy_prepared, uniproxy_prepared_errors, error_config.UniproxyPrepared,
        TUniproxyPrepared.TError)
    megamind_prepared_data = (
        'megamind_prepared', megamind_prepared, megamind_prepared_errors, error_config.MegamindPrepared,
        TMegamindPrepared.TError)
    asr_prepared_data = (
        'asr_prepared', asr_prepared, asr_prepared_errors, error_config.AsrPrepared, TAsrPrepared.TError)
    wonderlogs_data = ('wonderlogs', wonderlogs, wonderlogs_errors, error_config.Wonderlogs, TWonderlog.TError)

    for operation, table, error_table, config_thresholds, error in (
            uniproxy_prepared_data, megamind_prepared_data, asr_prepared_data, wonderlogs_data):
        yt_operation_name = operation.replace('_', '-')
        with yt.Transaction():
            sorted_tmp_table = create_temp_table(tmp_dir, yt_operation_name + '-errors-sorted')
            output_table = create_temp_table(tmp_dir, yt_operation_name + '-errors-output')

            yt.run_sort(source_table=error_table, destination_table=sorted_tmp_table, sort_by=['process', 'reason'])

            yt.run_reduce(partial(count_process_reason_reducer, row_count=yt.row_count(table)),
                          source_table=sorted_tmp_table, destination_table=output_table, reduce_by=['process', 'reason'],
                          format=yt.YsonFormat(encoding=None))

            thresholds = generate_thresholds(error, config_thresholds)

            rows = yt.read_table(output_table)
            status, checks = report(rows, thresholds)
            tags = ['__'.join(lst) for lst in checks]
            send_raw_event(host='yt.job', service='wonderlogs.' + operation + '.raw', status=status, tags=tags,
                           source='yt.job')


if __name__ == "__main__":
    main()
