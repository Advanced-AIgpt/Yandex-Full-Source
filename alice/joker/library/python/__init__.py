import html
import json
import logging
import os
import os.path
import requests
import socket
import subprocess
import sys
import yatest.common
import yatest.common.network

from alice.joker.library.python.run_info import RunInfo
from alice.joker.library.python.util import joker_bin_path


logger = logging.getLogger(__name__)


def urljoin(hostport, path):
    return '/'.join([hostport, path])


class Config(object):
    def __init__(self, project, s3_bucket_name, s3_host, workdir, session_id, runinfo_file, arcadia_root, skip_headers=[], skip_cgis=[], stub_force_update=False, stub_fetch_if_not_exists=False):
        self.project = project
        self.session_id = session_id
        self.s3_bucket_name = s3_bucket_name
        self.s3_host = s3_host
        self.workdir = workdir
        self.runinfo_file = runinfo_file
        self.binary_path = joker_bin_path(arcadia_root)
        self.skipped_headers = skip_headers
        self.skipped_cgis = skip_cgis
        self.stub_force_update = stub_force_update
        self.stub_fetch_if_not_exists = stub_fetch_if_not_exists

        self._s3_credentials = None

    def set_s3_credentials(self, id, secret):
        self._s3_credentials = {'KeyId': id, 'KeySecret': secret}

    def s3_credentials(self):
        return self._s3_credentials

    def threads(self):
        return 10  # FIXME (petrk) use params


class JokerSyncRequest:
    class Exception(Exception):
        pass

    def __init__(self, joker):
        self.joker = joker
        self.body = None

    def add_stub(self, data):
        if self.body is not None:
            self.body += '\n'
        else:
            self.body = ''

        self.body += 'sync\t{}'.format(data)

    def check_run(self):
        if self.body is None:
            return

        response = requests.post(urljoin(self.joker.scheme, 'sync_versioned'), params={'session_id': self.joker.session_id}, data=self.body)
        response.raise_for_status()
        resp_json = response.json()

        invalid_actions = []
        for action in resp_json['actions']:
            if 'success' not in action:
                invalid_actions.append(action)

        if invalid_actions:
            raise JokerSyncRequest.Exception(invalid_actions)


class JokerMocker:
    def __init__(self, config, url_requester=None):
        self.config = config

        if not os.path.exists(config.workdir):
            os.makedirs(config.workdir)

        with yatest.common.network.PortManager() as pm:
            self.port = pm.get_port()
            json_config = {
                'Backend': {
                    'Type': 's3',
                    'Params': {
                        'Host': config.s3_host,
                        'Bucket': config.s3_bucket_name,
                        'Timeout': '150ms'
                    }
                },
                'Server': {
                    'HttpPort': self.port,
                    'HttpThreads': config.threads(),
                },
                'WorkDir': config.workdir,
                'StubIdGenerator': {
                    'SkipHeader': config.skipped_headers,
                    'SkipCGI': config.skipped_cgis,
                }
            }

            logger.debug('joker config (wo creds): %s', json_config)

            if config.s3_credentials() is not None:
                json_config['Backend']['Params']['Credentials'] = config.s3_credentials()

            self.config_filename = os.path.join(config.workdir, 'config.json')
            with open(self.config_filename, 'w') as f:
                f.write(json.dumps(json_config, indent=1))

            self._io = subprocess.Popen([self.config.binary_path, 'server', self.config_filename], stdout=sys.stdout, stderr=sys.stderr)

            self.host_port = '{}:{}'.format(socket.gethostname(), self.port)
            self.scheme = 'http://{}'.format(self.host_port)

            if url_requester:
                # Wait until joker is started
                resp = url_requester('{}/admin?action=ping'.format(self.scheme))
                assert resp == 'pong'

            self.proxy_header = '{}:{}'.format(socket.gethostname(), self.port)

        # FIXME add posting urls
        session_params = {
            'id': config.session_id,
            'fetch_if_not_exists': config.stub_fetch_if_not_exists,
            'force_update': config.stub_force_update,
        }
        session = requests.post(self.url('session'), params=session_params).json()
        if 'session_id' not in session:
            raise Exception('unable to obtain session from joker')
        self.session_id = html.escape(session['session_id'])
        # FIXME add check for errors

    def url(self, path):
        return urljoin(self.scheme, path)

    def stop(self):
        # shutdown joker gracefuly
        resp = requests.get(self.url('admin'), params=[('action', 'shutdown')])
        logger.debug('joker stop response: %s', resp.content)
        self._io.wait()

    def test_stub_id(self, test_id):
        return 'prj={}&sess={}&test={}'.format(html.escape(self.config.project), html.escape(self.session_id), html.escape(test_id))

    def insert_headers(self, test_id, headers):
        headers['x-yandex-via-proxy'] = self.proxy_header
        headers['x-yandex-proxy-header-x-yandex-joker'] = self.test_stub_id(test_id)
        skip_vins_bass = 0 if 'SKIP_VINS_BASS' not in os.environ else int(os.environ['SKIP_VINS_BASS'])
        if skip_vins_bass:
            headers['x-yandex-via-proxy-skip'] = 'Vins'
            headers['x-yandex-proxy-header-x-yandex-proxy-header-x-yandex-via-proxy'] = self.proxy_header
            headers['x-yandex-proxy-header-x-yandex-proxy-header-x-yandex-proxy-header-x-yandex-joker'] = self.test_stub_id(test_id)

    def write_runinfo(self):
        if self.config.runinfo_file is not None:
            RunInfo(filename=self.config.runinfo_file, joker_bin=self.config.binary_path, config_file=self.config_filename, session_id=self.session_id).write()

    def info(self):
        cmd = [self.config.binary_path, 'session', '--id', self.session_id, 'info', self.config_filename]
        run_io = subprocess.Popen(cmd)
        run_io.wait(timeout=20)

    def push(self, stubs_dir, rewrite=True):
        cmd = [self.config.binary_path, 'session', '--id', self.session_id, 'push', self.config_filename]
        logger.debug('Run joker: {}'.format(' '.join(cmd)))
        push_io = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=sys.stderr, stdin=None)
        line = push_io.stdout.readline().decode('utf-8')
        name = ''
        f = open(stubs_dir + '/junk.txt', 'w')
        while line:
            if line.startswith(' '):
                f.write('1\t{}\n'.format(line.lstrip().rstrip()))
            else:
                name = line.split('\n')[0].split('/')[1] + '.txt'
                f.close()
                f = open(stubs_dir + '/' + name, 'w')
            line = push_io.stdout.readline().decode('utf-8')
        f.close()
        push_io.wait()

        if push_io.returncode:
            raise Exception("Something wrong with push command (rcode %d)", push_io.returncode)
