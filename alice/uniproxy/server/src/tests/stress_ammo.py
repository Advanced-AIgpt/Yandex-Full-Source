#!/usr/bin/env python
# -*- coding=utf8 -*-
from __future__ import print_function

import argparse
import codecs
import datetime
import json
import io
import logging
import os
import random
import re
import subprocess
import sys
import time
import urllib2
import wave
import yt.wrapper as yt
import yt.yson as yson

from contextlib import closing
from distutils.util import strtobool
from yql.api.v1.client import YqlClient

sys.path.append('/usr/lib/yandex/voice/python-decoder')  # noqa
from decoder import Decoder  # pylint: disable=no-name-in-module


def retries(max_tries, delay=1, backoff=2, exceptions=(Exception,), hook=None, log=True, raise_class=None):
    """
        Wraps function into subsequent attempts with increasing delay between attempts.
        Adopted from https://wiki.python.org/moin/PythonDecoratorLibrary#Another_Retrying_Decorator
        (copy-paste from arcadia sandbox-tasks/projects/common/decorators.py)
    """
    def dec(func):
        def f2(*args, **kwargs):
            current_delay = delay
            for n_try in xrange(0, max_tries + 1):
                try:
                    return func(*args, **kwargs)
                except exceptions as e:
                    if n_try < max_tries:
                        if log:
                            logging.exception(
                                "Error in function %s on %s try:\n%s\nWill sleep for %s seconds...",
                                func.__name__, n_try, e, current_delay
                            )
                        if hook is not None:
                            hook(n_try, e, current_delay)
                        time.sleep(current_delay)
                        current_delay *= backoff
                    else:
                        logging.error("Max retry limit %s reached, giving up with error:\n%s", n_try, e)
                        if raise_class is None:
                            raise
                        else:
                            raise raise_class("Max retry limit {} reached, giving up with error: {}".format(n_try, str(e)))

        return f2
    return dec


