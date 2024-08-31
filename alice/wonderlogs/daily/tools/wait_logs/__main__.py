import time

import click
import datetime
import pytz
import yt.wrapper


@click.command()
@click.option('--downloaded-table')
@click.option('--uniproxy-logs-dir')
@click.option('--megamind-logs-dir')
@click.option('--asr-logs-dir')
@click.option('--dt-from-out')
@click.option('--dt-to-out')
@click.option('--uniproxy-logs-dir-out')
@click.option('--uniproxy-logs-from-out')
@click.option('--uniproxy-logs-to-out')
@click.option('--megamind-logs-dir-out')
@click.option('--megamind-logs-from-out')
@click.option('--megamind-logs-to-out')
@click.option('--asr-logs-dir-out')
@click.option('--asr-logs-from-out')
@click.option('--asr-logs-to-out')
@click.option('--yt-cluster')
@click.option('--yt-token')
def main(downloaded_table, uniproxy_logs_dir, megamind_logs_dir, asr_logs_dir, dt_from_out, dt_to_out,
         uniproxy_logs_dir_out, uniproxy_logs_from_out, uniproxy_logs_to_out, megamind_logs_dir_out,
         megamind_logs_from_out, megamind_logs_to_out, asr_logs_dir_out, asr_logs_from_out, asr_logs_to_out, yt_cluster,
         yt_token):
    yt.wrapper.config['token'] = yt_token
    yt.wrapper.config.set_proxy(yt_cluster)

    dt_from = None
    dt_to = None

    print('dt_from, dt_to loop')
    for row in yt.wrapper.read_table(downloaded_table):
        dt = datetime.datetime.strptime(row['DownloaderTime'], '%Y%m%dT%H%M%S')
        dt_from = min(dt_from, dt) if dt_from else dt
        dt_to = max(dt_to, dt) if dt_to else dt
    print('dt_from:', dt_from, 'dt_to: ', dt_to)

    dt_from -= datetime.timedelta(minutes=20)
    dt_to += datetime.timedelta(minutes=20)

    uniproxy_logs_dir = uniproxy_logs_dir.rstrip('/')
    megamind_logs_dir = megamind_logs_dir.rstrip('/')
    asr_logs_dir = asr_logs_dir.rstrip('/')

    def five_min_replacer(dt):
        return dt.replace(second=0, microsecond=0, minute=(dt.minute // 5 * 5))

    def day_replacer(dt):
        return dt.replace(hour=0, minute=0, second=0, microsecond=0)

    def init_logs():
        return {'dir': None, 'from': None, 'to': None}

    uniproxy = init_logs()
    megamind = init_logs()
    asr = init_logs()

    asr['dir'] = asr_logs_dir
    found_table = False
    dt = dt_from
    while not found_table:
        table = dt.strftime('%Y-%m-%d')
        full_path_table = asr_logs_dir + '/' + table
        if yt.wrapper.exists(full_path_table):
            asr['from'] = table
            asr['to'] = table
            found_table = True
            print(full_path_table, 'exists')
        else:
            print(full_path_table, "doesn't exist")
        dt -= datetime.timedelta(days=1)

    all_present = False
    first = True
    while not all_present:
        if not first:
            time.sleep(5 * 60)
        first = False

        for directory, step, date_replacer, date_format in (
            ('stream/5min', datetime.timedelta(minutes=5),
             five_min_replacer, '%Y-%m-%dT%H:%M:%S'),
                ('1d', datetime.timedelta(days=1), day_replacer, '%Y-%m-%d')):

            all_present = True

            datetime_from = date_replacer(dt_from)
            datetime_to = date_replacer(dt_to)

            print('qq', datetime_from, datetime_to)

            for logs_dir, logs_info in ((uniproxy_logs_dir, uniproxy), (megamind_logs_dir, megamind)):

                dt = datetime_from
                while dt <= datetime_to and all_present:
                    t = logs_dir + '/' + directory + \
                        '/' + dt.strftime(date_format)
                    if not yt.wrapper.exists(t):
                        print(t, "doesn't exist")
                        all_present = False
                    else:
                        print(t, 'exists')

                    dt += step

                logs_info['dir'] = logs_dir + '/' + directory
                logs_info['from'] = datetime_from.strftime(date_format)
                logs_info['to'] = datetime_to.strftime(date_format)

            if all_present:
                break
    tz = pytz.timezone('Europe/Moscow')
    with open(dt_from_out, 'w') as f:
        f.write(tz.localize(dt_from).strftime('%Y-%m-%d %H:%M:%S%z'))
    with open(dt_to_out, 'w') as f:
        f.write(tz.localize(dt_to).strftime('%Y-%m-%d %H:%M:%S%z'))

    for directory, from_table, to_table, data in (
        (uniproxy_logs_dir_out, uniproxy_logs_from_out,
         uniproxy_logs_to_out, uniproxy),
        (megamind_logs_dir_out, megamind_logs_from_out,
         megamind_logs_to_out, megamind),
            (asr_logs_dir_out, asr_logs_from_out, asr_logs_to_out, asr)):

        for file, value in ((directory, data['dir']), (from_table, data['from']), (to_table, data['to'])):
            with open(file, 'w') as f:
                f.write(value)


if __name__ == '__main__':
    main()
