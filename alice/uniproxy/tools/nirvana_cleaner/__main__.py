import subprocess
import argparse
import datetime
from datetime import timedelta
import os
import yt.wrapper as yt

os.environ['YT_PROXY'] = 'hahn'


def process(table, column, day):
    filename = "{}.{}.{}".format(table, day, column)
    table_name = "//home/voice-speechbase/uniproxy/{}/{}".format(table, day)
    with open(filename, 'w') as f:
        for row in yt.read_table(yt.TablePath(table_name)):
            key = row[column]
            if key is not None:
                f.write(key + "\n")
    return subprocess.Popen(['./mdsdelete', '-file', filename, '-jobs', '1500'])


def main():
    yt.config.set_proxy('hahn')
    fields = [
        ('logs_unified_qloud', 'mds_key'),
        ('logs_unified_qloud', 'SpotterStreamKey'),
        ('logs_unified_qloud', 'LongSpotterStreamKey'),
        ('spotter', 'mds_key')
    ]

    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--period', dest='period',
        help='period in days',
        default=None,
        required=True
    )
    parser.add_argument(
        '--start-day', dest='start_day',
        help='start day in format "2020-01-31"',
        default=None,
        required=True
    )
    args = parser.parse_args()

# load mds_delete
    subprocess.Popen(['curl', '--silent', '--insecure', '-o', 'mdsdelete', 'https://proxy.sandbox.yandex-team.ru/3064746424']).wait()
    subprocess.Popen(['chmod', '777', './mdsdelete']).wait()

    start = datetime.datetime.strptime(args.start_day, '%Y-%m-%d').date()
    for i in range(int(args.period)):
        day = (start + timedelta(days=i)).strftime('%Y-%m-%d')
        processes = []
        for table, column in fields:
            processes.append(process(table, column, day))

        for p in processes:
            p.wait()

if __name__ == '__main__':
    main()
