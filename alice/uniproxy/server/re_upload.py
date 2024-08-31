#!/usr/bin/env python2.7
#
# wrapper script for more reliable usage upload.sfx.py (sandbox upload client)
# https://st.yandex-team.ru/VOICE-4318
# first version stored at Arcadia:
# svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/junk/and42/lingware_update/re_upload.py
#
import json
import logging
import os
import shutil
import subprocess
import sys
import tempfile
import urllib2

from time import sleep
from contextlib import closing

logger = logging.getLogger(__name__)


class TmpDir:  # replacer TemporaryDirectory from python3
    def __init__(self, prefix='sandbox_uploader_'):
        self.prefix = prefix

    def __enter__(self):
        self.tmp_dir = tempfile.mkdtemp(prefix=self.prefix)
        logger.debug('create tmpdir={}'.format(self.tmp_dir))
        return self.tmp_dir

    def __exit__(self, exc_type, exc_value, traceback):
        if self.tmp_dir:
            logger.debug('remove tmp dir={}'.format(self.tmp_dir))
            shutil.rmtree(self.tmp_dir)
            self.tmp_dir = None


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
                            logger.error("Error in function {} on {} try:\n{}\nWill sleep for {} seconds...".format(
                                func.__name__, n_try, e, current_delay
                            ))
                        if hook is not None:
                            hook(n_try, e, current_delay)
                        sleep(current_delay)
                        current_delay *= backoff
                    else:
                        logger.error("Max retry limit {} reached, giving up with error:\n{}".format(n_try, e))
                        if raise_class is None:
                            raise
                        else:
                            raise raise_class("Max retry limit {} reached, giving up with error: {}".format(n_try, str(e)))

        return f2
    return dec


@retries(5, delay=4)
def getfile(url, filename, executable=False):
    logger.debug('download file "{}" from url {}'.format(filename, url))
    with closing(urllib2.urlopen(url, timeout=60)) as response:
        with open(filename, 'wb') as f:
            f.write(response.read())
    if executable:
        os.chmod(filename, 0744)


def hide_oauth_key(cmd):
    token_keys = [i for i, j in enumerate(cmd) if j == '-T']
    for ti in token_keys:
        cmd[ti+1] = '### secure-token ###'


@retries(5, delay=10)
def upload(upload_exe):
    """ execute upload_exe using current args + '-q -D -' enable output json
    """
    cmd = [
        upload_exe, '-q',
        '-D', '-',
    ]
    cmd += sys.argv[1:]
    cmd_copy = list(cmd)
    hide_oauth_key(cmd_copy)
    logger.debug('execute command: {}'.format(cmd_copy))
    try:
        result = subprocess.check_output(cmd, shell=False)
    except subprocess.CalledProcessError as cp_err:
        hide_oauth_key(cp_err.cmd)
        raise
    logger.debug('command execution ok, output:\n{}'.format(result))
    res_info = json.loads(result)
    logger.debug('sandbox resource={}'.format(res_info['id']))
    return res_info['id']


def main():
    root_logger = logging.getLogger('')
    file_logger = logging.StreamHandler()
    file_logger.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(filename)s:%(lineno)d %(message)s'))
    root_logger.addHandler(file_logger)
    root_logger.setLevel(logging.DEBUG)

    try:
        with TmpDir() as tmpdir:
            upload_exe = os.path.join(tmpdir, 'upload.sfx.py')
            getfile('http://proxy.sandbox.yandex-team.ru/last/SANDBOX_UPLOAD_SCRIPT', upload_exe, executable=True)
            res_id = upload(upload_exe)
            print res_id
            return 0
    except Exception as ex:
        logger.exception(ex)

if __name__ == "__main__":
    sys.exit(main())