@retries(3, 10)
def get_and_check_audio(url, local_dir, min_dur, max_dur):
    filename = url.split('/')[-1]
    if local_dir:
        filename = os.path.join(local_dir, filename)
    dc = Decoder(8000)
    audio_data = b''
    decoded_audio_size = 0
    audio_duration = None
    wav_data = None
    with closing(urllib2.urlopen(url)) as sock:
    # with open('4063be70-3963-497c-95df-9aaaffae2487_aea824e4-bd89-4500-b85a-393e428ed90b_5.opus', 'rb') as sock:
        first_chunk = True
        while True:
            data = sock.read(4096*4)
            if first_chunk:
                if data.startswith('RIFF'):
                    # logging.debug('skip audio {} (RIFF != opus)')
                    data += sock.read()
                    wav_data = data
                    with io.BytesIO(data) as f:
                        wavef = wave.open(f)
                        audio_duration = wavef.getnframes() / float(wavef.getframerate())
                        break
                first_chunk = False
            if data:
                audio_data += data
                dc.write(data)
            else:
                dc.close()
            data = dc.read()
            while data is not None:
                decoded_audio_size += len(data)
                data = dc.read()
            if dc.eof():
                break
        if audio_duration is None:
            audio_duration = decoded_audio_size / (8000. * 2)
        if audio_duration < min_dur or max_dur < audio_duration:
            logging.debug('skip audio {} (duration={} out of limits)'.format(url, audio_duration))
            return None
        logging.debug('audio {} duration={} is Ok'.format(url, audio_duration))

    if wav_data:
        with io.BytesIO(wav_data) as fw, open(filename, 'wb') as f:
            proc = subprocess.Popen(['opusenc', '-', '-'], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
            out, err = proc.communicate(wav_data)
            f.write(out)
    else:
        with open(filename, 'wb') as f:
            f.write(audio_data)
    return filename


def norm_date(date):
    if date == 'yesterday':
        dt = datetime.datetime.now() - datetime.timedelta(1)
    elif date.startswith('-'):
        dt = datetime.datetime.now() - datetime.timedelta(days=int(date[1:]))
    else:
        dt = datetime.datetime.strptime(date, '%Y-%m-%d')
    return dt.strftime('%Y-%m-%d')


def main():
    """
        пример выборки команд на включение музыки из колоночных аннотаций за прошедших 5 дней
        (с послезавтра, так-как аннотации создаются с задержкой)
        python stress_ammo.py --from-date=-7 --to-date=-2 \
            --from-yt-folder=//home/voice/toloka/ru-RU/daily/quasar-general \
            --yql-cond='(text regexp "(поставь|включи).*(музыку|песню|группу)")' \
            --to-folder=stress_ammo_music 2>stress_ammo.log

        аналогичный пример выборки команд для biometry классификатора
        python stress_ammo.py --from-date=-7 --to-date=-2 \
            --from-yt-folder=//home/voice/toloka/ru-RU/daily/quasar-general \
            --yql-cond='(text regexp "давай знакомиться")'
            --to-folder=stress_ammo_bio 2>stress_ammo.log

    """
    root_logger = logging.getLogger('')
    file_logger = logging.StreamHandler()
    file_logger.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(filename)s:%(lineno)d %(message)s'))
    root_logger.addHandler(file_logger)
    root_logger.setLevel(logging.DEBUG)

    parser = argparse.ArgumentParser()
    parser.add_argument('--yql-cond', metavar='YQL_EXPR', type=str,
                        help='yql filter for selected records')
    parser.add_argument('--from-yt-folder', metavar='YT_PATH', type=str, default='//home/voice/toloka/ru-RU/daily/assistant',
                        help="folder for table with annotation records")
    parser.add_argument('--from-date', metavar='FROM_DATE', type=str, default='-1',
                        help="date for first log table (YYYY-MM-DD) or '-' + time_delta" +
                             " (days) from TO_DATE (default: 0 == use only TO_DATE day)")
    parser.add_argument('--to-date', metavar='TO_DATE', type=str, default='-1',
                        help='date for last log table (YYYY-MM-DD or yesterday) (default: yesterday)')
    parser.add_argument('--min-audio-duration', type=int, default=3)
    parser.add_argument('--max-audio-duration', type=int, default=10)
    parser.add_argument('--to-folder', metavar='FOLDERNAME', type=str, default='stress_ammo',
                        help='output (text/audio files) folder (will be created if not exist) (default: stress_ammo)')

    context = parser.parse_args()

    if os.path.exists(context.to_folder):
        if not os.path.isdir(context.to_folder):
            raise Exception('{} already exist and it is not directory'.format(context.to_folder))
    else:
        os.makedirs(context.to_folder)

    with open(os.getenv('HOME') + '/.yt/token') as f:
        yt.config['token'] = f.read().strip()
    with open(os.getenv('HOME') + '/.yql/token') as f:
        yql_token = f.read().strip()

    yt.config["proxy"]["url"] = "hahn.yt.yandex.net"

    from_date = norm_date(context.from_date)
    to_date = norm_date(context.to_date)
    query = 'PRAGMA yt.InferSchema;\nuse hahn;\n\n'

    query += """
$tables = (
    select *
    from range([{yt_folder}], [{from_date}], [to_date])
);
""".format(
            yt_folder=context.from_yt_folder,
            from_date=from_date,
            to_date=to_date,
        )
    yql_cond = ''
    if context.yql_cond:
        yql_cond = 'where ' + context.yql_cond
    query += """
select text, url
from $tables
{}
;
""".format(yql_cond)

    client = YqlClient(db='hahn', token=yql_token)
    request = client.query(query.decode('utf-8'))
    logging.info('run YQL query {}'.format(query))
    request.run()  # just start the execution
    logging.info('wait query result')
    results = request.get_results()  # wait execution finish
    if not results.is_success:
        logging.error('request failed: status={}'.format(results.status))
        if results.errors:
            for error in results.errors:
                logging.error('returned error: {}'.format(error))
        exit(1)
    logging.info('query is success')

    audio_index = []
    for table in results:
        result = table
        table.fetch_full_data()
        n = 0
        for row in table.rows:
            n += 1
            text, url = row
            logging.debug('process audio from url={}'.format(url))
            local_file = get_and_check_audio(url, context.to_folder, context.min_audio_duration, context.max_audio_duration)
            if local_file:
                audio_index.append((text, url))
        print('from YQL selected {} records'.format(n))
    if not audio_index:
        print('nothing found')
        exit(42)
    print('after audio duration filter has {} records'.format(len(audio_index)))

    with codecs.open(os.path.join(context.to_folder, 'index'), 'w', encoding='utf-8') as f, \
        codecs.open(os.path.join(context.to_folder, 'text'), 'w', encoding='utf-8') as ft:
        for rec in audio_index:
            f.write(u'\t'.join(rec))
            f.write(u'\n')
        for rec in audio_index:
            ft.write(rec[0])
            ft.write(u'\n')
    logging.info('success finish')


if __name__ == '__main__':
    main()
